#include <assert.h>
#include <stdio.h>

#include "roof_compound_prototype.h"

static void assert_rational(RoofPrototypeRational value,
    int64_t numerator, int64_t denominator)
{
    assert(value.numerator == numerator);
    assert(value.denominator == denominator);
}

static void assert_xy(RoofPrototypePoint3 point,
    int64_t xn, int64_t xd, int64_t yn, int64_t yd)
{
    assert_rational(point.x, xn, xd);
    assert_rational(point.y, yn, yd);
}


static RoofPrototypeRational plane_z_at(RoofPrototypePlaneEquation plane,
    RoofPrototypeRational x, RoofPrototypeRational y)
{
    int64_t numerator = (plane.gradient_x_ppm * x.numerator * y.denominator) +
        (plane.gradient_y_ppm * y.numerator * x.denominator) +
        (plane.offset_scaled_mm * x.denominator * y.denominator);
    int64_t denominator = ROOF_PROTOTYPE_SLOPE_SCALE * x.denominator * y.denominator;
    int64_t a = numerator < 0 ? -numerator : numerator;
    int64_t b = denominator;
    while (b != 0)
    {
        int64_t r = a % b;
        a = b;
        b = r;
    }
    if (a != 0)
    {
        numerator /= a;
        denominator /= a;
    }
    return (RoofPrototypeRational){numerator, denominator};
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

static RoofPrototypeCompoundIntent c_intent(int64_t wing_slope,
    RoofPrototypeIntent portions[2], RoofPrototypeComposition relation[1])
{
    static const PlanPosition main_support[] = {
        {0,0}, {12000,0}, {12000,8000}, {0,8000}
    };
    static const PlanPosition wing_support[] = {
        {4000,4000}, {8000,4000}, {8000,11000}, {4000,11000}
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
        .support_vertices = wing_support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_OPPOSING_SLOPES,
        .slope_ppm = wing_slope,
        .reference_z_mm = 0,
        .direction = {0, 1}
    };
    relation[0] = (RoofPrototypeComposition){
        .first_portion = 0,
        .second_portion = 1,
        .kind = ROOF_PROTOTYPE_COMPOSITION_INTERSECTS
    };
    return (RoofPrototypeCompoundIntent){
        .portions = portions,
        .portion_count = 2,
        .compositions = relation,
        .composition_count = 1
    };
}

static void test_c0_equal_pitch_derives_two_valleys_and_clipped_planes(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relation[1];
    RoofPrototypeCompoundIntent intent = c_intent(414214, portions, relation);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.plane_count == 4);
    assert(g.boundary_count == 8);
    assert(g.interior_edge_count == 4);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 2);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_VALLEY) == 2);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_HIP) == 0);

    assert(g.planes[0].vertex_count == 4);
    assert_xy(g.planes[0].vertices[0], 0,1, 0,1);
    assert_xy(g.planes[0].vertices[1], 12000,1, 0,1);
    assert_xy(g.planes[0].vertices[2], 12000,1, 4000,1);
    assert_xy(g.planes[0].vertices[3], 0,1, 4000,1);

    assert(g.planes[1].vertex_count == 7);
    assert_xy(g.planes[1].vertices[0], 0,1, 4000,1);
    assert_xy(g.planes[1].vertices[1], 12000,1, 4000,1);
    assert_xy(g.planes[1].vertices[2], 12000,1, 8000,1);
    assert_xy(g.planes[1].vertices[3], 8000,1, 8000,1);
    assert_xy(g.planes[1].vertices[4], 6000,1, 6000,1);
    assert_xy(g.planes[1].vertices[5], 4000,1, 8000,1);
    assert_xy(g.planes[1].vertices[6], 0,1, 8000,1);

    assert_xy(g.planes[2].vertices[0], 4000,1, 8000,1);
    assert_xy(g.planes[2].vertices[1], 6000,1, 6000,1);
    assert_xy(g.planes[2].vertices[2], 6000,1, 11000,1);
    assert_xy(g.planes[2].vertices[3], 4000,1, 11000,1);

    assert_xy(g.planes[3].vertices[0], 6000,1, 6000,1);
    assert_xy(g.planes[3].vertices[1], 8000,1, 8000,1);
    assert_xy(g.planes[3].vertices[2], 8000,1, 11000,1);
    assert_xy(g.planes[3].vertices[3], 6000,1, 11000,1);

    assert_xy(g.interior_edges[0].start, 0,1, 4000,1);
    assert_xy(g.interior_edges[0].end, 12000,1, 4000,1);
    assert_xy(g.interior_edges[1].start, 6000,1, 6000,1);
    assert_xy(g.interior_edges[1].end, 6000,1, 11000,1);
    assert_xy(g.interior_edges[2].start, 4000,1, 8000,1);
    assert_xy(g.interior_edges[2].end, 6000,1, 6000,1);
    assert_xy(g.interior_edges[3].start, 8000,1, 8000,1);
    assert_xy(g.interior_edges[3].end, 6000,1, 6000,1);
    assert_rational(g.interior_edges[2].end.z, 207107, 250);

    roof_prototype_geometry_destroy(&g);
}

static void test_c1_pitch_change_moves_only_derived_valley_network(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relation[1];
    RoofPrototypeCompoundIntent intent = c_intent(577350, portions, relation);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    RoofPrototypePoint3 junction = g.interior_edges[2].end;
    assert_rational(junction.x, 6000, 1);
    assert_rational(junction.y, INT64_C(1079506000), INT64_C(207107));
    assert_rational(junction.z, 11547, 10);
    assert(junction.y.denominator != 1);

    RoofPrototypeRational z_from_main = plane_z_at(g.planes[1].equation,
        junction.x, junction.y);
    RoofPrototypeRational z_from_wing = plane_z_at(g.planes[2].equation,
        junction.x, junction.y);
    assert(z_from_main.numerator == junction.z.numerator);
    assert(z_from_main.denominator == junction.z.denominator);
    assert(z_from_wing.numerator == junction.z.numerator);
    assert(z_from_wing.denominator == junction.z.denominator);

    assert_xy(g.interior_edges[2].start, 4000,1, 8000,1);
    assert_xy(g.interior_edges[3].start, 8000,1, 8000,1);
    assert(g.interior_edges[1].start.y.numerator == junction.y.numerator);
    assert(g.interior_edges[1].start.y.denominator == junction.y.denominator);

    /* The authoritative source coordinates remain untouched. */
    assert(portions[0].support_vertices[2].y == 8000);
    assert(portions[1].support_vertices[0].y == 4000);
    assert(portions[1].support_vertices[2].y == 11000);

    roof_prototype_geometry_destroy(&g);
}

static void test_plan_overlap_without_intersects_is_not_implicit_composition(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relation[1];
    RoofPrototypeCompoundIntent intent = c_intent(414214, portions, relation);
    RoofPrototypeGeometry g = {0};

    intent.composition_count = 0;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_INVALID_ARGUMENT);
    assert(g.planes == NULL);
}

static void test_relation_order_does_not_change_geometry(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relation[1];
    RoofPrototypeCompoundIntent intent = c_intent(577350, portions, relation);
    RoofPrototypeGeometry a = {0}, b = {0};
    assert(roof_prototype_build_compound(&intent, &a) == ROOF_PROTOTYPE_SUCCESS);

    relation[0].first_portion = 1;
    relation[0].second_portion = 0;
    assert(roof_prototype_build_compound(&intent, &b) == ROOF_PROTOTYPE_SUCCESS);

    assert(a.interior_edge_count == b.interior_edge_count);
    for (size_t i = 0; i < a.interior_edge_count; ++i)
    {
        assert(a.interior_edges[i].kind == b.interior_edges[i].kind);
        assert(a.interior_edges[i].start.x.numerator == b.interior_edges[i].start.x.numerator);
        assert(a.interior_edges[i].start.x.denominator == b.interior_edges[i].start.x.denominator);
        assert(a.interior_edges[i].start.y.numerator == b.interior_edges[i].start.y.numerator);
        assert(a.interior_edges[i].start.y.denominator == b.interior_edges[i].start.y.denominator);
        assert(a.interior_edges[i].end.x.numerator == b.interior_edges[i].end.x.numerator);
        assert(a.interior_edges[i].end.x.denominator == b.interior_edges[i].end.x.denominator);
        assert(a.interior_edges[i].end.y.numerator == b.interior_edges[i].end.y.numerator);
        assert(a.interior_edges[i].end.y.denominator == b.interior_edges[i].end.y.denominator);
    }

    roof_prototype_geometry_destroy(&a);
    roof_prototype_geometry_destroy(&b);
}

static void test_failed_compound_build_preserves_existing_output(void)
{
    RoofPrototypeIntent portions[2];
    RoofPrototypeComposition relation[1];
    RoofPrototypeCompoundIntent intent = c_intent(414214, portions, relation);
    RoofPrototypeGeometry g = {0};
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);

    RoofPrototypePlane *planes = g.planes;
    size_t plane_count = g.plane_count;
    portions[1].reference_z_mm = 100;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY);
    assert(g.planes == planes);
    assert(g.plane_count == plane_count);

    roof_prototype_geometry_destroy(&g);
}

int main(void)
{
    test_c0_equal_pitch_derives_two_valleys_and_clipped_planes();
    test_c1_pitch_change_moves_only_derived_valley_network();
    test_plan_overlap_without_intersects_is_not_implicit_composition();
    test_relation_order_does_not_change_geometry();
    test_failed_compound_build_preserves_existing_output();
    puts("roof compound prototype: ok");
    return 0;
}
