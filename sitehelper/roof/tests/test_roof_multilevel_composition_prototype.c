#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "roof_compound_prototype.h"

static void assert_rational(RoofPrototypeRational value,
    int64_t numerator, int64_t denominator)
{
    assert(value.numerator == numerator);
    assert(value.denominator == denominator);
}

static void assert_xy(RoofPrototypePoint3 p,
    int64_t xn, int64_t xd, int64_t yn, int64_t yd)
{
    assert_rational(p.x, xn, xd);
    assert_rational(p.y, yn, yd);
}

static size_t count_edges(const RoofPrototypeGeometry *g,
    RoofPrototypeInteriorEdgeKind kind)
{
    size_t count = 0;
    for (size_t i = 0; i < g->interior_edge_count; ++i)
        if (g->interior_edges[i].kind == kind)
            ++count;
    return count;
}

static RoofPrototypeCompoundIntent f0_intent(RoofPrototypeIntent portions[2],
    RoofPrototypeComposition relations[1])
{
    static const PlanPosition main_support[] = {
        {0,0}, {12000,0}, {12000,8000}, {0,8000}
    };
    static const PlanPosition lean_support[] = {
        {2000,-3000}, {10000,-3000}, {10000,0}, {2000,0}
    };
    portions[0] = (RoofPrototypeIntent){
        .support_vertices = main_support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_OPPOSING_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {1, 0}
    };
    portions[1] = (RoofPrototypeIntent){
        .support_vertices = lean_support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_SINGLE_SLOPE,
        .slope_ppm = 87489,
        .reference_z_mm = -300,
        .direction = {0, -1},
        .single_slope_reference = ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE
    };
    relations[0] = (RoofPrototypeComposition){
        .first_portion = 0,
        .second_portion = 1,
        .kind = ROOF_PROTOTYPE_COMPOSITION_ABUTS
    };
    return (RoofPrototypeCompoundIntent){
        .portions = portions,
        .portion_count = 2,
        .compositions = relations,
        .composition_count = 1
    };
}

static RoofPrototypeCompoundIntent f1_intent(RoofPrototypeIntent portions[2],
    RoofPrototypeComposition relations[1])
{
    static const PlanPosition main_support[] = {
        {0,0}, {12000,0}, {12000,8000}, {0,8000}
    };
    static const PlanPosition overlap_support[] = {
        {3000,7000}, {9000,7000}, {9000,12000}, {3000,12000}
    };
    portions[0] = (RoofPrototypeIntent){
        .support_vertices = main_support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_OPPOSING_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {1, 0}
    };
    portions[1] = (RoofPrototypeIntent){
        .support_vertices = overlap_support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_SINGLE_SLOPE,
        .slope_ppm = 176327,
        .reference_z_mm = 250,
        .direction = {0, 1},
        .single_slope_reference = ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE
    };
    relations[0] = (RoofPrototypeComposition){
        .first_portion = 0,
        .second_portion = 1,
        .kind = ROOF_PROTOTYPE_COMPOSITION_INTERSECTS
    };
    return (RoofPrototypeCompoundIntent){
        .portions = portions,
        .portion_count = 2,
        .compositions = relations,
        .composition_count = 1
    };
}

static void test_f0_abutment_keeps_two_edges_at_same_plan_position(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relations[1];
    RoofPrototypeCompoundIntent intent = f0_intent(portions, relations);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.plane_count == 3);
    assert(g.interface_count == 1);
    assert(g.interfaces[0].kind == ROOF_PROTOTYPE_INTERFACE_STEP_ABUTMENT);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 1);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_VALLEY) == 0);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM) == 0);

    RoofPrototypeInteriorEdge main_edge = g.interfaces[0].first_edge;
    RoofPrototypeInteriorEdge lean_edge = g.interfaces[0].second_edge;
    assert_xy(main_edge.start, 2000,1, 0,1);
    assert_xy(main_edge.end, 10000,1, 0,1);
    assert_xy(lean_edge.start, 2000,1, 0,1);
    assert_xy(lean_edge.end, 10000,1, 0,1);
    assert_rational(main_edge.start.z, 0, 1);
    assert_rational(main_edge.end.z, 0, 1);
    assert_rational(lean_edge.start.z, -300, 1);
    assert_rational(lean_edge.end.z, -300, 1);

    /* The same plan segment therefore remains two physical 3D edges. */
    assert(main_edge.start.z.numerator != lean_edge.start.z.numerator);

    roof_prototype_geometry_destroy(&g);
}

static void test_f0_semantics_are_explicit_not_inferred_from_contact(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relations[1];
    RoofPrototypeCompoundIntent intent = f0_intent(portions, relations);
    RoofPrototypeGeometry g = {0};

    intent.composition_count = 0;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_INVALID_ARGUMENT);
    assert(g.planes == NULL);

    intent.composition_count = 1;
    relations[0].kind = ROOF_PROTOTYPE_COMPOSITION_INTERSECTS;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY);
    assert(g.planes == NULL);
}

static void test_f1_intersection_clips_surfaces_at_exact_plane_seam(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relations[1];
    RoofPrototypeCompoundIntent intent = f1_intent(portions, relations);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.plane_count == 3);
    assert(g.interface_count == 0);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 1);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM) == 1);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_VALLEY) == 0);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_HIP) == 0);

    RoofPrototypeInteriorEdge seam = g.interior_edges[1];
    assert_xy(seam.start, 3000,1, INT64_C(1829423000),INT64_C(237887));
    assert_xy(seam.end, 9000,1, INT64_C(1829423000),INT64_C(237887));
    assert_rational(seam.start.z, INT64_C(15258194011), INT64_C(118943500));
    assert_rational(seam.end.z, INT64_C(15258194011), INT64_C(118943500));
    assert(seam.start.y.denominator != 1);

    /* Frozen F1 visible polygons. */
    assert(g.planes[0].vertex_count == 4);
    assert_xy(g.planes[0].vertices[0], 0,1, 0,1);
    assert_xy(g.planes[0].vertices[2], 12000,1, 4000,1);

    assert(g.planes[1].vertex_count == 8);
    assert_xy(g.planes[1].vertices[4], 9000,1,
        INT64_C(1829423000),INT64_C(237887));
    assert_xy(g.planes[1].vertices[5], 3000,1,
        INT64_C(1829423000),INT64_C(237887));

    assert(g.planes[2].vertex_count == 4);
    assert_xy(g.planes[2].vertices[0], 3000,1,
        INT64_C(1829423000),INT64_C(237887));
    assert_xy(g.planes[2].vertices[1], 9000,1,
        INT64_C(1829423000),INT64_C(237887));
    assert_xy(g.planes[2].vertices[2], 9000,1, 12000,1);

    /* The shared seam Z is exactly equal on both source plane equations. */
    assert(g.planes[1].vertices[5].z.numerator == g.planes[2].vertices[0].z.numerator);
    assert(g.planes[1].vertices[5].z.denominator == g.planes[2].vertices[0].z.denominator);

    roof_prototype_geometry_destroy(&g);
}

static void test_f1_high_edge_datum_is_authoritative(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relations[1];
    RoofPrototypeCompoundIntent intent = f1_intent(portions, relations);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    RoofPrototypeRational original_y = g.interior_edges[1].start.y;

    portions[1].reference_z_mm = 300;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.interior_edges[1].start.y.numerator != original_y.numerator ||
        g.interior_edges[1].start.y.denominator != original_y.denominator);

    roof_prototype_geometry_destroy(&g);
}

static void test_failed_e3_build_preserves_existing_output(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relations[1];
    RoofPrototypeCompoundIntent intent = f1_intent(portions, relations);
    RoofPrototypeGeometry g = {0};
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);

    RoofPrototypePlane *planes = g.planes;
    size_t plane_count = g.plane_count;
    portions[1].single_slope_reference = ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_LOW_EDGE;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY);
    assert(g.planes == planes);
    assert(g.plane_count == plane_count);

    roof_prototype_geometry_destroy(&g);
}

int main(void)
{
    test_f0_abutment_keeps_two_edges_at_same_plan_position();
    test_f0_semantics_are_explicit_not_inferred_from_contact();
    test_f1_intersection_clips_surfaces_at_exact_plane_seam();
    test_f1_high_edge_datum_is_authoritative();
    test_failed_e3_build_preserves_existing_output();
    puts("roof multi-level composition prototype: ok");
    return 0;
}
