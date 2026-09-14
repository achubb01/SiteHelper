#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "slab.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_malloc(size_t); void *__real_realloc(void *,size_t);
static int fail_now(void){return fail_after != SIZE_MAX && fail_after-- == 0;}
void *__wrap_malloc(size_t n){return fail_now()?NULL:__real_malloc(n);}
void *__wrap_realloc(void *p,size_t n){return fail_now()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition a[]={{1000,1000},{1500,1000},{1500,1500},{1000,1500}};
static const PlanPosition b[]={{3000,1000},{3500,1000},{3500,1500},{3000,1500}};
static const PlanPosition c[]={{5000,1000},{5500,1000},{5500,1500},{5000,1500}};

static Slab make_slab(void)
{
    Slab slab={0}; assert(slab_build(1,outer,4,100,0,&slab)==SLAB_SUCCESS); return slab;
}

static void assert_outline(const SlabOutline *outline,const PlanPosition *vertices)
{
    assert(outline->vertex_count==4);
    for(size_t i=0;i<4;i++){
        assert(outline->vertices[i].x==vertices[i].x&&outline->vertices[i].y==vertices[i].y);
    }
    assert(outline->vertices!=vertices);
}

static void test_penetration_order(void)
{
    Slab slab=make_slab();
    assert(slab_insert_penetration_at(&slab,0,a,4)==SLAB_SUCCESS);
    assert(slab_insert_penetration_at(&slab,1,c,4)==SLAB_SUCCESS);
    assert(slab_insert_penetration_at(&slab,1,b,4)==SLAB_SUCCESS);
    assert_outline(&slab.definition.penetrations.items[0].outline,a);
    assert_outline(&slab.definition.penetrations.items[1].outline,b);
    assert_outline(&slab.definition.penetrations.items[2].outline,c);
    assert(slab_insert_penetration_at(&slab,4,b,4)==SLAB_INVALID_ARGUMENT);
    PlanPosition outside[]={{0,0},{10,0},{10,10},{0,10}};
    Slab before={0}; assert(slab_clone(&slab,&before)==SLAB_SUCCESS);
    assert(slab_insert_penetration_at(&slab,1,outside,4)==SLAB_PENETRATION_OUTSIDE);
    test_assert_slab_equal(&before,&slab);
    slab_destroy(&before);slab_destroy(&slab);
}

static void test_region_order(void)
{
    Slab slab=make_slab();
    assert(slab_insert_region_at(&slab,0,a,4,-10,80)==SLAB_SUCCESS);
    assert(slab_insert_region_at(&slab,1,c,4,-30,60)==SLAB_SUCCESS);
    assert(slab_insert_region_at(&slab,1,b,4,-20,70)==SLAB_SUCCESS);
    assert_outline(&slab.definition.regions.items[0].outline,a);
    assert_outline(&slab.definition.regions.items[1].outline,b);
    assert(slab.definition.regions.items[1].top_level_offset_mm==-20);
    assert(slab.definition.regions.items[1].thickness_mm==70);
    assert_outline(&slab.definition.regions.items[2].outline,c);
    assert(slab_insert_region_at(&slab,4,b,4,0,100)==SLAB_INVALID_ARGUMENT);
    Slab before={0};assert(slab_clone(&slab,&before)==SLAB_SUCCESS);
    assert(slab_insert_region_at(&slab,1,a,4,0,100)==SLAB_REGION_OVERLAP);
    test_assert_slab_equal(&before,&slab);
    slab_destroy(&before);slab_destroy(&slab);
}

static void test_rebate_order(void)
{
    Slab slab=make_slab();
    assert(slab_insert_edge_rebate_at(&slab,0,0,0,1000,100,20)==SLAB_SUCCESS);
    assert(slab_insert_edge_rebate_at(&slab,1,0,4000,5000,100,40)==SLAB_SUCCESS);
    assert(slab_insert_edge_rebate_at(&slab,1,0,2000,3000,100,30)==SLAB_SUCCESS);
    assert(slab.definition.edge_rebates.items[0].depth_mm==20);
    assert(slab.definition.edge_rebates.items[1].depth_mm==30);
    assert(slab.definition.edge_rebates.items[2].depth_mm==40);
    assert(slab_insert_edge_rebate_at(&slab,4,0,6000,7000,100,20)==SLAB_INVALID_ARGUMENT);
    Slab before={0};assert(slab_clone(&slab,&before)==SLAB_SUCCESS);
    assert(slab_insert_edge_rebate_at(&slab,1,0,2500,3500,100,20)==SLAB_EDGE_REBATE_OVERLAP);
    test_assert_slab_equal(&before,&slab);
    slab_destroy(&before);slab_destroy(&slab);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    size_t penetration=0,region=0,rebate=0;
    for(size_t point=0;point<5;point++){
        fail_after=SIZE_MAX;Slab slab=make_slab(),before={0};
        assert(slab_add_penetration(&slab,a,4)==SLAB_SUCCESS);
        assert(slab_clone(&slab,&before)==SLAB_SUCCESS);
        fail_after=point;SlabCode code=slab_insert_penetration_at(&slab,0,b,4);fail_after=SIZE_MAX;
        if(code==SLAB_ALLOCATION_FAILED){penetration++;test_assert_slab_equal(&before,&slab);
            assert(slab_insert_penetration_at(&slab,0,b,4)==SLAB_SUCCESS);}
        slab_destroy(&before);slab_destroy(&slab);if(code==SLAB_SUCCESS)break;
    }
    for(size_t point=0;point<5;point++){
        fail_after=SIZE_MAX;Slab slab=make_slab(),before={0};
        assert(slab_add_region(&slab,a,4,-10,80)==SLAB_SUCCESS);
        assert(slab_clone(&slab,&before)==SLAB_SUCCESS);
        fail_after=point;SlabCode code=slab_insert_region_at(&slab,0,b,4,-20,70);fail_after=SIZE_MAX;
        if(code==SLAB_ALLOCATION_FAILED){region++;test_assert_slab_equal(&before,&slab);
            assert(slab_insert_region_at(&slab,0,b,4,-20,70)==SLAB_SUCCESS);}
        slab_destroy(&before);slab_destroy(&slab);if(code==SLAB_SUCCESS)break;
    }
    for(size_t point=0;point<3;point++){
        fail_after=SIZE_MAX;Slab slab=make_slab(),before={0};
        assert(slab_add_edge_rebate(&slab,0,0,1000,100,20)==SLAB_SUCCESS);
        assert(slab_clone(&slab,&before)==SLAB_SUCCESS);
        fail_after=point;SlabCode code=slab_insert_edge_rebate_at(&slab,0,0,2000,3000,100,30);fail_after=SIZE_MAX;
        if(code==SLAB_ALLOCATION_FAILED){rebate++;test_assert_slab_equal(&before,&slab);
            assert(slab_insert_edge_rebate_at(&slab,0,0,2000,3000,100,30)==SLAB_SUCCESS);}
        slab_destroy(&before);slab_destroy(&slab);if(code==SLAB_SUCCESS)break;
    }
    assert(penetration==2&&region==2&&rebate==1);
    printf("indexed insertion allocation failures: penetration=%zu region=%zu rebate=%zu\n",
        penetration,region,rebate);
}
#endif

int main(void)
{
    test_penetration_order();test_region_order();test_rebate_order();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    return 0;
}
