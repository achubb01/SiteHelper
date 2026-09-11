#include <assert.h>
#include <stdio.h>
#include "sitehelper_persistence.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t allocations_before_failure=SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count,size_t size);
void *__real_realloc(void *p,size_t size);
static int fail_allocation(void)
{
    if(allocations_before_failure==SIZE_MAX) return 0;
    if(allocations_before_failure--==0) {allocation_failed=1;return 1;}
    return 0;
}
void *__wrap_malloc(size_t n) {return fail_allocation()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s) {return fail_allocation()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n) {return fail_allocation()?NULL:__real_realloc(p,n);}
#endif

/* Physical values captured with the unmodified v9 generator, before cleanup.
 * Columns: type, stud subtype (-1 otherwise), U, Z, length, width, depth.
 * Includes plates, studs, noggins, headers and sills; order-independent match
 * also checks multiplicity, so no member may disappear or be added. */
typedef struct {int type,subtype,u,z,length,width,depth;} LegacyMember;
#include "legacy_opening_members.h"

static const char *legacy_path="opening_migration_legacy.txt";
static const char *current_path="opening_migration_current.txt";
static const char *second_path="opening_migration_second.txt";

static void write_legacy(int version,int configuration,const char *openings)
{
    FILE *f=fopen(legacy_path,"w");assert(f);
    int height=configuration && version<9 ? 3200:2400;
    fprintf(f,"sitehelper_project %d\ndomain_id_next %d\nsettings %d %d %d %d 500 %d %d %s\n",
        version,version<8?6:7,height,configuration?120:90,configuration?45:35,
        configuration?600:450,configuration?30:20,configuration?40:20,configuration?"even":"maximise");
    if(version>=8) fprintf(f,"storeys 1\nstorey 6 elevation 3000\n");
    if(version>=9) fprintf(f,"stud_height %s\n",configuration?"override 3200":"inherit");
    fprintf(f,"%s",version<3?"rooms 1\nroom 1 walls 1\n":"walls 1\n");
    fprintf(f,"wall 2 %s openings 3\n",version==1?"length 6000":
        version<4?"origin 0 0 length 6000":"segment 0 0 6000 0");
    if(openings) fputs(openings,f);
    else fprintf(f,"opening 4 door 2400 1000 820 1000 0 0 false\n"
        "opening 3 window 500 700 820 1000 12 15 %s\n"
        "opening 5 door 4400 0 820 2020 0 0 false\n",configuration?"true":"false");
    if(version<3) fputs("end_room\n",f);else fputs("rooms 0\n",f);
    if(version>=7) fputs("room_separators 0\n",f);
    if(version>=8) fputs("end_storey\n",f);
    fputs("end_project\n",f);assert(fclose(f)==0);
}

static int member_matches(const LegacyMember *e,const Timber *t)
{
    return e->type==(int)t->type && e->subtype==(t->type==TIMBER_STUD?(int)t->details.stud.type:-1) &&
        e->u==t->position.u && e->z==t->position.z && e->length==t->length && e->width==t->width && e->depth==t->depth;
}

static void assert_legacy_framing(const Wall *w,int configuration)
{
    const LegacyMember *expected=configuration?legacy_members_1:legacy_members_0;
    size_t count=configuration?sizeof legacy_members_1/sizeof *legacy_members_1:
        sizeof legacy_members_0/sizeof *legacy_members_0;
    assert(count==2+w->framing.stud_count+w->framing.nog_count+w->framing.member_count);
    for(size_t i=0;i<count;i++) {
        size_t wanted=0,actual=0;
        for(size_t j=0;j<count;j++) {
            const LegacyMember *a=&expected[i],*b=&expected[j];
            wanted+=a->type==b->type && a->subtype==b->subtype && a->u==b->u && a->z==b->z &&
                a->length==b->length && a->width==b->width && a->depth==b->depth;
        }
        actual+=member_matches(&expected[i],&w->framing.bottomplate);
        actual+=member_matches(&expected[i],&w->framing.topplate);
        for(size_t j=0;j<w->framing.stud_count;j++) actual+=member_matches(&expected[i],&w->framing.studs[j]);
        for(size_t j=0;j<w->framing.nog_count;j++) actual+=member_matches(&expected[i],&w->framing.nogs[j]);
        for(size_t j=0;j<w->framing.member_count;j++) actual+=member_matches(&expected[i],&w->framing.members[j]);
        assert(actual==wanted);
    }
}

static void assert_same_files(const char *a,const char *b)
{
    FILE *fa=fopen(a,"r"),*fb=fopen(b,"r");assert(fa && fb);
    int ca,cb;
    do {ca=fgetc(fa);cb=fgetc(fb);assert(ca==cb);} while(ca!=EOF);
    assert(!ferror(fa) && !ferror(fb));assert(fclose(fa)==0 && fclose(fb)==0);
}

static void test_all_legacy_versions_preserve_physical_framing(void)
{
    for(int version=1;version<=9;version++) for(int configuration=0;configuration<2;configuration++) {
        write_legacy(version,configuration,NULL);
        SiteHelperProject p,loaded;
        sitehelper_project_init(&p);sitehelper_project_init(&loaded);
        assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
        assert(p.storeys[0].id==6 && p.domain_ids.next==7);
        const Wall *w=sitehelper_project_find_wall_by_id_const(&p,2);assert(w);
        assert(w->definition.opening_count==3);
        assert(w->definition.openings[0].id==4 && w->definition.openings[1].id==3 && w->definition.openings[2].id==5);
        const Opening *window=&w->definition.openings[1],*door=&w->definition.openings[0];
        assert(window->frame_bottom==700+(configuration?45:35));
        assert(window->height==1000-(configuration?45:35));
        assert(window->custom_allowance==(configuration!=0));
        assert(window->width_allowance==12 && window->height_allowance==15);
        /* H-B remains positive only because of the inherited allowance.
         * Migration explicitly normalizes nominal/custom values in this case. */
        assert(door->frame_bottom==1000 && door->height==(configuration?40:20));
        assert(door->custom_allowance && door->height_allowance==0);
        assert(door->width_allowance==(configuration?30:20));
        const Opening *baseline=&w->definition.openings[2];
        assert(baseline->frame_bottom==0 && baseline->height==2020 && !baseline->custom_allowance);
        assert_legacy_framing(w,configuration);
        assert(sitehelper_project_save_file(&p,current_path)==SITEHELPER_PERSISTENCE_SUCCESS);
        FILE *f=fopen(current_path,"r");assert(f);char header[64];
        assert(fgets(header,sizeof header,f));assert(strcmp(header,"sitehelper_project 10\n")==0);assert(fclose(f)==0);
        assert(sitehelper_project_load_file(&loaded,current_path)==SITEHELPER_PERSISTENCE_SUCCESS);
        test_assert_project_authoritative_equal(&p,&loaded);
        assert_legacy_framing(sitehelper_project_find_wall_by_id_const(&loaded,2),configuration);
        assert(sitehelper_project_save_file(&loaded,second_path)==SITEHELPER_PERSISTENCE_SUCCESS);
        assert_same_files(current_path,second_path);
        sitehelper_project_destroy(&loaded);sitehelper_project_destroy(&p);
    }
}

static void test_legacy_nonzero_door_and_window_fallback(void)
{
    /* A small positive nominal window height can still have positive clear
     * geometry due to allowances. Do not fail merely because h-W <= 0. */
    write_legacy(9,0,"opening 4 door 2400 200 820 1000 0 0 false\n"
        "opening 3 window 500 700 820 20 12 30 true\n"
        "opening 5 door 4400 0 820 2020 0 0 false\n");
    SiteHelperProject p;sitehelper_project_init(&p);
    assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const Wall *w=sitehelper_project_find_wall_by_id_const(&p,2);
    const Opening *door=&w->definition.openings[0],*window=&w->definition.openings[1];
    assert(door->frame_bottom==200 && door->height==800 && !door->custom_allowance);
    assert(window->frame_bottom==735 && window->height==15 && window->height_allowance==0);
    assert(window->width_allowance==12 && window->custom_allowance);
    for(size_t i=0;i<w->framing.stud_count;i++) {
        const Timber *t=&w->framing.studs[i];
        if(t->details.stud.type==STUD_TRIMMER && t->position.u==2365) assert(t->length==1020);
    }
    sitehelper_project_destroy(&p);
}

static void test_migration_requires_resolved_storey_height(void)
{
    /* B+H=2715 exceeds Project height 2400, but fits this v9 Storey's 3200.
     * This must pass during legacy interpretation, not just regeneration. */
    write_legacy(9,1,"opening 4 door 2400 200 820 1000 0 0 false\n"
        "opening 3 window 500 1700 820 1000 12 15 true\n"
        "opening 5 door 4400 0 820 2020 0 0 false\n");
    SiteHelperProject p;sitehelper_project_init(&p);
    assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const Wall *w=sitehelper_project_find_wall_by_id_const(&p,2);
    size_t sill=0,header=0,trimmers=0,lower=0,upper=0;
    for(size_t i=0;i<w->framing.member_count;i++) {
        const Timber *t=&w->framing.members[i];
        if(t->type==TIMBER_SILL) {
            assert(t->position.u==500 && t->position.z==1700 && t->length==832);sill++;
        }
        if(t->type==TIMBER_HEADER) {
            assert(t->position.u==455 && t->position.z==2715 && t->length==922);header++;
        }
        assert(t->width==45 && t->depth==120);
    }
    for(size_t i=0;i<w->framing.stud_count;i++) {
        const Timber *t=&w->framing.studs[i];
        if(t->position.u<455 || t->position.u>1332) continue;
        if(t->details.stud.type==STUD_TRIMMER) {
            assert(t->position.z==0 && t->length==2715);trimmers++;
        }
        if(t->details.stud.type==STUD_CRIPPLE) {
            assert(t->position.u==500 || t->position.u==893 || t->position.u==1287);
            if(t->position.z==0) {assert(t->length==1700);lower++;}
            else {assert(t->position.z==2760 && t->length==440);upper++;}
        }
    }
    assert(sill==1 && header==1 && trimmers==2 && lower==3 && upper==3);
    sitehelper_project_destroy(&p);
}

static void test_v10_loads_canonical_boundaries_directly(void)
{
    write_legacy(10,0,"opening 4 door 2400 100 820 1000 0 0 false\n"
        "opening 3 window 500 35 15 2310 0 0 false\n"
        "opening 5 door 4400 0 820 2020 0 0 false\n");
    SiteHelperProject p;sitehelper_project_init(&p);
    assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const Wall *w=sitehelper_project_find_wall_by_id_const(&p,2);
    assert(w->definition.openings[0].height==1000);
    assert(w->definition.openings[1].frame_bottom==35 && w->definition.openings[1].height==2310);
    for(size_t i=0;i<w->framing.stud_count;i++) {
        const Timber *t=&w->framing.studs[i];
        assert(t->details.stud.type!=STUD_CRIPPLE);
        if(t->details.stud.type==STUD_TRIMMER && t->position.u==2365) assert(t->length==1120);
    }
    assert(sitehelper_project_save_file(&p,current_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&p,current_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_save_file(&p,second_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert_same_files(current_path,second_path);
    sitehelper_project_destroy(&p);
}

static void test_unrepresentable_legacy_definitions_fail_atomically(void)
{
    write_legacy(9,0,NULL);
    SiteHelperProject p,expected;sitehelper_project_init(&p);sitehelper_project_init(&expected);
    assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&expected,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    const char *bad[]={
        "opening 4 door 2400 1200 820 800 0 0 false\n", /* Bottom above actual trimmer top. */
        "opening 4 window 2400 700 820 10 0 0 false\n", /* Sill meets/overlaps header: no clear height. */
        "opening 4 window 2400 0 820 1000 0 0 false\n", /* No legacy lower-cripple space. */
        "opening 4 window 2400 1345 820 1000 0 0 false\n", /* Legacy rejected zero upper space. */
        "opening 4 window 2400 700 14 1000 0 0 false\n", /* Effective width below member width. */
        "opening 4 door 2400 1020 820 1000 0 0 false\n" /* Exactly zero canonical height. */
    };
    for(size_t i=0;i<sizeof bad/sizeof *bad;i++) {
        char openings[512];snprintf(openings,sizeof openings,"%sopening 3 window 500 700 820 1000 12 15 false\n"
            "opening 5 door 4400 0 820 2020 0 0 false\n",bad[i]);
        write_legacy(9,0,openings);
        Storey *storage=p.storeys;
        assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_OPENING_MIGRATION_FAILED);
        assert(p.storeys==storage);
        test_assert_project_authoritative_equal(&expected,&p);
        assert_legacy_framing(sitehelper_project_find_wall_by_id_const(&p,2),0);
    }
    sitehelper_project_destroy(&expected);sitehelper_project_destroy(&p);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_all_migration_allocation_failures(void)
{
    write_legacy(9,1,NULL);
    SiteHelperProject p;sitehelper_project_init(&p);
    assert(sitehelper_project_load_file(&p,legacy_path)==SITEHELPER_PERSISTENCE_SUCCESS);
    for(size_t i=0;;i++) {
        assert(i<1000);
        Storey *storage=p.storeys;
        allocation_failed=0;allocations_before_failure=i;
        SiteHelperPersistenceResult result=sitehelper_project_load_file(&p,legacy_path);
        allocations_before_failure=SIZE_MAX;
        if(result==SITEHELPER_PERSISTENCE_SUCCESS) {
            assert(!allocation_failed);
            printf("migration allocation failure positions checked: %zu\n",i);
            assert(i>5);break;
        }
        assert(allocation_failed && p.storeys==storage);
        assert_legacy_framing(sitehelper_project_find_wall_by_id_const(&p,2),1);
        assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);
    }
    sitehelper_project_destroy(&p);
}
#endif

int main(void)
{
    test_all_legacy_versions_preserve_physical_framing();
    test_legacy_nonzero_door_and_window_fallback();
    test_migration_requires_resolved_storey_height();
    test_v10_loads_canonical_boundaries_directly();
    test_unrepresentable_legacy_definitions_fail_atomically();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_all_migration_allocation_failures();
#endif
    assert(remove(legacy_path)==0);assert(remove(current_path)==0);assert(remove(second_path)==0);
    puts("opening persistence migration tests passed");return 0;
}
