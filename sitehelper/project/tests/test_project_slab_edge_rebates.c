#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"
#include "sitehelper_persistence.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;static int failed;
void *__real_malloc(size_t n);void *__real_calloc(size_t n,size_t s);void *__real_realloc(void *p,size_t n);
static int fail(void){if(fail_after!=SIZE_MAX&&fail_after--==0){failed=1;return 1;}return 0;}
void *__wrap_malloc(size_t n){return fail()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n){return fail()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition hole[]={{2500,2250},{3500,2250},{3500,2750},{2500,2750}};
static const PlanPosition region[]={{2000,2000},{4000,2000},{4000,3000},{2000,3000}};
static const char *path="project_slab_edge_rebates_test.txt";
static void fixture(SiteHelperProject *p)
{
    sitehelper_project_init(p);assert(sitehelper_project_add_storey(p,3000)==1);
    Slab *s;assert(sitehelper_project_add_slab(p,1,outer,4,100,0)==2);s=sitehelper_project_find_slab_by_id(p,2);
    assert(slab_add_penetration(s,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(s,region,4,-50,50)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(s,0,0,2500,110,20)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(s,0,2500,5000,110,50)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(s,3,0,8000,90,15)==SLAB_SUCCESS);
    assert(p->domain_ids.next==3);
}
static void validation(const SiteHelperProject *p,SiteHelperProjectValidationCode code)
{
    SiteHelperProjectValidation v=sitehelper_project_validate(p);assert(v.code==code);
    assert(v.subject_id==(code==SITEHELPER_PROJECT_VALID?0:2));
    assert(v.related_id==(code==SITEHELPER_PROJECT_VALID?0:1));
}
static void test_project_validation(void)
{
    SiteHelperProject p;fixture(&p);Slab *s=sitehelper_project_find_slab_by_id(&p,2);
    SlabEdgeRebateCollection *c=&s->definition.edge_rebates,saved=*c;
    SlabEdgeRebateCollection bad[]={{NULL,1,1},{saved.items,4,3},{saved.items,0,0}};
    for(size_t i=0;i<3;i++){*c=bad[i];validation(&p,SITEHELPER_PROJECT_INVALID_SLAB_EDGE_REBATE_COLLECTION);*c=saved;}
    c->capacity=SIZE_MAX;validation(&p,SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW);*c=saved;
    SlabEdgeRebate original=c->items[0];
    struct {SlabEdgeRebate value;SiteHelperProjectValidationCode code;} cases[]={
        {{4,0,1,1,1},SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_EDGE},
        {{0,-1,1,1,1},SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_INTERVAL},
        {{0,0,1,0,1},SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_DIMENSIONS},
        {{0,3000,4000,1,1},SITEHELPER_PROJECT_SLAB_EDGE_REBATE_OVERLAP}
    };
    for(size_t i=0;i<4;i++){c->items[0]=cases[i].value;validation(&p,cases[i].code);c->items[0]=original;}
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    fail_after=0;failed=0;validation(&p,SITEHELPER_PROJECT_VALID);fail_after=SIZE_MAX;assert(!failed);
#endif
    sitehelper_project_destroy(&p);
}

static void full_fixture(SiteHelperProject *p)
{
    fixture(p);assert(sitehelper_project_add_slab(p,1,outer,4,150,-25)==3);
    DomainId storey=sitehelper_project_add_storey(p,6000);assert(storey==4);
    DomainId id=sitehelper_project_add_slab(p,storey,outer,4,200,-100);assert(id==5);
    assert(slab_add_edge_rebate(sitehelper_project_find_slab_by_id(p,id),2,0,10000,75,30)==SLAB_SUCCESS);
}
static void test_round_trip_copy_and_legacy(void)
{
    SiteHelperProject p,loaded,clone;full_fixture(&p);fixture(&loaded);
    test_clone_project_authoritative(&p,&clone);test_assert_project_authoritative_equal(&p,&clone);
    assert(clone.storeys[0].slabs.items[0].definition.edge_rebates.items!=
        p.storeys[0].slabs.items[0].definition.edge_rebates.items);
    assert(sitehelper_project_save_file(&p,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    test_assert_project_authoritative_equal(&p,&loaded);assert(loaded.domain_ids.next==6);
    FILE *f=fopen(path,"r");assert(f);char text[8192];size_t n=fread(text,1,sizeof text-1,f);text[n]='\0';
    assert(feof(f)&&!ferror(f)&&fclose(f)==0);assert(strstr(text,"sitehelper_project 15\n")==text);
    assert(strstr(text,"edge_rebates 0\n")&&strstr(text,"edge_rebates 1\n")&&strstr(text,"edge_rebates 3\n"));
    assert(!strstr(text,"rebate_bottom")&&!strstr(text,"rebate_volume")&&!strstr(text,"compliance"));
    sitehelper_project_destroy(&p);test_assert_project_authoritative_equal(&clone,&loaded);
    sitehelper_project_destroy(&clone);sitehelper_project_destroy(&loaded);

    const char *versions[]={
        "", "penetrations 0\n", "penetrations 0\nregions 0\n"
    };
    for(int version=11;version<=13;version++) {
        f=fopen(path,"w");assert(f);
        fprintf(f,"sitehelper_project %d\ndomain_id_next 3\nsettings 2400 90 35 600 1200 0 0 maximise\n"
            "storeys 1\nstorey 1 elevation 0\nstud_height inherit\nwalls 0\nrooms 0\nroom_separators 0\n"
            "slabs 1\nslab 2 top_level_offset 0 thickness 100 outline 4\n"
            "vertex 0 0\nvertex 10000 0\nvertex 10000 8000\nvertex 0 8000\n%s"
            "end_slab\nend_storey\nend_project\n",version,versions[version-11]);assert(fclose(f)==0);
        sitehelper_project_init(&loaded);assert(sitehelper_project_load_file(&loaded,path)==SITEHELPER_PERSISTENCE_SUCCESS);
        assert(loaded.storeys[0].slabs.items[0].definition.edge_rebates.count==0);
        assert(!loaded.storeys[0].slabs.items[0].definition.edge_rebates.items);sitehelper_project_destroy(&loaded);
    }
}

#define REBATE "edge_rebate edge 0 start_offset 0 end_offset 2500 width 110 depth 20\nend_edge_rebate\n"
static void write_input(const char *rebates)
{
    FILE *f=fopen(path,"w");assert(f);
    fputs("sitehelper_project 14\ndomain_id_next 3\nsettings 2400 90 35 600 1200 0 0 maximise\n"
        "storeys 1\nstorey 1 elevation 3000\nstud_height inherit\nwalls 0\nrooms 0\nroom_separators 0\n"
        "slabs 1\nslab 2 top_level_offset 0 thickness 100 outline 4\n"
        "vertex 0 0\nvertex 10000 0\nvertex 10000 8000\nvertex 0 8000\n"
        "penetrations 0\nregions 0\n",f);fputs(rebates,f);
    fputs("end_slab\nend_storey\nend_project\n",f);assert(fclose(f)==0);
}
static void test_malformed_transactional(void)
{
    SiteHelperProject p,before;fixture(&p);test_clone_project_authoritative(&p,&before);
    const char *bad[]={"","edge_rebates -1\n","edge_rebates nope\n","edge_rebates 18446744073709551615\n",
        "edge_rebates 1\n","edge_rebates 1\nedge_rebate edge nope\n",
        "edge_rebates 1\nedge_rebate edge 18446744073709551615 start_offset 0 end_offset 1 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 4 start_offset 0 end_offset 1 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset -1 end_offset 1 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 2147483648 end_offset 2 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 2 end_offset 1 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 1 end_offset 1 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 0 end_offset 10001 width 1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 0 end_offset 1 width 0 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 0 end_offset 1 width -1 depth 1\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 0 end_offset 1 width 1 depth 0\nend_edge_rebate\n",
        "edge_rebates 1\nedge_rebate edge 0 start_offset 0 end_offset 1 width 1 depth -1\nend_edge_rebate\n",
        "edge_rebates 2\n" REBATE REBATE,
        "edge_rebates 1\nedge_rebate edge 0 start_offset 0 end_offset 1 width 1 depth 1\n",
        "edge_rebates 1\n" REBATE "extra\n"};
    for(size_t i=0;i<sizeof bad/sizeof *bad;i++) {
        write_input(bad[i]);unsigned char bytes[sizeof p];memcpy(bytes,&p,sizeof p);
        SlabEdgeRebate *items=p.storeys[0].slabs.items[0].definition.edge_rebates.items;
        assert(sitehelper_project_load_file(&p,path)!=SITEHELPER_PERSISTENCE_SUCCESS);
        assert(memcmp(bytes,&p,sizeof p)==0&&p.storeys[0].slabs.items[0].definition.edge_rebates.items==items);
        test_assert_project_authoritative_equal(&before,&p);
    }
    printf("edge rebate malformed persistence cases: %zu; destination preserved\n",sizeof bad/sizeof *bad);
    sitehelper_project_destroy(&p);sitehelper_project_destroy(&before);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocations(void)
{
    SiteHelperProject source;full_fixture(&source);assert(sitehelper_project_save_file(&source,path)==SITEHELPER_PERSISTENCE_SUCCESS);
    size_t loads=0,copies=0;
    for(size_t i=0;i<100;i++) {
        SiteHelperProject p,before;fixture(&p);test_clone_project_authoritative(&p,&before);
        unsigned char bytes[sizeof p];memcpy(bytes,&p,sizeof p);fail_after=i;failed=0;
        SiteHelperPersistenceResult result=sitehelper_project_load_file(&p,path);fail_after=SIZE_MAX;
        if(failed){loads++;assert(result==SITEHELPER_PERSISTENCE_ALLOCATION_FAILED&&memcmp(bytes,&p,sizeof p)==0);test_assert_project_authoritative_equal(&before,&p);assert(sitehelper_project_load_file(&p,path)==SITEHELPER_PERSISTENCE_SUCCESS);}else assert(result==SITEHELPER_PERSISTENCE_SUCCESS);
        test_assert_project_authoritative_equal(&source,&p);sitehelper_project_destroy(&p);sitehelper_project_destroy(&before);if(!failed)break;
    }
    Slab *src=&source.storeys[0].slabs.items[0];src->id=100;
    for(size_t i=0;i<20;i++) {
        SiteHelperProject p,before;fixture(&p);test_clone_project_authoritative(&p,&before);
        unsigned char bytes[sizeof p];memcpy(bytes,&p,sizeof p);fail_after=i;failed=0;
        int result=sitehelper_project_insert_slab(&p,1,src);fail_after=SIZE_MAX;
        if(failed){copies++;assert(!result&&memcmp(bytes,&p,sizeof p)==0);test_assert_project_authoritative_equal(&before,&p);assert(sitehelper_project_insert_slab(&p,1,src));}else assert(result);
        test_assert_slab_equal(src,&p.storeys[0].slabs.items[1]);sitehelper_project_destroy(&p);sitehelper_project_destroy(&before);if(!failed)break;
    }
    assert(loads>0&&copies>0);printf("edge rebate project allocation sweep: %zu load, %zu restore failures; retries passed\n",loads,copies);
    sitehelper_project_destroy(&source);
}
#endif

int main(void)
{
    test_project_validation();test_round_trip_copy_and_legacy();test_malformed_transactional();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocations();
#endif
    assert(remove(path)==0);puts("project slab edge rebate tests passed");return 0;
}
