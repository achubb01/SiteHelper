#include <assert.h>
#include "slab.h"

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition region0[]={{0,0},{3000,0},{3000,2000},{0,2000}};

int main(void)
{
    Slab slab={0};
    assert(slab_build(1,outer,4,100,0,&slab)==SLAB_SUCCESS);
    assert(slab_add_region(&slab,region0,4,-25,75)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,0,0,1000,100,20)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,0,2000,3000,120,30)==SLAB_SUCCESS);

    assert(slab_set_base_properties(&slab,125,-10)==SLAB_SUCCESS);
    assert(slab.definition.thickness_mm==125&&slab.definition.top_level_offset_mm==-10);
    assert(slab_set_base_properties(&slab,0,500)==SLAB_INVALID_THICKNESS);
    assert(slab.definition.thickness_mm==125&&slab.definition.top_level_offset_mm==-10);

    assert(slab_set_region_properties(&slab,0,-50,90)==SLAB_SUCCESS);
    assert(slab.definition.regions.items[0].top_level_offset_mm==-50&&
        slab.definition.regions.items[0].thickness_mm==90);
    assert(slab_set_region_properties(&slab,0,200,0)==SLAB_INVALID_REGION_THICKNESS);
    assert(slab.definition.regions.items[0].top_level_offset_mm==-50&&
        slab.definition.regions.items[0].thickness_mm==90);
    assert(slab_set_region_properties(&slab,99,0,100)==SLAB_INVALID_ARGUMENT);

    SlabEdgeRebate before=slab.definition.edge_rebates.items[1];
    assert(slab_set_edge_rebate_properties(&slab,1,900,2500,150,40)==SLAB_EDGE_REBATE_OVERLAP);
    SlabEdgeRebate after=slab.definition.edge_rebates.items[1];
    assert(after.edge_index==before.edge_index&&after.start_offset_mm==before.start_offset_mm&&
        after.end_offset_mm==before.end_offset_mm&&after.width_mm==before.width_mm&&after.depth_mm==before.depth_mm);
    assert(slab_set_edge_rebate_properties(&slab,1,1500,3000,150,40)==SLAB_SUCCESS);
    assert(slab.definition.edge_rebates.items[1].start_offset_mm==1500&&
        slab.definition.edge_rebates.items[1].end_offset_mm==3000&&
        slab.definition.edge_rebates.items[1].width_mm==150&&
        slab.definition.edge_rebates.items[1].depth_mm==40);
    assert(slab_set_edge_rebate_properties(&slab,1,1500,3000,0,40)==SLAB_EDGE_REBATE_INVALID_DIMENSIONS);
    assert(slab.definition.edge_rebates.items[1].width_mm==150);
    assert(slab_set_edge_rebate_properties(&slab,99,0,1,1,1)==SLAB_INVALID_ARGUMENT);

    assert(slab_validate(&slab)==SLAB_SUCCESS);
    slab_destroy(&slab);
    return 0;
}
