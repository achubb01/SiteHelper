#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "roof_geometry_prototype.h"

static void assert_rational(RoofPrototypeRational actual,
    int64_t numerator, int64_t denominator)
{
    assert(actual.numerator == numerator);
    assert(actual.denominator == denominator);
}

static void assert_xy(RoofPrototypePoint3 point,
    int64_t x_num, int64_t x_den, int64_t y_num, int64_t y_den)
{
    assert_rational(point.x, x_num, x_den);
    assert_rational(point.y, y_num, y_den);
}

static size_t count_interior_kind(const RoofPrototypeGeometry *geometry,
    RoofPrototypeInteriorEdgeKind kind)
{
    size_t count = 0;
    for (size_t i = 0; i < geometry->interior_edge_count; ++i)
        if (geometry->interior_edges[i].kind == kind)
            ++count;
    return count;
}

static size_t count_boundary_kind(const RoofPrototypeGeometry *geometry,
    RoofPrototypeBoundaryKind kind)
{
    size_t count = 0;
    for (size_t i = 0; i < geometry->boundary_count; ++i)
        if (geometry->boundaries[i].kind == kind)
            ++count;
    return count;
}

static RoofPrototypeIntent a0_intent(const PlanPosition *support)
{
    return (RoofPrototypeIntent){
        .support_vertices = support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_OPPOSING_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {1, 0}
    };
}

static void test_a0_gable_is_derived_from_minimal_intent(void)
{
    const PlanPosition support[] = {
        {12000, 8000}, {0, 8000}, {0, 0}, {12000, 0}
    }; /* deliberately not fixture start order */
    RoofPrototypeIntent intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};

    assert(roof_prototype_build(&intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(geometry.plane_count == 2);
    assert(geometry.boundary_count == 4);
    assert(geometry.interior_edge_count == 1);
    assert(count_interior_kind(&geometry, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 1);
    assert(count_interior_kind(&geometry, ROOF_PROTOTYPE_INTERIOR_HIP) == 0);
    assert(count_boundary_kind(&geometry, ROOF_PROTOTYPE_BOUNDARY_EAVE) == 2);
    assert(count_boundary_kind(&geometry, ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE) == 2);

    assert(geometry.planes[0].equation.gradient_x_ppm == 0);
    assert(geometry.planes[0].equation.gradient_y_ppm == 414214);
    assert(geometry.planes[1].equation.gradient_x_ppm == 0);
    assert(geometry.planes[1].equation.gradient_y_ppm == -414214);

    RoofPrototypeInteriorEdge ridge = geometry.interior_edges[0];
    assert(ridge.kind == ROOF_PROTOTYPE_INTERIOR_RIDGE);
    assert_xy(ridge.start, 0, 1, 4000, 1);
    assert_xy(ridge.end, 12000, 1, 4000, 1);
    assert_rational(ridge.start.z, 207107, 125);
    assert_rational(ridge.end.z, 207107, 125);

    assert(geometry.planes[0].vertex_count == 4);
    assert_xy(geometry.planes[0].vertices[0], 0, 1, 0, 1);
    assert_xy(geometry.planes[0].vertices[1], 12000, 1, 0, 1);
    assert_xy(geometry.planes[0].vertices[2], 12000, 1, 4000, 1);
    assert_xy(geometry.planes[0].vertices[3], 0, 1, 4000, 1);

    roof_prototype_geometry_destroy(&geometry);
    assert(geometry.planes == NULL && geometry.plane_count == 0);
    roof_prototype_geometry_destroy(&geometry);
}

static void test_a1_square_gable_requires_explicit_axis(void)
{
    const PlanPosition support[] = {{0,0}, {8000,0}, {8000,8000}, {0,8000}};
    RoofPrototypeGeometry x_geometry = {0}, y_geometry = {0};
    RoofPrototypeIntent x_intent = a0_intent(support);
    RoofPrototypeIntent y_intent = x_intent;
    y_intent.direction = (RoofPrototypeDirection){0, 1};

    assert(roof_prototype_build(&x_intent, &x_geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build(&y_intent, &y_geometry) == ROOF_PROTOTYPE_SUCCESS);

    assert_xy(x_geometry.interior_edges[0].start, 0, 1, 4000, 1);
    assert_xy(x_geometry.interior_edges[0].end, 8000, 1, 4000, 1);
    assert_xy(y_geometry.interior_edges[0].start, 4000, 1, 0, 1);
    assert_xy(y_geometry.interior_edges[0].end, 4000, 1, 8000, 1);

    x_intent.direction = (RoofPrototypeDirection){0, 0};
    assert(roof_prototype_validate_intent(&x_intent) == ROOF_PROTOTYPE_INVALID_DIRECTION);

    roof_prototype_geometry_destroy(&x_geometry);
    roof_prototype_geometry_destroy(&y_geometry);
}

static void test_b0_equal_pitch_hip_derives_ridge_and_four_hips(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    RoofPrototypeIntent intent = {
        .support_vertices = support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {0, 0}
    };
    RoofPrototypeGeometry geometry = {0};

    assert(roof_prototype_build(&intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(geometry.plane_count == 4);
    assert(geometry.boundary_count == 4);
    assert(geometry.interior_edge_count == 5);
    assert(count_boundary_kind(&geometry, ROOF_PROTOTYPE_BOUNDARY_EAVE) == 4);
    assert(count_interior_kind(&geometry, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 1);
    assert(count_interior_kind(&geometry, ROOF_PROTOTYPE_INTERIOR_HIP) == 4);

    RoofPrototypeInteriorEdge ridge = geometry.interior_edges[0];
    assert_xy(ridge.start, 4000, 1, 4000, 1);
    assert_xy(ridge.end, 8000, 1, 4000, 1);
    assert_rational(ridge.start.z, 207107, 125);
    assert_rational(ridge.end.z, 207107, 125);

    assert(geometry.planes[0].vertex_count == 4); /* south */
    assert_xy(geometry.planes[0].vertices[0], 0, 1, 0, 1);
    assert_xy(geometry.planes[0].vertices[1], 12000, 1, 0, 1);
    assert_xy(geometry.planes[0].vertices[2], 8000, 1, 4000, 1);
    assert_xy(geometry.planes[0].vertices[3], 4000, 1, 4000, 1);
    assert(geometry.planes[2].vertex_count == 3); /* west hip-end */
    assert_xy(geometry.planes[2].vertices[0], 0, 1, 0, 1);
    assert_xy(geometry.planes[2].vertices[1], 4000, 1, 4000, 1);
    assert_xy(geometry.planes[2].vertices[2], 0, 1, 8000, 1);

    roof_prototype_geometry_destroy(&geometry);
}

static void test_e0_and_e1_same_footprint_have_opposite_planes(void)
{
    const PlanPosition support[] = {{0,0}, {8000,0}, {8000,6000}, {0,6000}};
    RoofPrototypeIntent e0 = {
        .support_vertices = support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_SINGLE_SLOPE,
        .slope_ppm = 131652,
        .reference_z_mm = 0,
        .direction = {0, -1}
    };
    RoofPrototypeIntent e1 = e0;
    e1.direction = (RoofPrototypeDirection){0, 1};
    RoofPrototypeGeometry g0 = {0}, g1 = {0};

    assert(roof_prototype_build(&e0, &g0) == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build(&e1, &g1) == ROOF_PROTOTYPE_SUCCESS);

    assert(g0.plane_count == 1 && g1.plane_count == 1);
    assert(g0.interior_edge_count == 0 && g1.interior_edge_count == 0);
    assert(g0.planes[0].equation.gradient_y_ppm == 131652);
    assert(g1.planes[0].equation.gradient_y_ppm == -131652);
    assert(count_boundary_kind(&g0, ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE) == 1);
    assert(count_boundary_kind(&g0, ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE) == 1);
    assert(count_boundary_kind(&g0, ROOF_PROTOTYPE_BOUNDARY_SIDE_VERGE) == 2);
    assert(g0.boundaries[0].kind == ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE);
    assert(g0.boundaries[2].kind == ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE);
    assert(g1.boundaries[2].kind == ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE);
    assert(g1.boundaries[0].kind == ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE);

    assert_rational(g0.planes[0].vertices[0].z, 0, 1);
    assert_rational(g0.planes[0].vertices[3].z, 98739, 125); /* Y=6000 */
    assert_rational(g1.planes[0].vertices[2].z, 0, 1);
    assert_rational(g1.planes[0].vertices[0].z, 98739, 125); /* Y=0 */

    roof_prototype_geometry_destroy(&g0);
    roof_prototype_geometry_destroy(&g1);
}


static void test_fractional_midpoint_is_not_rounded(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8001}, {0,8001}};
    RoofPrototypeIntent intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};

    assert(roof_prototype_build(&intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    RoofPrototypeInteriorEdge ridge = geometry.interior_edges[0];
    assert_xy(ridge.start, 0, 1, 8001, 2);
    assert_xy(ridge.end, 12000, 1, 8001, 2);
    assert(ridge.start.y.denominator == 2);

    roof_prototype_geometry_destroy(&geometry);
}

static void test_square_hip_collapses_ridge_to_apex_without_persisted_special_case(void)
{
    const PlanPosition support[] = {{0,0}, {8000,0}, {8000,8000}, {0,8000}};
    RoofPrototypeIntent intent = {
        .support_vertices = support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {0, 0}
    };
    RoofPrototypeGeometry geometry = {0};

    assert(roof_prototype_build(&intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(geometry.plane_count == 4);
    assert(geometry.interior_edge_count == 4);
    assert(count_interior_kind(&geometry, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 0);
    assert(count_interior_kind(&geometry, ROOF_PROTOTYPE_INTERIOR_HIP) == 4);
    for (size_t i = 0; i < geometry.plane_count; ++i)
        assert(geometry.planes[i].vertex_count == 3);

    roof_prototype_geometry_destroy(&geometry);
}

static void test_failure_is_transactional(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    RoofPrototypeIntent intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};
    assert(roof_prototype_build(&intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);

    RoofPrototypePlane *old_planes = geometry.planes;
    RoofPrototypeBoundary *old_boundaries = geometry.boundaries;
    RoofPrototypeInteriorEdge *old_edges = geometry.interior_edges;
    size_t old_plane_count = geometry.plane_count;

    intent.slope_ppm = 0;
    assert(roof_prototype_build(&intent, &geometry) == ROOF_PROTOTYPE_INVALID_SLOPE);
    assert(geometry.planes == old_planes);
    assert(geometry.boundaries == old_boundaries);
    assert(geometry.interior_edges == old_edges);
    assert(geometry.plane_count == old_plane_count);

    roof_prototype_geometry_destroy(&geometry);
}

static void test_non_rectangular_or_diagonal_26d_inputs_are_explicitly_rejected(void)
{
    const PlanPosition trapezoid[] = {{0,0}, {8000,0}, {7000,6000}, {0,6000}};
    RoofPrototypeIntent intent = a0_intent(trapezoid);
    assert(roof_prototype_validate_intent(&intent) == ROOF_PROTOTYPE_INVALID_SUPPORT);

    const PlanPosition rectangle[] = {{0,0}, {8000,0}, {8000,6000}, {0,6000}};
    intent = a0_intent(rectangle);
    intent.direction = (RoofPrototypeDirection){1, 1};
    assert(roof_prototype_validate_intent(&intent) == ROOF_PROTOTYPE_INVALID_DIRECTION);
}

int main(void)
{
    test_a0_gable_is_derived_from_minimal_intent();
    test_a1_square_gable_requires_explicit_axis();
    test_b0_equal_pitch_hip_derives_ridge_and_four_hips();
    test_e0_and_e1_same_footprint_have_opposite_planes();
    test_fractional_midpoint_is_not_rounded();
    test_square_hip_collapses_ridge_to_apex_without_persisted_special_case();
    test_failure_is_transactional();
    test_non_rectangular_or_diagonal_26d_inputs_are_explicitly_rejected();
    puts("roof geometry prototype: ok");
    return 0;
}
