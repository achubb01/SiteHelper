#include <assert.h>
#include <stdlib.h>

#include "roof_plan_query.h"
#include "roof.h"

static Roof make_roof(DomainId roof_id, DomainId portion_id,
    PlanPosition *vertices)
{
    Roof roof={.id=roof_id};
    RoofPortionSpec spec={vertices,4,ROOF_PORTION_OPPOSING_SLOPES,414214,0,{1,0},
        ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
    assert(roof_definition_append_portion(&roof.definition,portion_id,&spec)==ROOF_SUCCESS);
    assert(roof_validate(&roof)==ROOF_SUCCESS);
    return roof;
}

int main(void)
{
    Storey s={0};
    s.id=1;
    PlanPosition a[4]={{0,0},{10000,0},{10000,8000},{0,8000}};
    PlanPosition b[4]={{4000,2000},{12000,2000},{12000,6000},{4000,6000}};
    s.roofs.items=calloc(2,sizeof *s.roofs.items);
    assert(s.roofs.items != NULL);
    s.roofs.capacity=s.roofs.count=2;
    s.roofs.items[0]=make_roof(10,11,a);
    s.roofs.items[1]=make_roof(20,21,b);

    RoofPlanHit hit=roof_plan_hit_test_storey(&s,(PlanPoint){5000,3000},0.0);
    assert(hit.kind==ROOF_PLAN_HIT_PORTION && hit.roof_id==20 && hit.portion_id==21);
    hit=roof_plan_hit_test_storey(&s,(PlanPoint){1000,1000},0.0);
    assert(hit.roof_id==10 && hit.portion_id==11);
    hit=roof_plan_hit_test_storey(&s,(PlanPoint){-25,4000},30.0);
    assert(hit.roof_id==10 && hit.portion_id==11);
    hit=roof_plan_hit_test_storey(&s,(PlanPoint){-100,4000},30.0);
    assert(hit.kind==ROOF_PLAN_HIT_NONE);

    roof_collection_destroy(&s.roofs);
    return 0;
}
