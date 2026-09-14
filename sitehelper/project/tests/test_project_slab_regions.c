#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"
#include "sitehelper_persistence.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
static int failed;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n,size_t s);
void *__real_realloc(void *p,size_t n);
static int fail(void) { if(fail_after!=SIZE_MAX && fail_after--==0) {failed=1;return 1;} return 0; }
void *__wrap_malloc(size_t n) {return fail()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s) {return fail()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n) {return fail()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition region[]={{2000,2000},{4000,2000},{4000,3000},{2000,3000}};
static const PlanPosition hole[]={{2500,2250},{3500,2250},{3500,2750},{2500,2750}};
static const PlanPosition second[]={{5000,2000},{6000,2000},{6000,3000},{5000,3000}};
static const char *path="project_slab_regions_test.txt";
static void fixture(SiteHelperProject *p)
{
    sitehelper_project_init(p); assert(sitehelper_project_add_storey(p,3000)==1);
    assert(sitehelper_project_add_slab(p,1,outer,4,100,-50)==2);
    assert(slab_add_penetration(sitehelper_project_find_slab_by_id(p,2),hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(sitehelper_project_find_slab_by_id(p,2),region,4,-50,50)==SLAB_SUCCESS);
    assert(p->domain_ids.next==3); /* Subordinate geometry consumes no IDs. */
}
static void check(const SiteHelperProject *p, SiteHelperProjectValidationCode code)
{
    SiteHelperProjectValidation v=sitehelper_project_validate(p);
    assert(v.code==code);
    assert(v.subject_id==(code==SITEHELPER_PROJECT_VALID?0:2));
    assert(v.related_id==(code==SITEHELPER_PROJECT_VALID?0:1));
}
static void test_validation(void)
{
    SiteHelperProject p; fixture(&p);
    Slab *s=sitehelper_project_find_slab_by_id(&p,2);
    SlabRegionCollection *c=&s->definition.regions, saved=*c;
    SlabRegionCollection bad[]={{NULL,1,1},{saved.items,2,1},{saved.items,0,0}};
    for(size_t i=0;i<3;i++) {
        *c=bad[i]; check(&p,SITEHELPER_PROJECT_INVALID_SLAB_REGION_COLLECTION); *c=saved;
    }
    c->capacity=SIZE_MAX; check(&p,SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW); *c=saved;
    SlabOutline outline=c->items[0].outline;
    SlabOutline bad_outline[]={{NULL,4,4},{outline.vertices,5,4},{outline.vertices,0,0}};
    for(size_t i=0;i<3;i++) {
        c->items[0].outline=bad_outline[i]; check(&p,SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE_COLLECTION);
    }
    c->items[0].outline=outline; c->items[0].outline.vertex_capacity=SIZE_MAX;
    check(&p,SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW); c->items[0].outline=outline;
    c->items[0].outline.vertex_count=2; check(&p,SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE);
    c->items[0].outline=outline;
    c->items[0].thickness_mm=0; check(&p,SITEHELPER_PROJECT_INVALID_SLAB_REGION_THICKNESS);
    c->items[0].thickness_mm=50;
    const PlanPosition invalid[][4]={
        {{1,1},{4,4},{1,4},{4,1}},
        {{-5,1},{-1,1},{-1,5},{-5,5}},
        {{3000,2000},{4000,2000},{4000,3000},{3000,3000}},
        {{INT_MIN,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MAX},{INT_MIN,INT_MAX}}
    };
    const SiteHelperProjectValidationCode codes[]={SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE,
        SITEHELPER_PROJECT_SLAB_REGION_OUTSIDE,SITEHELPER_PROJECT_SLAB_REGION_PENETRATION_INTERSECTION,
        SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW};
    for(size_t i=0;i<4;i++) { memcpy(outline.vertices,invalid[i],sizeof invalid[i]); check(&p,codes[i]); }
    /* Independently constructed invalid state must fail full validation too. */
    const PlanPosition empty_regions[][4]={
        {{2600,2300},{2700,2300},{2700,2400},{2600,2400}},
        {{2500,2250},{3500,2250},{3500,2750},{2500,2750}}
    };
    for(size_t i=0;i<2;i++) {
        memcpy(outline.vertices,empty_regions[i],sizeof empty_regions[i]);
        assert(slab_validate(s)==SLAB_REGION_PENETRATION_INTERSECTION);
        check(&p,SITEHELPER_PROJECT_SLAB_REGION_PENETRATION_INTERSECTION);
        SlabRegionQuantities q={1,2,3,4,5};unsigned char bytes[sizeof q];memcpy(bytes,&q,sizeof q);
        assert(slab_region_measure(&s->definition,0,&q)==SLAB_REGION_PENETRATION_INTERSECTION);
        assert(memcmp(bytes,&q,sizeof q)==0);
    }
    memcpy(outline.vertices,region,sizeof region);
    assert(slab_add_region(s,second,4,25,150)==SLAB_SUCCESS); c=&s->definition.regions;
    const PlanPosition interactions[][4]={
        {{3500,2500},{4500,2500},{4500,3500},{3500,3500}},
        {{2100,2100},{2200,2100},{2200,2200},{2100,2200}}
    };
    for(size_t i=0;i<2;i++) {
        memcpy(c->items[1].outline.vertices,interactions[i],sizeof interactions[i]);
        check(&p,SITEHELPER_PROJECT_SLAB_REGION_OVERLAP);
    }
    memcpy(c->items[1].outline.vertices,second,sizeof second); check(&p,SITEHELPER_PROJECT_VALID);
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    fail_after=0; failed=0; check(&p,SITEHELPER_PROJECT_VALID); fail_after=SIZE_MAX;
    assert(!failed);
#endif
    sitehelper_project_destroy(&p);
}

static void full_fixture(SiteHelperProject *p)
{
    fixture(p);
    assert(slab_add_region(sitehelper_project_find_slab_by_id(p,2),second,4,25,150)==SLAB_SUCCESS);
    assert(sitehelper_project_add_slab(p,1,outer,4,150,-25)); /* Zero regions and penetrations. */
    DomainId level=sitehelper_project_add_storey(p,6000);
    DomainId id=sitehelper_project_add_slab(p,level,outer,4,200,-100);
    assert(id);
    const PlanPosition triangle[]={{1,1},{4,1},{1,2}};
    assert(slab_add_region(sitehelper_project_find_slab_by_id(p,id),triangle,3,-25,75)==SLAB_SUCCESS);
}
static void test_round_trip_and_copy(void)
{
    SiteHelperProject p,loaded,clone; full_fixture(&p); fixture(&loaded);
    test_clone_project_authoritative(&p,&clone); test_assert_project_authoritative_equal(&p,&clone);
    assert(clone.storeys[0].slabs.items[0].definition.regions.items!=p.storeys[0].slabs.items[0].definition.regions.items);
    assert(sitehelper_project_save_file(&p,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&p,&loaded);
    assert(sitehelper_project_find_owning_storey(&loaded,5)->id==4);
    assert(loaded.domain_ids.next==6);
    assert(sitehelper_project_add_room(&loaded,1)==6);
    FILE *f=fopen(path,"r"); assert(f); char text[4096];
    size_t n=fread(text,1,sizeof text-1,f); text[n]='\0'; assert(feof(f) && !ferror(f) && fclose(f)==0);
    assert(strstr(text,"sitehelper_project 14\n")==text);
    assert(strstr(text,"regions 0\n") && strstr(text,"regions 1\n") && strstr(text,"regions 2\n"));
    assert(!strstr(text,"area") && !strstr(text,"volume") && !strstr(text,"perimeter"));
    PlanPosition *vertices=p.storeys[0].slabs.items[0].definition.regions.items[0].outline.vertices;
    vertices[0].x=-1;
    assert(sitehelper_project_save_file(&p,path)==SITEHELPER_PERSISTENCE_INVALID_PROJECT);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&clone,&loaded);
    sitehelper_project_destroy(&p);
    test_assert_project_authoritative_equal(&clone,&loaded);
    sitehelper_project_destroy(&clone); sitehelper_project_destroy(&loaded);
}

static void write_input(int version, const char *sections)
{
    FILE *f=fopen(path,"w"); assert(f);
    fprintf(f,"sitehelper_project %d\ndomain_id_next 3\nsettings 2400 90 35 600 1200 0 0 maximise\n"
        "storeys 1\nstorey 1 elevation 3000\nstud_height inherit\nwalls 0\nrooms 0\nroom_separators 0\n"
        "slabs 1\nslab 2 top_level_offset -50 thickness 100 outline 4\n"
        "vertex 0 0\nvertex 10000 0\nvertex 10000 8000\nvertex 0 8000\n",version);
    fputs(sections,f); fputs("end_slab\nend_storey\nend_project\n",f); assert(fclose(f)==0);
}
#define HOLE "penetration outline 4\nvertex 2500 2250\nvertex 3500 2250\nvertex 3500 2750\nvertex 2500 2750\nend_penetration\n"
#define REGION "region top_level_offset -50 thickness 50 outline 4\nvertex 2000 2000\nvertex 4000 2000\nvertex 4000 3000\nvertex 2000 3000\nend_region\n"
static void test_legacy_and_malformed(void)
{
    SiteHelperProject p,before; fixture(&p); test_clone_project_authoritative(&p,&before);
    for(int version=11;version<=12;version++) {
        write_input(version,version==11?"":"penetrations 1\n" HOLE);
        assert(sitehelper_project_load_file(&p,path)==SITEHELPER_PERSISTENCE_SUCCESS);
        Slab *s=&p.storeys[0].slabs.items[0];
        assert(s->id==2 && p.domain_ids.next==3);
        assert(s->definition.regions.count==0 && !s->definition.regions.items);
        assert(s->definition.penetrations.count==(size_t)(version-11));
        if(version==11) {assert(slab_add_penetration(s,hole,4)==SLAB_SUCCESS);}
        assert(slab_add_region(s,region,4,-50,50)==SLAB_SUCCESS);
        test_assert_project_authoritative_equal(&before,&p);
    }
    const char *bad[]={
        "", "regions -1\n", "regions nope\n", "regions 18446744073709551615\n",
        "regions 1\n", "regions 1\nregion top_level_offset nope\n",
        "regions 1\nregion top_level_offset 2147483648 thickness 50 outline 4\n",
        "regions 1\nregion top_level_offset 0 thickness 0 outline 4\n",
        "regions 1\nregion top_level_offset 0 thickness -1 outline 4\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 2\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline -3\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 18446744073709551615\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 3\nvertex 1 nope\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 3\nvertex 2147483648 1\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 3\nvertex 1 1\nvertex 4 1\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 3\nvertex 1 1\nvertex 4 1\nvertex 1 2\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 4\nvertex 1 1\nvertex 4 4\nvertex 1 4\nvertex 4 1\nend_region\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 4\nvertex -5 1\nvertex -1 1\nvertex -1 5\nvertex -5 5\nend_region\n",
        "regions 2\n" REGION REGION,
        "regions 2\n" REGION "region top_level_offset 0 thickness 50 outline 4\nvertex 2100 2100\nvertex 2200 2100\nvertex 2200 2200\nvertex 2100 2200\nend_region\n",
        "regions 2\n" REGION "region top_level_offset 0 thickness 50 outline 4\nvertex 3500 2500\nvertex 4500 2500\nvertex 4500 3500\nvertex 3500 3500\nend_region\n",
        "regions 1\nregion top_level_offset 0 thickness 50 outline 4\nvertex 3000 2000\nvertex 4000 2000\nvertex 4000 3000\nvertex 3000 3000\nend_region\n",
        "regions 1\n" REGION "extra\n",
        "regions 1\nregion top_level_offset -75 thickness 25 outline 4\nvertex 2600 2300\nvertex 2700 2300\nvertex 2700 2400\nvertex 2600 2400\nend_region\n",
        "regions 1\nregion top_level_offset -75 thickness 25 outline 4\nvertex 2500 2250\nvertex 3500 2250\nvertex 3500 2750\nvertex 2500 2750\nend_region\n"
    };
    for(size_t i=0;i<sizeof bad/sizeof *bad;i++) {
        char input[2048]; int n=snprintf(input,sizeof input,"penetrations 1\n%s%s",HOLE,bad[i]);
        assert(n>0 && (size_t)n<sizeof input); write_input(13,input);
        unsigned char bytes[sizeof p]; memcpy(bytes,&p,sizeof p);
        SlabRegion *items=p.storeys[0].slabs.items[0].definition.regions.items;
        PlanPosition *vertices=items[0].outline.vertices;
        assert(sitehelper_project_load_file(&p,path)!=SITEHELPER_PERSISTENCE_SUCCESS);
        assert(memcmp(bytes,&p,sizeof p)==0);
        assert(p.storeys[0].slabs.items[0].definition.regions.items==items && items[0].outline.vertices==vertices);
        test_assert_project_authoritative_equal(&before,&p);
    }
    printf("region malformed persistence cases: %zu; destination preserved\n",sizeof bad/sizeof *bad);
    sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocations(void)
{
    SiteHelperProject source; full_fixture(&source);
    assert(sitehelper_project_save_file(&source,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    size_t load_failures=0,copy_failures=0;
    for(size_t index=0;index<100;index++) {
        SiteHelperProject p,before; fixture(&p); test_clone_project_authoritative(&p,&before);
        unsigned char bytes[sizeof p]; memcpy(bytes,&p,sizeof p);
        SlabRegion *items=p.storeys[0].slabs.items[0].definition.regions.items;
        PlanPosition *vertices=items[0].outline.vertices;
        fail_after=index; failed=0;
        SiteHelperPersistenceResult result=sitehelper_project_load_file(&p,path);
        fail_after=SIZE_MAX;
        if(failed) {
            load_failures++; assert(result==SITEHELPER_PERSISTENCE_ALLOCATION_FAILED);
            assert(memcmp(bytes,&p,sizeof p)==0);
            assert(p.storeys[0].slabs.items[0].definition.regions.items==items && items[0].outline.vertices==vertices);
            test_assert_project_authoritative_equal(&before,&p);
            assert(sitehelper_project_load_file(&p,path)==SITEHELPER_PERSISTENCE_SUCCESS);
        } else { assert(result==SITEHELPER_PERSISTENCE_SUCCESS); }
        test_assert_project_authoritative_equal(&source,&p);
        sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
        if(!failed) { break; }
    }
    Slab *copy_source=&source.storeys[0].slabs.items[0]; copy_source->id=100;
    for(size_t index=0;index<20;index++) {
        SiteHelperProject p,before; fixture(&p); test_clone_project_authoritative(&p,&before);
        unsigned char bytes[sizeof p]; memcpy(bytes,&p,sizeof p);
        Slab *items=p.storeys[0].slabs.items;
        fail_after=index; failed=0;
        int result=sitehelper_project_insert_slab(&p,1,copy_source);
        fail_after=SIZE_MAX;
        if(failed) {
            copy_failures++; assert(!result); assert(memcmp(bytes,&p,sizeof p)==0);
            assert(p.storeys[0].slabs.items==items); test_assert_project_authoritative_equal(&before,&p);
            assert(sitehelper_project_insert_slab(&p,1,copy_source));
        } else { assert(result); }
        test_assert_slab_equal(copy_source,&p.storeys[0].slabs.items[1]);
        assert(p.domain_ids.next==3); /* Restoration preserves the caller-owned watermark. */
        p.domain_ids.next=101;
        assert(sitehelper_project_validate(&p).code==SITEHELPER_PROJECT_VALID);
        sitehelper_project_destroy(&p); sitehelper_project_destroy(&before);
        if(!failed) { break; }
    }
    assert(load_failures>20 && load_failures<100 && copy_failures==7);
    printf("region project allocation sweep: %zu load, %zu restore failures; retries passed\n",load_failures,copy_failures);
    sitehelper_project_destroy(&source);
}
#endif
int main(void)
{
    test_validation(); test_round_trip_and_copy(); test_legacy_and_malformed();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocations();
#endif
    assert(remove(path)==0);
    puts("project slab region tests passed"); return 0;
}
