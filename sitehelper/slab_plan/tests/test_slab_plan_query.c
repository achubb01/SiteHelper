#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "slab_plan_query.h"

static int close_to(double a, double b)
{
    return fabs(a - b) < 1e-9;
}

static Slab make_slab(DomainId id, const PlanPosition *vertices, size_t count)
{
    Slab slab = {0};
    assert(slab_build(id, vertices, count, 100, 0, &slab) == SLAB_SUCCESS);
    return slab;
}

static void test_rebate_endpoint_conversion(void)
{
    const PlanPosition rectangle[] = {{0,0},{300,0},{300,400},{0,400}};
    Slab slab = make_slab(1, rectangle, 4);
    assert(slab_add_edge_rebate(&slab,0,0,300,100,20)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,1,100,300,100,20)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,3,100,400,100,20)==SLAB_SUCCESS);
    PlanPoint a = {-1,-1}, b = {-1,-1};
    assert(slab_plan_rebate_endpoints(&slab.definition,
        &slab.definition.edge_rebates.items[0],&a,&b));
    assert(close_to(a.x,0)&&close_to(a.y,0)&&close_to(b.x,300)&&close_to(b.y,0));
    assert(slab_plan_rebate_endpoints(&slab.definition,
        &slab.definition.edge_rebates.items[1],&a,&b));
    assert(close_to(a.x,300)&&close_to(a.y,100)&&close_to(b.x,300)&&close_to(b.y,300));
    assert(slab_plan_rebate_endpoints(&slab.definition,
        &slab.definition.edge_rebates.items[2],&a,&b));
    assert(close_to(a.x,0)&&close_to(a.y,300)&&close_to(b.x,0)&&close_to(b.y,0));
    slab_destroy(&slab);

    const PlanPosition diagonal[] = {{0,0},{300,400},{600,0}};
    slab = make_slab(2,diagonal,3);
    assert(slab_add_edge_rebate(&slab,0,100,500,50,15)==SLAB_SUCCESS);
    assert(slab_plan_rebate_endpoints(&slab.definition,
        &slab.definition.edge_rebates.items[0],&a,&b));
    assert(close_to(a.x,60)&&close_to(a.y,80)&&close_to(b.x,300)&&close_to(b.y,400));
    slab_destroy(&slab);

    const PlanPosition reverse[] = {{300,400},{0,0},{600,0}};
    slab = make_slab(3,reverse,3);
    assert(slab_add_edge_rebate(&slab,0,100,500,50,15)==SLAB_SUCCESS);
    assert(slab_plan_rebate_endpoints(&slab.definition,
        &slab.definition.edge_rebates.items[0],&a,&b));
    assert(close_to(a.x,240)&&close_to(a.y,320)&&close_to(b.x,0)&&close_to(b.y,0));
    PlanPoint saved_a=a,saved_b=b;
    SlabEdgeRebate invalid=slab.definition.edge_rebates.items[0];
    invalid.edge_index=99;
    assert(!slab_plan_rebate_endpoints(&slab.definition,&invalid,&a,&b));
    assert(a.x==saved_a.x&&a.y==saved_a.y&&b.x==saved_b.x&&b.y==saved_b.y);
    slab_destroy(&slab);
}

static void append_slab(Storey *storey, Slab *slab)
{
    assert(slab_collection_append(&storey->slabs,slab)==SLAB_SUCCESS);
}

static void test_hit_precedence_and_polygon_semantics(void)
{
    const PlanPosition outer[]={{0,0},{1000,0},{1000,1000},{0,1000}};
    const PlanPosition region[]={{100,100},{900,100},{900,900},{100,900}};
    const PlanPosition hole[]={{400,400},{600,400},{600,600},{400,600}};
    Slab slab=make_slab(10,outer,4);
    assert(slab_add_penetration(&slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(&slab,region,4,-50,80)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,0,0,300,100,20)==SLAB_SUCCESS);
    Storey storey={.id=1}; append_slab(&storey,&slab);

    SlabPlanHit hit=slab_plan_hit_test_storey(&storey,(PlanPoint){50,5},10);
    assert(hit.kind==SLAB_PLAN_HIT_EDGE_REBATE&&hit.slab_id==10&&hit.feature_index==0);
    hit=slab_plan_hit_test_storey(&storey,(PlanPoint){500,500},0);
    assert(hit.kind==SLAB_PLAN_HIT_PENETRATION&&hit.feature_index==0);
    hit=slab_plan_hit_test_storey(&storey,(PlanPoint){200,200},0);
    assert(hit.kind==SLAB_PLAN_HIT_REGION&&hit.feature_index==0);
    hit=slab_plan_hit_test_storey(&storey,(PlanPoint){950,950},0);
    assert(hit.kind==SLAB_PLAN_HIT_SLAB&&hit.feature_index==SIZE_MAX);
    hit=slab_plan_hit_test_storey(&storey,(PlanPoint){1000,950},0);
    assert(hit.kind==SLAB_PLAN_HIT_SLAB);
    hit=slab_plan_hit_test_storey(&storey,(PlanPoint){1200,500},0);
    assert(hit.kind==SLAB_PLAN_HIT_NONE&&hit.slab_id==DOMAIN_ID_INVALID);
    slab_collection_destroy(&storey.slabs);

    const PlanPosition concave[]={{0,0},{1000,0},{1000,300},{300,300},
        {300,1000},{0,1000}};
    slab=make_slab(11,concave,6); append_slab(&storey,&slab);
    assert(slab_plan_hit_test_storey(&storey,(PlanPoint){100,800},0).kind==SLAB_PLAN_HIT_SLAB);
    assert(slab_plan_hit_test_storey(&storey,(PlanPoint){800,800},0).kind==SLAB_PLAN_HIT_NONE);
    slab_collection_destroy(&storey.slabs);

    const PlanPosition clockwise[]={{0,0},{0,500},{500,500},{500,0}};
    slab=make_slab(12,clockwise,4); append_slab(&storey,&slab);
    assert(slab_plan_hit_test_storey(&storey,(PlanPoint){250,250},0).slab_id==12);
    slab_collection_destroy(&storey.slabs);
}

static void test_multiple_slab_order_and_invalid_input(void)
{
    const PlanPosition outer[]={{0,0},{500,0},{500,500},{0,500}};
    Storey storey={.id=1};
    Slab first=make_slab(20,outer,4), second=make_slab(21,outer,4);
    append_slab(&storey,&first); append_slab(&storey,&second);
    SlabPlanHit hit=slab_plan_hit_test_storey(&storey,(PlanPoint){250,250},0);
    assert(hit.kind==SLAB_PLAN_HIT_SLAB&&hit.slab_id==21);
    storey.slabs.items[1].definition.thickness_mm=0;
    hit=slab_plan_hit_test_storey(&storey,(PlanPoint){250,250},0);
    assert(hit.slab_id==20); /* Malformed later slab is safely ignored. */
    assert(slab_plan_hit_test_storey(&storey,(PlanPoint){NAN,0},0).kind==SLAB_PLAN_HIT_NONE);
    assert(slab_plan_hit_test_storey(&storey,(PlanPoint){0,0},-1).kind==SLAB_PLAN_HIT_NONE);
    storey.slabs.items[1].definition.thickness_mm=100;
    slab_collection_destroy(&storey.slabs);
}

int main(void)
{
    test_rebate_endpoint_conversion();
    test_hit_precedence_and_polygon_semantics();
    test_multiple_slab_order_and_invalid_input();
    puts("All slab plan query tests passed.");
    return 0;
}
