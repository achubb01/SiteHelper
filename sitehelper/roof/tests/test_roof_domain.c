#include <assert.h>
#include <stdio.h>

#include "roof.h"

static size_t count_edges(const RoofPrototypeGeometry *g, RoofPrototypeInteriorEdgeKind kind)
{
    size_t count = 0;
    for (size_t i = 0; i < g->interior_edge_count; i++) if (g->interior_edges[i].kind == kind) count++;
    return count;
}

static RoofPortionSpec spec(const PlanPosition *support, RoofPortionGeneration generation,
    int64_t slope, int z, RoofDirection direction, RoofSingleSlopeReference reference)
{
    return (RoofPortionSpec){support,4,generation,slope,z,direction,reference};
}

static void init_roof(Roof *roof, DomainId roof_id, DomainId portion_id, const RoofPortionSpec *portion)
{
    *roof = (Roof){.id = roof_id};
    assert(roof_definition_append_portion(&roof->definition, portion_id, portion) == ROOF_SUCCESS);
}

static void test_simple_families(void)
{
    static const PlanPosition a[]={{0,0},{12000,0},{12000,8000},{0,8000}};
    static const PlanPosition e[]={{0,0},{8000,0},{8000,6000},{0,6000}};
    Roof roof={0}; RoofPrototypeGeometry g={0};
    RoofPortionSpec p=spec(a,ROOF_PORTION_OPPOSING_SLOPES,414214,0,(RoofDirection){1,0},0);
    init_roof(&roof,1,2,&p); assert(roof_validate(&roof)==ROOF_SUCCESS);
    assert(roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS && g.plane_count==2 && count_edges(&g,ROOF_PROTOTYPE_INTERIOR_RIDGE)==1);
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);

    p=spec(a,ROOF_PORTION_ALL_BOUNDARY_SLOPES,414214,0,(RoofDirection){0,0},0);
    init_roof(&roof,3,4,&p); assert(roof_validate(&roof)==ROOF_SUCCESS);
    assert(roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS && g.plane_count==4 && count_edges(&g,ROOF_PROTOTYPE_INTERIOR_HIP)==4);
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);

    p=spec(e,ROOF_PORTION_SINGLE_SLOPE,131652,0,(RoofDirection){0,-1},ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE);
    init_roof(&roof,5,6,&p); assert(roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS && g.plane_count==1);
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);
}

static void test_compound_valley_and_ids(void)
{
    static const PlanPosition main_support[]={{0,0},{12000,0},{12000,8000},{0,8000}};
    static const PlanPosition wing_support[]={{4000,4000},{8000,4000},{8000,11000},{4000,11000}};
    Roof roof={0}; RoofPrototypeGeometry g={0};
    RoofPortionSpec main=spec(main_support,ROOF_PORTION_OPPOSING_SLOPES,414214,0,(RoofDirection){1,0},0);
    RoofPortionSpec wing=spec(wing_support,ROOF_PORTION_OPPOSING_SLOPES,577350,0,(RoofDirection){0,1},0);
    init_roof(&roof,10,11,&main);
    assert(roof_definition_append_portion(&roof.definition,12,&wing)==ROOF_SUCCESS);
    assert(roof_definition_append_composition(&roof.definition,(RoofComposition){11,12,ROOF_COMPOSITION_INTERSECTS})==ROOF_SUCCESS);
    assert(roof_validate(&roof)==ROOF_SUCCESS);
    assert(roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS);
    assert(g.plane_count==4 && count_edges(&g,ROOF_PROTOTYPE_INTERIOR_VALLEY)==2);
    assert(roof_find_portion_by_id(&roof,11) && roof_find_portion_by_id(&roof,12));
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);
}

static void test_termination_and_multilevel(void)
{
    static const PlanPosition main_support[]={{0,0},{12000,0},{12000,8000},{0,8000}};
    static const PlanPosition lean_support[]={{2000,-3000},{10000,-3000},{10000,0},{2000,0}};
    static const PlanPosition overlap_support[]={{3000,7000},{9000,7000},{9000,12000},{3000,12000}};
    Roof roof={0}; RoofPrototypeGeometry g={0};
    RoofPortionSpec main=spec(main_support,ROOF_PORTION_OPPOSING_SLOPES,414214,0,(RoofDirection){1,0},0);
    init_roof(&roof,20,21,&main);
    assert(roof_definition_append_termination(&roof.definition,(RoofTermination){21,ROOF_END_NEGATIVE_AXIS,2000})==ROOF_SUCCESS);
    assert(roof_validate(&roof)==ROOF_SUCCESS && roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS);
    assert(g.plane_count==3 && count_edges(&g,ROOF_PROTOTYPE_INTERIOR_TERMINATION_CUT)==1);
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);

    RoofPortionSpec lean=spec(lean_support,ROOF_PORTION_SINGLE_SLOPE,87489,-300,(RoofDirection){0,-1},ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE);
    init_roof(&roof,30,31,&main); assert(roof_definition_append_portion(&roof.definition,32,&lean)==ROOF_SUCCESS);
    assert(roof_definition_append_composition(&roof.definition,(RoofComposition){31,32,ROOF_COMPOSITION_ABUTS})==ROOF_SUCCESS);
    assert(roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS && g.interface_count==1);
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);

    RoofPortionSpec overlap=spec(overlap_support,ROOF_PORTION_SINGLE_SLOPE,176327,250,(RoofDirection){0,1},ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE);
    init_roof(&roof,40,41,&main); assert(roof_definition_append_portion(&roof.definition,42,&overlap)==ROOF_SUCCESS);
    assert(roof_definition_append_composition(&roof.definition,(RoofComposition){41,42,ROOF_COMPOSITION_INTERSECTS})==ROOF_SUCCESS);
    assert(roof_build_derived_geometry(&roof,&g)==ROOF_SUCCESS && count_edges(&g,ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM)==1);
    assert(g.interior_edges[1].start.y.denominator != 1);
    roof_prototype_geometry_destroy(&g); roof_destroy(&roof);
}

static void test_clone_owns_source(void)
{
    PlanPosition support[]={{0,0},{12000,0},{12000,8000},{0,8000}};
    RoofPortionSpec p=spec(support,ROOF_PORTION_OPPOSING_SLOPES,414214,0,(RoofDirection){1,0},0);
    Roof source={0}, clone={0}; init_roof(&source,50,51,&p);
    assert(roof_clone(&source,&clone)==ROOF_SUCCESS);
    support[0].x=999; source.definition.portions[0].support_vertices[0].x=123;
    assert(clone.definition.portions[0].support_vertices[0].x==0);
    roof_destroy(&source); assert(roof_validate(&clone)==ROOF_SUCCESS); roof_destroy(&clone);
}

int main(void)
{
    test_simple_families(); test_compound_valley_and_ids(); test_termination_and_multilevel(); test_clone_owns_source();
    puts("roof domain: ok"); return 0;
}
