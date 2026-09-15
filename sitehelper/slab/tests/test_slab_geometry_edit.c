#include <assert.h>

#include "slab.h"

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition hole[]={{1000,1000},{2500,1000},{2500,2500},{1000,2500}};
static const PlanPosition region[]={{5000,1000},{7000,1000},{7000,3000},{5000,3000}};

static Slab make_slab(void)
{
    Slab slab={0};
    assert(slab_build(1,outer,4,100,0,&slab)==SLAB_SUCCESS);
    assert(slab_add_penetration(&slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(&slab,region,4,-20,80)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,0,0,7000,100,20)==SLAB_SUCCESS);
    return slab;
}

static void test_outer_vertex_edit(void)
{
    Slab slab=make_slab();
    assert(slab_set_outline_vertex(&slab,1,(PlanPosition){11000,0})==SLAB_SUCCESS);
    assert(slab.definition.outline.vertices[1].x==11000);
    const SlabEdgeRebate *rebate=slab_edge_rebate_at(&slab,0);
    assert(rebate!=NULL&&rebate->edge_index==0&&rebate->start_offset_mm==0&&
        rebate->end_offset_mm==7000);

    PlanPosition before=slab.definition.outline.vertices[1];
    assert(slab_set_outline_vertex(&slab,1,(PlanPosition){6000,0})!=SLAB_SUCCESS);
    assert(slab.definition.outline.vertices[1].x==before.x&&
        slab.definition.outline.vertices[1].y==before.y);
    rebate=slab_edge_rebate_at(&slab,0);
    assert(rebate!=NULL&&rebate->edge_index==0&&rebate->start_offset_mm==0&&
        rebate->end_offset_mm==7000);
    assert(slab_set_outline_vertex(&slab,99,(PlanPosition){1,1})==SLAB_INVALID_ARGUMENT);
    slab_destroy(&slab);
}

static void test_penetration_vertex_edit(void)
{
    Slab slab=make_slab();
    assert(slab_set_penetration_vertex(&slab,0,1,(PlanPosition){3000,1000})==SLAB_SUCCESS);
    assert(slab.definition.penetrations.items[0].outline.vertices[1].x==3000);
    PlanPosition before=slab.definition.penetrations.items[0].outline.vertices[0];
    assert(slab_set_penetration_vertex(&slab,0,0,(PlanPosition){0,1000})==
        SLAB_PENETRATION_OUTSIDE);
    assert(slab.definition.penetrations.items[0].outline.vertices[0].x==before.x);
    assert(slab_set_penetration_vertex(&slab,0,0,
        slab.definition.penetrations.items[0].outline.vertices[1])!=SLAB_SUCCESS);
    assert(slab.definition.penetrations.items[0].outline.vertices[0].x==before.x);
    assert(slab_set_penetration_vertex(&slab,9,0,(PlanPosition){1,1})==SLAB_INVALID_ARGUMENT);
    slab_destroy(&slab);
}

static void test_region_vertex_edit(void)
{
    Slab slab=make_slab();
    assert(slab_set_region_vertex(&slab,0,2,(PlanPosition){7500,3000})==SLAB_SUCCESS);
    assert(slab.definition.regions.items[0].outline.vertices[2].x==7500);
    PlanPosition before=slab.definition.regions.items[0].outline.vertices[2];
    assert(slab_set_region_vertex(&slab,0,2,(PlanPosition){12000,3000})==SLAB_REGION_OUTSIDE);
    assert(slab.definition.regions.items[0].outline.vertices[2].x==before.x);
    assert(slab_set_region_vertex(&slab,0,99,(PlanPosition){1,1})==SLAB_INVALID_ARGUMENT);
    slab_destroy(&slab);
}

int main(void)
{
    test_outer_vertex_edit();
    test_penetration_vertex_edit();
    test_region_vertex_edit();
    return 0;
}
