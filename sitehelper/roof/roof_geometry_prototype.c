#include "roof_geometry_prototype.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int xmin, xmax, ymin, ymax;
} RoofPrototypeRect;

static uint64_t magnitude_i64(int64_t value)
{
    return value < 0 ? UINT64_C(0) - (uint64_t)value : (uint64_t)value;
}

static uint64_t gcd_u64(uint64_t a, uint64_t b)
{
    while (b != 0)
    {
        uint64_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

static int checked_add_i64(int64_t a, int64_t b, int64_t *out)
{
    if ((b > 0 && a > INT64_MAX - b) ||
        (b < 0 && a < INT64_MIN - b))
        return 0;
    *out = a + b;
    return 1;
}

static int checked_sub_i64(int64_t a, int64_t b, int64_t *out)
{
    if ((b < 0 && a > INT64_MAX + b) ||
        (b > 0 && a < INT64_MIN + b))
        return 0;
    *out = a - b;
    return 1;
}

static int checked_mul_i64(int64_t a, int64_t b, int64_t *out)
{
    if (a == 0 || b == 0)
    {
        *out = 0;
        return 1;
    }
    if (a == -1 && b == INT64_MIN)
        return 0;
    if (b == -1 && a == INT64_MIN)
        return 0;

    if (a > 0)
    {
        if (b > 0)
        {
            if (a > INT64_MAX / b)
                return 0;
        }
        else if (b < INT64_MIN / a)
            return 0;
    }
    else
    {
        if (b > 0)
        {
            if (a < INT64_MIN / b)
                return 0;
        }
        else if (a < INT64_MAX / b)
            return 0;
    }

    *out = a * b;
    return 1;
}

static RoofPrototypeCode rational_make(int64_t numerator, int64_t denominator,
    RoofPrototypeRational *out)
{
    if (!out || denominator <= 0)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    if (numerator == 0)
    {
        *out = (RoofPrototypeRational){0, 1};
        return ROOF_PROTOTYPE_SUCCESS;
    }

    uint64_t divisor = gcd_u64(magnitude_i64(numerator), (uint64_t)denominator);
    *out = (RoofPrototypeRational){
        numerator / (int64_t)divisor,
        denominator / (int64_t)divisor
    };
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeRational rational_integer(int value)
{
    return (RoofPrototypeRational){value, 1};
}

static RoofPrototypeCode rational_half_sum(int a, int b, RoofPrototypeRational *out)
{
    int64_t sum;
    if (!checked_add_i64((int64_t)a, (int64_t)b, &sum))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    return rational_make(sum, 2, out);
}

static int rational_equal(RoofPrototypeRational a, RoofPrototypeRational b)
{
    return a.numerator == b.numerator && a.denominator == b.denominator;
}

static int point_plan_equal(RoofPrototypePoint3 a, RoofPrototypePoint3 b)
{
    return rational_equal(a.x, b.x) && rational_equal(a.y, b.y);
}

static RoofPrototypeCode plane_from_reference(int x, int y, int z,
    int64_t gx, int64_t gy, RoofPrototypePlaneEquation *out)
{
    int64_t z_scaled, gx_x, gy_y, offset;
    if (!checked_mul_i64(ROOF_PROTOTYPE_SLOPE_SCALE, (int64_t)z, &z_scaled) ||
        !checked_mul_i64(gx, (int64_t)x, &gx_x) ||
        !checked_mul_i64(gy, (int64_t)y, &gy_y) ||
        !checked_sub_i64(z_scaled, gx_x, &offset) ||
        !checked_sub_i64(offset, gy_y, &offset))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;

    *out = (RoofPrototypePlaneEquation){gx, gy, offset};
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode plane_z_at(const RoofPrototypePlaneEquation *plane,
    RoofPrototypeRational x, RoofPrototypeRational y, RoofPrototypeRational *out)
{
    int64_t xy_den, x_term, y_term, offset_term, numerator, denominator;
    int64_t temp;
    if (!checked_mul_i64(x.denominator, y.denominator, &xy_den) ||
        !checked_mul_i64(plane->gradient_x_ppm, x.numerator, &temp) ||
        !checked_mul_i64(temp, y.denominator, &x_term) ||
        !checked_mul_i64(plane->gradient_y_ppm, y.numerator, &temp) ||
        !checked_mul_i64(temp, x.denominator, &y_term) ||
        !checked_mul_i64(plane->offset_scaled_mm, xy_den, &offset_term) ||
        !checked_add_i64(x_term, y_term, &numerator) ||
        !checked_add_i64(numerator, offset_term, &numerator) ||
        !checked_mul_i64(ROOF_PROTOTYPE_SLOPE_SCALE, xy_den, &denominator))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;

    return rational_make(numerator, denominator, out);
}

static RoofPrototypeCode point_from_xy(const RoofPrototypePlaneEquation *plane,
    RoofPrototypeRational x, RoofPrototypeRational y, RoofPrototypePoint3 *out)
{
    RoofPrototypeRational z;
    RoofPrototypeCode code = plane_z_at(plane, x, y, &z);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    *out = (RoofPrototypePoint3){x, y, z};
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode point_from_integer_xy(const RoofPrototypePlaneEquation *plane,
    int x, int y, RoofPrototypePoint3 *out)
{
    return point_from_xy(plane, rational_integer(x), rational_integer(y), out);
}

static RoofPrototypeCode support_rect(const RoofPrototypeIntent *intent,
    RoofPrototypeRect *out)
{
    if (!intent || !intent->support_vertices || intent->support_vertex_count != 4 || !out)
        return ROOF_PROTOTYPE_INVALID_SUPPORT;

    int xmin = intent->support_vertices[0].x;
    int xmax = xmin;
    int ymin = intent->support_vertices[0].y;
    int ymax = ymin;
    for (size_t i = 1; i < 4; ++i)
    {
        PlanPosition p = intent->support_vertices[i];
        if (p.x < xmin) xmin = p.x;
        if (p.x > xmax) xmax = p.x;
        if (p.y < ymin) ymin = p.y;
        if (p.y > ymax) ymax = p.y;
    }
    if (xmin == xmax || ymin == ymax)
        return ROOF_PROTOTYPE_INVALID_SUPPORT;

    unsigned mask = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        PlanPosition p = intent->support_vertices[i];
        unsigned bit;
        if (p.x == xmin && p.y == ymin) bit = 1u;
        else if (p.x == xmax && p.y == ymin) bit = 2u;
        else if (p.x == xmax && p.y == ymax) bit = 4u;
        else if (p.x == xmin && p.y == ymax) bit = 8u;
        else return ROOF_PROTOTYPE_INVALID_SUPPORT;
        if (mask & bit)
            return ROOF_PROTOTYPE_INVALID_SUPPORT;
        mask |= bit;
    }
    if (mask != 15u)
        return ROOF_PROTOTYPE_INVALID_SUPPORT;

    *out = (RoofPrototypeRect){xmin, xmax, ymin, ymax};
    return ROOF_PROTOTYPE_SUCCESS;
}

static int direction_is_axis_unit(RoofPrototypeDirection direction)
{
    return (direction.x == 0 && (direction.y == -1 || direction.y == 1)) ||
           (direction.y == 0 && (direction.x == -1 || direction.x == 1));
}

RoofPrototypeCode roof_prototype_validate_intent(const RoofPrototypeIntent *intent)
{
    RoofPrototypeRect rect;
    RoofPrototypeCode code;
    if (!intent)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    code = support_rect(intent, &rect);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    (void)rect;

    if (intent->slope_ppm <= 0 || intent->slope_ppm > INT32_MAX)
        return ROOF_PROTOTYPE_INVALID_SLOPE;

    switch (intent->generation)
    {
        case ROOF_PROTOTYPE_OPPOSING_SLOPES:
            return direction_is_axis_unit(intent->direction)
                ? ROOF_PROTOTYPE_SUCCESS : ROOF_PROTOTYPE_INVALID_DIRECTION;
        case ROOF_PROTOTYPE_SINGLE_SLOPE:
            if (intent->single_slope_reference !=
                    ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_LOW_EDGE &&
                intent->single_slope_reference !=
                    ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE)
                return ROOF_PROTOTYPE_INVALID_ARGUMENT;
            return direction_is_axis_unit(intent->direction)
                ? ROOF_PROTOTYPE_SUCCESS : ROOF_PROTOTYPE_INVALID_DIRECTION;
        case ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES:
            return intent->direction.x == 0 && intent->direction.y == 0
                ? ROOF_PROTOTYPE_SUCCESS : ROOF_PROTOTYPE_INVALID_DIRECTION;
        default:
            return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    }
}

static RoofPrototypeCode geometry_allocate(RoofPrototypeGeometry *geometry,
    size_t plane_count, const size_t *plane_vertex_counts,
    size_t boundary_count, size_t interior_edge_count)
{
    memset(geometry, 0, sizeof *geometry);
    if (plane_count)
    {
        geometry->planes = calloc(plane_count, sizeof *geometry->planes);
        if (!geometry->planes)
            goto allocation_failed;
        geometry->plane_count = plane_count;
        for (size_t i = 0; i < plane_count; ++i)
        {
            geometry->planes[i].vertices = calloc(plane_vertex_counts[i],
                sizeof *geometry->planes[i].vertices);
            if (!geometry->planes[i].vertices)
                goto allocation_failed;
            geometry->planes[i].vertex_count = plane_vertex_counts[i];
        }
    }
    if (boundary_count)
    {
        geometry->boundaries = calloc(boundary_count, sizeof *geometry->boundaries);
        if (!geometry->boundaries)
            goto allocation_failed;
        geometry->boundary_count = boundary_count;
    }
    if (interior_edge_count)
    {
        geometry->interior_edges = calloc(interior_edge_count,
            sizeof *geometry->interior_edges);
        if (!geometry->interior_edges)
            goto allocation_failed;
        geometry->interior_edge_count = interior_edge_count;
    }
    return ROOF_PROTOTYPE_SUCCESS;

allocation_failed:
    roof_prototype_geometry_destroy(geometry);
    return ROOF_PROTOTYPE_ALLOCATION_FAILED;
}

static void set_canonical_boundaries(const RoofPrototypeRect *r,
    RoofPrototypeBoundary *boundaries)
{
    boundaries[0].start = (PlanPosition){r->xmin, r->ymin};
    boundaries[0].end   = (PlanPosition){r->xmax, r->ymin}; /* south */
    boundaries[1].start = (PlanPosition){r->xmax, r->ymin};
    boundaries[1].end   = (PlanPosition){r->xmax, r->ymax}; /* east */
    boundaries[2].start = (PlanPosition){r->xmax, r->ymax};
    boundaries[2].end   = (PlanPosition){r->xmin, r->ymax}; /* north */
    boundaries[3].start = (PlanPosition){r->xmin, r->ymax};
    boundaries[3].end   = (PlanPosition){r->xmin, r->ymin}; /* west */
}

static RoofPrototypeCode build_gable(const RoofPrototypeIntent *intent,
    const RoofPrototypeRect *r, RoofPrototypeGeometry *geometry)
{
    const size_t vertex_counts[2] = {4, 4};
    RoofPrototypeCode code = geometry_allocate(geometry, 2, vertex_counts, 4, 1);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    set_canonical_boundaries(r, geometry->boundaries);

    RoofPrototypeRational mid;
    RoofPrototypePoint3 ridge_start, ridge_end;
    if (intent->direction.y == 0) /* ridge parallel X */
    {
        code = rational_half_sum(r->ymin, r->ymax, &mid);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        code = plane_from_reference(r->xmin, r->ymin, intent->reference_z_mm,
            0, intent->slope_ppm, &geometry->planes[0].equation);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        code = plane_from_reference(r->xmin, r->ymax, intent->reference_z_mm,
            0, -intent->slope_ppm, &geometry->planes[1].equation);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;

        RoofPrototypeRational xs[4] = {
            rational_integer(r->xmin), rational_integer(r->xmax),
            rational_integer(r->xmax), rational_integer(r->xmin)};
        RoofPrototypeRational ys0[4] = {
            rational_integer(r->ymin), rational_integer(r->ymin), mid, mid};
        RoofPrototypeRational ys1[4] = {
            mid, mid, rational_integer(r->ymax), rational_integer(r->ymax)};
        for (size_t i = 0; i < 4; ++i)
        {
            code = point_from_xy(&geometry->planes[0].equation, xs[i], ys0[i],
                &geometry->planes[0].vertices[i]);
            if (code != ROOF_PROTOTYPE_SUCCESS) return code;
            code = point_from_xy(&geometry->planes[1].equation, xs[i], ys1[i],
                &geometry->planes[1].vertices[i]);
            if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        }
        code = point_from_xy(&geometry->planes[0].equation,
            rational_integer(r->xmin), mid, &ridge_start);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        code = point_from_xy(&geometry->planes[0].equation,
            rational_integer(r->xmax), mid, &ridge_end);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;

        geometry->boundaries[0].kind = ROOF_PROTOTYPE_BOUNDARY_EAVE;
        geometry->boundaries[1].kind = ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE;
        geometry->boundaries[2].kind = ROOF_PROTOTYPE_BOUNDARY_EAVE;
        geometry->boundaries[3].kind = ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE;
    }
    else /* ridge parallel Y */
    {
        code = rational_half_sum(r->xmin, r->xmax, &mid);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        code = plane_from_reference(r->xmin, r->ymin, intent->reference_z_mm,
            intent->slope_ppm, 0, &geometry->planes[0].equation);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        code = plane_from_reference(r->xmax, r->ymin, intent->reference_z_mm,
            -intent->slope_ppm, 0, &geometry->planes[1].equation);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;

        RoofPrototypeRational xs0[4] = {
            rational_integer(r->xmin), mid, mid, rational_integer(r->xmin)};
        RoofPrototypeRational xs1[4] = {
            mid, rational_integer(r->xmax), rational_integer(r->xmax), mid};
        RoofPrototypeRational ys[4] = {
            rational_integer(r->ymin), rational_integer(r->ymin),
            rational_integer(r->ymax), rational_integer(r->ymax)};
        for (size_t i = 0; i < 4; ++i)
        {
            code = point_from_xy(&geometry->planes[0].equation, xs0[i], ys[i],
                &geometry->planes[0].vertices[i]);
            if (code != ROOF_PROTOTYPE_SUCCESS) return code;
            code = point_from_xy(&geometry->planes[1].equation, xs1[i], ys[i],
                &geometry->planes[1].vertices[i]);
            if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        }
        code = point_from_xy(&geometry->planes[0].equation, mid,
            rational_integer(r->ymin), &ridge_start);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;
        code = point_from_xy(&geometry->planes[0].equation, mid,
            rational_integer(r->ymax), &ridge_end);
        if (code != ROOF_PROTOTYPE_SUCCESS) return code;

        geometry->boundaries[0].kind = ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE;
        geometry->boundaries[1].kind = ROOF_PROTOTYPE_BOUNDARY_EAVE;
        geometry->boundaries[2].kind = ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE;
        geometry->boundaries[3].kind = ROOF_PROTOTYPE_BOUNDARY_EAVE;
    }

    geometry->interior_edges[0] = (RoofPrototypeInteriorEdge){
        ridge_start, ridge_end, ROOF_PROTOTYPE_INTERIOR_RIDGE};
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode hip_ridge_coordinates(const RoofPrototypeRect *r,
    RoofPrototypeRational *x1, RoofPrototypeRational *y1,
    RoofPrototypeRational *x2, RoofPrototypeRational *y2)
{
    int64_t width = (int64_t)r->xmax - r->xmin;
    int64_t height = (int64_t)r->ymax - r->ymin;
    RoofPrototypeCode code;
    if (width >= height)
    {
        int64_t left2, right2;
        if (!checked_add_i64((int64_t)r->xmin * 2, height, &left2) ||
            !checked_sub_i64((int64_t)r->xmax * 2, height, &right2))
            return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
        code = rational_make(left2, 2, x1); if (code) return code;
        code = rational_half_sum(r->ymin, r->ymax, y1); if (code) return code;
        code = rational_make(right2, 2, x2); if (code) return code;
        *y2 = *y1;
    }
    else
    {
        int64_t low2, high2;
        code = rational_half_sum(r->xmin, r->xmax, x1); if (code) return code;
        if (!checked_add_i64((int64_t)r->ymin * 2, width, &low2) ||
            !checked_sub_i64((int64_t)r->ymax * 2, width, &high2))
            return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
        code = rational_make(low2, 2, y1); if (code) return code;
        *x2 = *x1;
        code = rational_make(high2, 2, y2); if (code) return code;
    }
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode fill_plane_points(RoofPrototypePlane *plane,
    const RoofPrototypeRational *xs, const RoofPrototypeRational *ys, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        RoofPrototypeCode code = point_from_xy(&plane->equation, xs[i], ys[i],
            &plane->vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS)
            return code;
    }
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode build_hip(const RoofPrototypeIntent *intent,
    const RoofPrototypeRect *r, RoofPrototypeGeometry *geometry)
{
    int64_t width = (int64_t)r->xmax - r->xmin;
    int64_t height = (int64_t)r->ymax - r->ymin;
    size_t plane_counts[4];
    if (width == height)
        plane_counts[0] = plane_counts[1] = plane_counts[2] = plane_counts[3] = 3;
    else if (width > height)
    {
        plane_counts[0] = 4; plane_counts[1] = 4;
        plane_counts[2] = 3; plane_counts[3] = 3;
    }
    else
    {
        plane_counts[0] = 3; plane_counts[1] = 3;
        plane_counts[2] = 4; plane_counts[3] = 4;
    }

    size_t ridge_count = width == height ? 0 : 1;
    RoofPrototypeCode code = geometry_allocate(geometry, 4, plane_counts, 4,
        4 + ridge_count);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    set_canonical_boundaries(r, geometry->boundaries);
    for (size_t i = 0; i < 4; ++i)
        geometry->boundaries[i].kind = ROOF_PROTOTYPE_BOUNDARY_EAVE;

    code = plane_from_reference(r->xmin, r->ymin, intent->reference_z_mm,
        0, intent->slope_ppm, &geometry->planes[0].equation); /* south */
    if (code) return code;
    code = plane_from_reference(r->xmin, r->ymax, intent->reference_z_mm,
        0, -intent->slope_ppm, &geometry->planes[1].equation); /* north */
    if (code) return code;
    code = plane_from_reference(r->xmin, r->ymin, intent->reference_z_mm,
        intent->slope_ppm, 0, &geometry->planes[2].equation); /* west */
    if (code) return code;
    code = plane_from_reference(r->xmax, r->ymin, intent->reference_z_mm,
        -intent->slope_ppm, 0, &geometry->planes[3].equation); /* east */
    if (code) return code;

    RoofPrototypeRational x1, y1, x2, y2;
    code = hip_ridge_coordinates(r, &x1, &y1, &x2, &y2);
    if (code) return code;

    RoofPrototypeRational xmin = rational_integer(r->xmin), xmax = rational_integer(r->xmax);
    RoofPrototypeRational ymin = rational_integer(r->ymin), ymax = rational_integer(r->ymax);

    if (width == height)
    {
        RoofPrototypeRational sx[3] = {xmin, xmax, x1};
        RoofPrototypeRational sy[3] = {ymin, ymin, y1};
        RoofPrototypeRational nx[3] = {xmin, x1, xmax};
        RoofPrototypeRational ny[3] = {ymax, y1, ymax};
        RoofPrototypeRational wx[3] = {xmin, x1, xmin};
        RoofPrototypeRational wy[3] = {ymin, y1, ymax};
        RoofPrototypeRational ex[3] = {xmax, xmax, x1};
        RoofPrototypeRational ey[3] = {ymin, ymax, y1};
        code = fill_plane_points(&geometry->planes[0], sx, sy, 3); if (code) return code;
        code = fill_plane_points(&geometry->planes[1], nx, ny, 3); if (code) return code;
        code = fill_plane_points(&geometry->planes[2], wx, wy, 3); if (code) return code;
        code = fill_plane_points(&geometry->planes[3], ex, ey, 3); if (code) return code;
    }
    else if (width > height)
    {
        RoofPrototypeRational sx[4] = {xmin, xmax, x2, x1};
        RoofPrototypeRational sy[4] = {ymin, ymin, y1, y1};
        RoofPrototypeRational nx[4] = {xmin, x1, x2, xmax};
        RoofPrototypeRational ny[4] = {ymax, y1, y1, ymax};
        RoofPrototypeRational wx[3] = {xmin, x1, xmin};
        RoofPrototypeRational wy[3] = {ymin, y1, ymax};
        RoofPrototypeRational ex[3] = {xmax, xmax, x2};
        RoofPrototypeRational ey[3] = {ymin, ymax, y1};
        code = fill_plane_points(&geometry->planes[0], sx, sy, 4); if (code) return code;
        code = fill_plane_points(&geometry->planes[1], nx, ny, 4); if (code) return code;
        code = fill_plane_points(&geometry->planes[2], wx, wy, 3); if (code) return code;
        code = fill_plane_points(&geometry->planes[3], ex, ey, 3); if (code) return code;
    }
    else
    {
        RoofPrototypeRational sx[3] = {xmin, xmax, x1};
        RoofPrototypeRational sy[3] = {ymin, ymin, y1};
        RoofPrototypeRational nx[3] = {xmin, x2, xmax};
        RoofPrototypeRational ny[3] = {ymax, y2, ymax};
        RoofPrototypeRational wx[4] = {xmin, x1, x2, xmin};
        RoofPrototypeRational wy[4] = {ymin, y1, y2, ymax};
        RoofPrototypeRational ex[4] = {xmax, xmax, x2, x1};
        RoofPrototypeRational ey[4] = {ymin, ymax, y2, y1};
        code = fill_plane_points(&geometry->planes[0], sx, sy, 3); if (code) return code;
        code = fill_plane_points(&geometry->planes[1], nx, ny, 3); if (code) return code;
        code = fill_plane_points(&geometry->planes[2], wx, wy, 4); if (code) return code;
        code = fill_plane_points(&geometry->planes[3], ex, ey, 4); if (code) return code;
    }

    RoofPrototypePoint3 ridge1, ridge2;
    code = point_from_xy(&geometry->planes[0].equation, x1, y1, &ridge1); if (code) return code;
    code = point_from_xy(&geometry->planes[0].equation, x2, y2, &ridge2); if (code) return code;

    size_t edge_index = 0;
    if (!point_plan_equal(ridge1, ridge2))
    {
        geometry->interior_edges[edge_index++] = (RoofPrototypeInteriorEdge){
            ridge1, ridge2, ROOF_PROTOTYPE_INTERIOR_RIDGE};
    }

    RoofPrototypePoint3 corners[4];
    code = point_from_integer_xy(&geometry->planes[0].equation, r->xmin, r->ymin, &corners[0]); if (code) return code;
    code = point_from_integer_xy(&geometry->planes[0].equation, r->xmax, r->ymin, &corners[1]); if (code) return code;
    code = point_from_integer_xy(&geometry->planes[1].equation, r->xmax, r->ymax, &corners[2]); if (code) return code;
    code = point_from_integer_xy(&geometry->planes[1].equation, r->xmin, r->ymax, &corners[3]); if (code) return code;

    geometry->interior_edges[edge_index++] = (RoofPrototypeInteriorEdge){corners[0], ridge1, ROOF_PROTOTYPE_INTERIOR_HIP};
    geometry->interior_edges[edge_index++] = (RoofPrototypeInteriorEdge){corners[3], ridge1, ROOF_PROTOTYPE_INTERIOR_HIP};
    geometry->interior_edges[edge_index++] = (RoofPrototypeInteriorEdge){corners[1], ridge2, ROOF_PROTOTYPE_INTERIOR_HIP};
    geometry->interior_edges[edge_index++] = (RoofPrototypeInteriorEdge){corners[2], ridge2, ROOF_PROTOTYPE_INTERIOR_HIP};
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode build_single_slope(const RoofPrototypeIntent *intent,
    const RoofPrototypeRect *r, RoofPrototypeGeometry *geometry)
{
    const size_t vertex_counts[1] = {4};
    RoofPrototypeCode code = geometry_allocate(geometry, 1, vertex_counts, 4, 0);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    set_canonical_boundaries(r, geometry->boundaries);

    int ref_x = r->xmin, ref_y = r->ymin;
    int64_t gx = 0, gy = 0;
    const int high_ref = intent->single_slope_reference ==
        ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE;
    if (intent->direction.x == -1) {
        ref_x = high_ref ? r->xmax : r->xmin; gx = intent->slope_ppm;
    }
    if (intent->direction.x ==  1) {
        ref_x = high_ref ? r->xmin : r->xmax; gx = -intent->slope_ppm;
    }
    if (intent->direction.y == -1) {
        ref_y = high_ref ? r->ymax : r->ymin; gy = intent->slope_ppm;
    }
    if (intent->direction.y ==  1) {
        ref_y = high_ref ? r->ymin : r->ymax; gy = -intent->slope_ppm;
    }

    code = plane_from_reference(ref_x, ref_y, intent->reference_z_mm,
        gx, gy, &geometry->planes[0].equation);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;

    RoofPrototypeRational xs[4] = {
        rational_integer(r->xmin), rational_integer(r->xmax),
        rational_integer(r->xmax), rational_integer(r->xmin)};
    RoofPrototypeRational ys[4] = {
        rational_integer(r->ymin), rational_integer(r->ymin),
        rational_integer(r->ymax), rational_integer(r->ymax)};
    code = fill_plane_points(&geometry->planes[0], xs, ys, 4);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;

    /* Canonical boundary order: south/east/north/west. */
    for (size_t i = 0; i < 4; ++i)
        geometry->boundaries[i].kind = ROOF_PROTOTYPE_BOUNDARY_SIDE_VERGE;
    if (intent->direction.y == -1)
    {
        geometry->boundaries[0].kind = ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE;
        geometry->boundaries[2].kind = ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE;
    }
    else if (intent->direction.y == 1)
    {
        geometry->boundaries[2].kind = ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE;
        geometry->boundaries[0].kind = ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE;
    }
    else if (intent->direction.x == -1)
    {
        geometry->boundaries[3].kind = ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE;
        geometry->boundaries[1].kind = ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE;
    }
    else
    {
        geometry->boundaries[1].kind = ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE;
        geometry->boundaries[3].kind = ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE;
    }
    return ROOF_PROTOTYPE_SUCCESS;
}

RoofPrototypeCode roof_prototype_build(const RoofPrototypeIntent *intent,
    RoofPrototypeGeometry *output)
{
    RoofPrototypeRect rect;
    RoofPrototypeGeometry candidate = {0};
    if (!output)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    RoofPrototypeCode code = roof_prototype_validate_intent(intent);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    code = support_rect(intent, &rect);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;

    switch (intent->generation)
    {
        case ROOF_PROTOTYPE_OPPOSING_SLOPES:
            code = build_gable(intent, &rect, &candidate);
            break;
        case ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES:
            code = build_hip(intent, &rect, &candidate);
            break;
        case ROOF_PROTOTYPE_SINGLE_SLOPE:
            code = build_single_slope(intent, &rect, &candidate);
            break;
        default:
            code = ROOF_PROTOTYPE_INVALID_ARGUMENT;
            break;
    }

    if (code != ROOF_PROTOTYPE_SUCCESS)
    {
        roof_prototype_geometry_destroy(&candidate);
        return code;
    }

    roof_prototype_geometry_destroy(output);
    *output = candidate;
    return ROOF_PROTOTYPE_SUCCESS;
}

void roof_prototype_geometry_destroy(RoofPrototypeGeometry *geometry)
{
    if (!geometry)
        return;
    if (geometry->planes)
    {
        for (size_t i = 0; i < geometry->plane_count; ++i)
            free(geometry->planes[i].vertices);
    }
    free(geometry->planes);
    free(geometry->boundaries);
    free(geometry->interior_edges);
    free(geometry->interfaces);
    memset(geometry, 0, sizeof *geometry);
}
