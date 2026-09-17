#include "roof_structural_layout_prototype.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int plan_point_equal(PlanPosition a, PlanPosition b)
{
    return a.x == b.x && a.y == b.y;
}

static int support_line_equal_unordered(RoofPrototypeSupportLine a,
    RoofPrototypeSupportLine b)
{
    return (plan_point_equal(a.start, b.start) && plan_point_equal(a.end, b.end)) ||
           (plan_point_equal(a.start, b.end) && plan_point_equal(a.end, b.start));
}

static int support_line_is_axis_aligned_nonzero(RoofPrototypeSupportLine line)
{
    if (plan_point_equal(line.start, line.end))
        return 0;
    return line.start.x == line.end.x || line.start.y == line.end.y;
}

static int direction_is_axis_unit(RoofPrototypeDirection direction)
{
    return (direction.x == 0 && (direction.y == -1 || direction.y == 1)) ||
           (direction.y == 0 && (direction.x == -1 || direction.x == 1));
}

static int direction_perpendicular_to_line(RoofPrototypeDirection direction,
    RoofPrototypeSupportLine line)
{
    if (line.start.y == line.end.y)
        return direction.x == 0 && direction.y != 0;
    if (line.start.x == line.end.x)
        return direction.y == 0 && direction.x != 0;
    return 0;
}

static int rational_is_integer_equal(RoofPrototypeRational value, int integer)
{
    return value.denominator == 1 && value.numerator == (int64_t)integer;
}

static int plane_contains_plan_point(const RoofPrototypePlane *plane,
    PlanPosition point)
{
    if (!plane || !plane->vertices)
        return 0;
    for (size_t i = 0; i < plane->vertex_count; ++i)
    {
        if (rational_is_integer_equal(plane->vertices[i].x, point.x) &&
            rational_is_integer_equal(plane->vertices[i].y, point.y))
            return 1;
    }
    return 0;
}

static int plane_contains_support_line(const RoofPrototypePlane *plane,
    RoofPrototypeSupportLine line)
{
    return plane_contains_plan_point(plane, line.start) &&
           plane_contains_plan_point(plane, line.end);
}

static int find_eave_boundary(const RoofPrototypeGeometry *geometry,
    RoofPrototypeSupportLine line)
{
    if (!geometry)
        return 0;
    for (size_t i = 0; i < geometry->boundary_count; ++i)
    {
        const RoofPrototypeBoundary *boundary = &geometry->boundaries[i];
        if (boundary->kind != ROOF_PROTOTYPE_BOUNDARY_EAVE)
            continue;
        RoofPrototypeSupportLine candidate = {boundary->start, boundary->end};
        if (support_line_equal_unordered(candidate, line))
            return 1;
    }
    return 0;
}

static int find_plane_for_support(const RoofPrototypeGeometry *geometry,
    RoofPrototypeSupportLine line, size_t *plane_index)
{
    size_t found = SIZE_MAX;
    for (size_t i = 0; i < geometry->plane_count; ++i)
    {
        if (!plane_contains_support_line(&geometry->planes[i], line))
            continue;
        if (found != SIZE_MAX)
            return 0; /* ambiguous in this deliberately narrow prototype */
        found = i;
    }
    if (found == SIZE_MAX)
        return 0;
    *plane_index = found;
    return 1;
}

static int find_single_ridge(const RoofPrototypeGeometry *geometry,
    size_t *ridge_index)
{
    size_t found = SIZE_MAX;
    for (size_t i = 0; i < geometry->interior_edge_count; ++i)
    {
        if (geometry->interior_edges[i].kind != ROOF_PROTOTYPE_INTERIOR_RIDGE)
            continue;
        if (found != SIZE_MAX)
            return 0;
        found = i;
    }
    if (found == SIZE_MAX)
        return 0;
    *ridge_index = found;
    return 1;
}

static RoofPrototypeCode validate_common(const RoofPrototypeGeometry *geometry,
    const RoofPrototypeStructuralIntent *intent,
    size_t plane_for_bearing[2], size_t *ridge_index)
{
    if (!geometry || !intent || !plane_for_bearing || !ridge_index)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    if (!geometry->planes || geometry->plane_count != 2 ||
        !geometry->boundaries || !geometry->interior_edges)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    if (!intent->bearing_lines || intent->bearing_line_count != 2)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    if (!direction_is_axis_unit(intent->span_direction))
        return ROOF_PROTOTYPE_INVALID_DIRECTION;

    if (!support_line_is_axis_aligned_nonzero(intent->bearing_lines[0]) ||
        !support_line_is_axis_aligned_nonzero(intent->bearing_lines[1]) ||
        support_line_equal_unordered(intent->bearing_lines[0], intent->bearing_lines[1]))
        return ROOF_PROTOTYPE_INVALID_SUPPORT;

    for (size_t i = 0; i < 2; ++i)
    {
        if (!find_eave_boundary(geometry, intent->bearing_lines[i]))
            return ROOF_PROTOTYPE_INVALID_SUPPORT;
        if (!find_plane_for_support(geometry, intent->bearing_lines[i], &plane_for_bearing[i]))
            return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    }

    if (!direction_perpendicular_to_line(intent->span_direction, intent->bearing_lines[0]) ||
        !direction_perpendicular_to_line(intent->span_direction, intent->bearing_lines[1]))
        return ROOF_PROTOTYPE_INVALID_DIRECTION;
    if (plane_for_bearing[0] == plane_for_bearing[1])
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    if (!find_single_ridge(geometry, ridge_index))
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    return ROOF_PROTOTYPE_SUCCESS;
}

void roof_prototype_structural_layout_destroy(
    RoofPrototypeStructuralLayout *layout)
{
    if (!layout)
        return;
    free(layout->bearing_lines);
    free(layout->fields);
    memset(layout, 0, sizeof(*layout));
}

RoofPrototypeCode roof_prototype_build_structural_layout(
    const RoofPrototypeGeometry *geometry,
    const RoofPrototypeStructuralIntent *intent,
    RoofPrototypeStructuralLayout *output)
{
    size_t plane_for_bearing[2], ridge_index;
    RoofPrototypeCode code;
    RoofPrototypeStructuralLayout candidate = {0};

    if (!output)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    code = validate_common(geometry, intent, plane_for_bearing, &ridge_index);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;

    if (intent->strategy != ROOF_PROTOTYPE_STRUCTURE_CONVENTIONAL_RAFTERS &&
        intent->strategy != ROOF_PROTOTYPE_STRUCTURE_PREFABRICATED_TRUSSES)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    if (intent->conventional_ridge_role != ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING &&
        intent->conventional_ridge_role != ROOF_PROTOTYPE_RIDGE_REQUIRES_BEARING_SUPPORT)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    candidate.strategy = intent->strategy;
    candidate.span_direction = intent->span_direction;
    candidate.conventional_ridge_role = intent->conventional_ridge_role;
    candidate.bearing_line_count = 2;
    candidate.bearing_lines = calloc(2, sizeof(*candidate.bearing_lines));
    if (!candidate.bearing_lines)
        return ROOF_PROTOTYPE_ALLOCATION_FAILED;
    memcpy(candidate.bearing_lines, intent->bearing_lines,
        2 * sizeof(*candidate.bearing_lines));

    if (intent->strategy == ROOF_PROTOTYPE_STRUCTURE_CONVENTIONAL_RAFTERS)
    {
        candidate.field_count = 2;
        candidate.fields = calloc(2, sizeof(*candidate.fields));
        if (!candidate.fields)
        {
            roof_prototype_structural_layout_destroy(&candidate);
            return ROOF_PROTOTYPE_ALLOCATION_FAILED;
        }
        for (size_t i = 0; i < 2; ++i)
        {
            candidate.fields[i].kind = ROOF_PROTOTYPE_LAYOUT_RAFTER_FIELD;
            candidate.fields[i].plane_indices[0] = plane_for_bearing[i];
            candidate.fields[i].plane_count = 1;
            candidate.fields[i].bearing_line_indices[0] = i;
            candidate.fields[i].bearing_line_count = 1;
            candidate.fields[i].interior_edge_index = ridge_index;
        }
    }
    else
    {
        /* The roof ridge remains envelope geometry. A prefabricated truss run
         * spans from one authored bearing line to the other across both roof
         * planes; the ridge is therefore not emitted as a structural support. */
        candidate.field_count = 1;
        candidate.fields = calloc(1, sizeof(*candidate.fields));
        if (!candidate.fields)
        {
            roof_prototype_structural_layout_destroy(&candidate);
            return ROOF_PROTOTYPE_ALLOCATION_FAILED;
        }
        candidate.fields[0].kind = ROOF_PROTOTYPE_LAYOUT_TRUSS_RUN;
        candidate.fields[0].plane_indices[0] = plane_for_bearing[0];
        candidate.fields[0].plane_indices[1] = plane_for_bearing[1];
        candidate.fields[0].plane_count = 2;
        candidate.fields[0].bearing_line_indices[0] = 0;
        candidate.fields[0].bearing_line_indices[1] = 1;
        candidate.fields[0].bearing_line_count = 2;
        candidate.fields[0].interior_edge_index = SIZE_MAX;
        candidate.conventional_ridge_role = ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING;
    }

    roof_prototype_structural_layout_destroy(output);
    *output = candidate;
    return ROOF_PROTOTYPE_SUCCESS;
}
