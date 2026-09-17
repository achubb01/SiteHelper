#include <assert.h>
#include <stdio.h>

#include "roof_compound_prototype.h"

static void assert_rational(RoofPrototypeRational value,
    int64_t numerator, int64_t denominator)
{
    assert(value.numerator == numerator);
    assert(value.denominator == denominator);
}

static void assert_xyz(RoofPrototypePoint3 point,
    int64_t x, int64_t y, int64_t zn, int64_t zd)
{
    assert_rational(point.x, x, 1);
    assert_rational(point.y, y, 1);
    assert_rational(point.z, zn, zd);
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

static RoofPrototypeCompoundIntent d0_intent(RoofPrototypeIntent portions[1],
    RoofPrototypeTermination terminations[1])
{
    static const PlanPosition support[] = {
        {0,0}, {12000,0}, {12000,8000}, {0,8000}
    };
    portions[0] = (RoofPrototypeIntent){
        .support_vertices = support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_OPPOSING_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {1, 0}
    };
    terminations[0] = (RoofPrototypeTermination){
        .portion = 0,
        .end = ROOF_PROTOTYPE_END_NEGATIVE_AXIS,
        .termination_offset_mm = 2000
    };
    return (RoofPrototypeCompoundIntent){
        .portions = portions,
        .portion_count = 1,
        .terminations = terminations,
        .termination_count = 1
    };
}

static void test_d0_derives_hip_end_cut_without_dutch_gable_type(void)
{
    RoofPrototypeIntent portions[1];
    RoofPrototypeTermination terminations[1];
    RoofPrototypeCompoundIntent intent = d0_intent(portions, terminations);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.plane_count == 3);
    assert(g.boundary_count == 4);
    assert(g.interior_edge_count == 4);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_RIDGE) == 1);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_HIP) == 2);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_VALLEY) == 0);
    assert(count_edges(&g, ROOF_PROTOTYPE_INTERIOR_TERMINATION_CUT) == 1);

    /* Frozen D0 plan polygons. */
    assert(g.planes[0].vertex_count == 5);
    assert_xyz(g.planes[0].vertices[0], 0, 0, 0, 1);
    assert_xyz(g.planes[0].vertices[1], 12000, 0, 0, 1);
    assert_xyz(g.planes[0].vertices[2], 12000, 4000, 207107, 125);
    assert_xyz(g.planes[0].vertices[3], 2000, 4000, 207107, 125);
    assert_xyz(g.planes[0].vertices[4], 2000, 2000, 207107, 250);

    assert(g.planes[1].vertex_count == 5);
    assert_xyz(g.planes[1].vertices[0], 0, 8000, 0, 1);
    assert_xyz(g.planes[1].vertices[1], 2000, 6000, 207107, 250);
    assert_xyz(g.planes[1].vertices[2], 2000, 4000, 207107, 125);
    assert_xyz(g.planes[1].vertices[3], 12000, 4000, 207107, 125);
    assert_xyz(g.planes[1].vertices[4], 12000, 8000, 0, 1);

    assert(g.planes[2].vertex_count == 4);
    assert_xyz(g.planes[2].vertices[0], 0, 0, 0, 1);
    assert_xyz(g.planes[2].vertices[1], 2000, 2000, 207107, 250);
    assert_xyz(g.planes[2].vertices[2], 2000, 6000, 207107, 250);
    assert_xyz(g.planes[2].vertices[3], 0, 8000, 0, 1);

    /* The lower hip plane is an eave at the west boundary; east remains gable. */
    assert(g.boundaries[0].kind == ROOF_PROTOTYPE_BOUNDARY_EAVE);
    assert(g.boundaries[1].kind == ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE);
    assert(g.boundaries[2].kind == ROOF_PROTOTYPE_BOUNDARY_EAVE);
    assert(g.boundaries[3].kind == ROOF_PROTOTYPE_BOUNDARY_EAVE);

    assert_xyz(g.interior_edges[0].start, 2000, 4000, 207107, 125);
    assert_xyz(g.interior_edges[0].end, 12000, 4000, 207107, 125);
    assert_xyz(g.interior_edges[1].start, 0, 0, 0, 1);
    assert_xyz(g.interior_edges[1].end, 2000, 2000, 207107, 250);
    assert_xyz(g.interior_edges[2].start, 0, 8000, 0, 1);
    assert_xyz(g.interior_edges[2].end, 2000, 6000, 207107, 250);
    assert_xyz(g.interior_edges[3].start, 2000, 2000, 207107, 250);
    assert_xyz(g.interior_edges[3].end, 2000, 6000, 207107, 250);

    /* The closure is not a fourth roof plane. Its apex is derivable from the
     * ridge start and its base from the termination cut. */
    assert(g.interior_edges[3].start.x.numerator ==
        g.interior_edges[0].start.x.numerator);
    assert(g.interior_edges[3].end.x.numerator ==
        g.interior_edges[0].start.x.numerator);

    roof_prototype_geometry_destroy(&g);
}

static void test_termination_offset_is_authority_and_regenerates_geometry(void)
{
    RoofPrototypeIntent portions[1];
    RoofPrototypeTermination terminations[1];
    RoofPrototypeCompoundIntent intent = d0_intent(portions, terminations);
    RoofPrototypeGeometry g = {0};

    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.interior_edges[3].start.x.numerator == 2000);
    assert_rational(g.interior_edges[3].start.z, 207107, 250);

    terminations[0].termination_offset_mm = 1500;
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);
    assert(g.interior_edges[3].start.x.numerator == 1500);
    assert(g.interior_edges[3].start.y.numerator == 1500);
    assert_rational(g.interior_edges[3].start.z, 621321, 1000);
    assert(g.interior_edges[0].start.x.numerator == 1500);
    assert_rational(g.interior_edges[0].start.z, 207107, 125);

    roof_prototype_geometry_destroy(&g);
}

static void test_unsupported_termination_preserves_existing_output(void)
{
    RoofPrototypeIntent portions[1];
    RoofPrototypeTermination terminations[1];
    RoofPrototypeCompoundIntent intent = d0_intent(portions, terminations);
    RoofPrototypeGeometry g = {0};
    assert(roof_prototype_build_compound(&intent, &g) == ROOF_PROTOTYPE_SUCCESS);

    RoofPrototypePlane *planes = g.planes;
    size_t plane_count = g.plane_count;
    terminations[0].termination_offset_mm = 4000;
    assert(roof_prototype_build_compound(&intent, &g) ==
        ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY);
    assert(g.planes == planes);
    assert(g.plane_count == plane_count);

    roof_prototype_geometry_destroy(&g);
}

int main(void)
{
    test_d0_derives_hip_end_cut_without_dutch_gable_type();
    test_termination_offset_is_authority_and_regenerates_geometry();
    test_unsupported_termination_preserves_existing_output();
    puts("roof termination prototype: ok");
    return 0;
}
