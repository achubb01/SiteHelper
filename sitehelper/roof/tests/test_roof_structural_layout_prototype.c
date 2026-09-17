#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "roof_geometry_prototype.h"
#include "roof_structural_layout_prototype.h"

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

static RoofPrototypeStructuralIntent conventional_intent(
    const RoofPrototypeSupportLine *supports)
{
    return (RoofPrototypeStructuralIntent){
        .strategy = ROOF_PROTOTYPE_STRUCTURE_CONVENTIONAL_RAFTERS,
        .bearing_lines = supports,
        .bearing_line_count = 2,
        .span_direction = {0, 1},
        .conventional_ridge_role = ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING
    };
}

static RoofPrototypeStructuralIntent truss_intent(
    const RoofPrototypeSupportLine *supports)
{
    return (RoofPrototypeStructuralIntent){
        .strategy = ROOF_PROTOTYPE_STRUCTURE_PREFABRICATED_TRUSSES,
        .bearing_lines = supports,
        .bearing_line_count = 2,
        .span_direction = {0, 1},
        .conventional_ridge_role = ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING
    };
}

static void test_same_a0_geometry_resolves_to_two_different_structural_layouts(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    const RoofPrototypeSupportLine bearings[] = {
        {{0,0}, {12000,0}},
        {{12000,8000}, {0,8000}} /* reversed on purpose */
    };
    RoofPrototypeIntent roof_intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};
    RoofPrototypeStructuralLayout conventional = {0};
    RoofPrototypeStructuralLayout truss = {0};
    RoofPrototypeStructuralIntent c_intent = conventional_intent(bearings);
    RoofPrototypeStructuralIntent t_intent = truss_intent(bearings);

    assert(roof_prototype_build(&roof_intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(geometry.plane_count == 2);
    assert(geometry.interior_edge_count == 1);
    assert(geometry.interior_edges[0].kind == ROOF_PROTOTYPE_INTERIOR_RIDGE);

    RoofPrototypePlane *same_planes = geometry.planes;
    RoofPrototypeInteriorEdge *same_edges = geometry.interior_edges;

    assert(roof_prototype_build_structural_layout(&geometry, &c_intent, &conventional)
        == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build_structural_layout(&geometry, &t_intent, &truss)
        == ROOF_PROTOTYPE_SUCCESS);

    /* Both strategies consume the exact same immutable geometry snapshot. */
    assert(geometry.planes == same_planes);
    assert(geometry.interior_edges == same_edges);
    assert(geometry.plane_count == 2);
    assert(geometry.interior_edge_count == 1);

    /* Conventional layout resolves one rafter field per roof plane. */
    assert(conventional.strategy == ROOF_PROTOTYPE_STRUCTURE_CONVENTIONAL_RAFTERS);
    assert(conventional.field_count == 2);
    assert(conventional.bearing_line_count == 2);
    for (size_t i = 0; i < conventional.field_count; ++i)
    {
        assert(conventional.fields[i].kind == ROOF_PROTOTYPE_LAYOUT_RAFTER_FIELD);
        assert(conventional.fields[i].plane_count == 1);
        assert(conventional.fields[i].bearing_line_count == 1);
        assert(conventional.fields[i].interior_edge_index == 0);
    }
    assert(conventional.fields[0].plane_indices[0] != conventional.fields[1].plane_indices[0]);

    /* Trussed layout spans bearing-to-bearing across both envelope planes.
     * The geometric ridge is not silently promoted into a support. */
    assert(truss.strategy == ROOF_PROTOTYPE_STRUCTURE_PREFABRICATED_TRUSSES);
    assert(truss.field_count == 1);
    assert(truss.fields[0].kind == ROOF_PROTOTYPE_LAYOUT_TRUSS_RUN);
    assert(truss.fields[0].plane_count == 2);
    assert(truss.fields[0].bearing_line_count == 2);
    assert(truss.fields[0].interior_edge_index == SIZE_MAX);

    roof_prototype_structural_layout_destroy(&conventional);
    roof_prototype_structural_layout_destroy(&truss);
    roof_prototype_geometry_destroy(&geometry);
}

static void test_geometric_eave_is_not_automatically_a_bearing_line(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    RoofPrototypeIntent roof_intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};
    RoofPrototypeStructuralLayout layout = {0};
    const RoofPrototypeSupportLine wrong_supports[] = {
        {{0,0}, {12000,0}},
        {{0,0}, {0,8000}} /* gable verge, not an authored eave bearing */
    };
    RoofPrototypeStructuralIntent intent = conventional_intent(wrong_supports);

    assert(roof_prototype_build(&roof_intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build_structural_layout(&geometry, &intent, &layout)
        == ROOF_PROTOTYPE_INVALID_SUPPORT);
    assert(layout.fields == NULL && layout.field_count == 0);

    roof_prototype_geometry_destroy(&geometry);
}

static void test_conventional_ridge_support_role_is_authored_not_inferred(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    const RoofPrototypeSupportLine bearings[] = {
        {{0,0}, {12000,0}}, {{0,8000}, {12000,8000}}
    };
    RoofPrototypeIntent roof_intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};
    RoofPrototypeStructuralLayout nonbearing = {0}, bearing = {0};
    RoofPrototypeStructuralIntent intent = conventional_intent(bearings);

    assert(roof_prototype_build(&roof_intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build_structural_layout(&geometry, &intent, &nonbearing)
        == ROOF_PROTOTYPE_SUCCESS);
    assert(nonbearing.conventional_ridge_role == ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING);

    intent.conventional_ridge_role = ROOF_PROTOTYPE_RIDGE_REQUIRES_BEARING_SUPPORT;
    assert(roof_prototype_build_structural_layout(&geometry, &intent, &bearing)
        == ROOF_PROTOTYPE_SUCCESS);
    assert(bearing.conventional_ridge_role == ROOF_PROTOTYPE_RIDGE_REQUIRES_BEARING_SUPPORT);

    /* Envelope geometry did not change merely because structural support intent did. */
    assert(geometry.interior_edge_count == 1);
    assert(geometry.interior_edges[0].kind == ROOF_PROTOTYPE_INTERIOR_RIDGE);

    roof_prototype_structural_layout_destroy(&nonbearing);
    roof_prototype_structural_layout_destroy(&bearing);
    roof_prototype_geometry_destroy(&geometry);
}

static void test_layout_references_are_snapshot_local_and_build_is_transactional(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    const RoofPrototypeSupportLine bearings[] = {
        {{0,0}, {12000,0}}, {{0,8000}, {12000,8000}}
    };
    RoofPrototypeIntent roof_intent = a0_intent(support);
    RoofPrototypeGeometry geometry = {0};
    RoofPrototypeStructuralLayout layout = {0};
    RoofPrototypeStructuralIntent intent = conventional_intent(bearings);

    assert(roof_prototype_build(&roof_intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build_structural_layout(&geometry, &intent, &layout)
        == ROOF_PROTOTYPE_SUCCESS);

    RoofPrototypeStructuralField *old_fields = layout.fields;
    RoofPrototypeSupportLine *old_supports = layout.bearing_lines;
    size_t old_count = layout.field_count;

    intent.span_direction = (RoofPrototypeDirection){1, 0};
    assert(roof_prototype_build_structural_layout(&geometry, &intent, &layout)
        == ROOF_PROTOTYPE_INVALID_DIRECTION);
    assert(layout.fields == old_fields);
    assert(layout.bearing_lines == old_supports);
    assert(layout.field_count == old_count);

    roof_prototype_structural_layout_destroy(&layout);
    roof_prototype_geometry_destroy(&geometry);
}

static void test_26f_does_not_claim_general_hip_or_compound_structural_resolution(void)
{
    const PlanPosition support[] = {{0,0}, {12000,0}, {12000,8000}, {0,8000}};
    const RoofPrototypeSupportLine bearings[] = {
        {{0,0}, {12000,0}}, {{0,8000}, {12000,8000}}
    };
    RoofPrototypeIntent hip_intent = {
        .support_vertices = support,
        .support_vertex_count = 4,
        .generation = ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES,
        .slope_ppm = 414214,
        .reference_z_mm = 0,
        .direction = {0,0}
    };
    RoofPrototypeGeometry geometry = {0};
    RoofPrototypeStructuralLayout layout = {0};
    RoofPrototypeStructuralIntent intent = conventional_intent(bearings);

    assert(roof_prototype_build(&hip_intent, &geometry) == ROOF_PROTOTYPE_SUCCESS);
    assert(roof_prototype_build_structural_layout(&geometry, &intent, &layout)
        == ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY);

    roof_prototype_geometry_destroy(&geometry);
}

int main(void)
{
    test_same_a0_geometry_resolves_to_two_different_structural_layouts();
    test_geometric_eave_is_not_automatically_a_bearing_line();
    test_conventional_ridge_support_role_is_authored_not_inferred();
    test_layout_references_are_snapshot_local_and_build_is_transactional();
    test_26f_does_not_claim_general_hip_or_compound_structural_resolution();
    puts("roof structural layout prototype: ok");
    return 0;
}
