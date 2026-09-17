#include "roof_compound_prototype.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int xmin, xmax, ymin, ymax;
} CompoundRect;

typedef struct {
    int64_t numerator;
    int64_t denominator;
} Fraction;

typedef struct {
    int main_axis_x;
    int join_high;
    int u0, u1;
    int v0, v1;
    int w0, w1;
    int wing_v0, wing_v1;
    int join_v;
    int outer_v;
    Fraction main_mid_v;
    Fraction wing_mid_u;
    Fraction junction_v;
} IntersectionLayout;

static uint64_t magnitude_i64(int64_t value)
{
    return value < 0 ? UINT64_C(0) - (uint64_t)value : (uint64_t)value;
}

static uint64_t gcd_u64(uint64_t a, uint64_t b)
{
    while (b != 0)
    {
        uint64_t r = a % b;
        a = b;
        b = r;
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
            if (a > INT64_MAX / b) return 0;
        }
        else if (b < INT64_MIN / a) return 0;
    }
    else
    {
        if (b > 0)
        {
            if (a < INT64_MIN / b) return 0;
        }
        else if (a < INT64_MAX / b) return 0;
    }
    *out = a * b;
    return 1;
}

static RoofPrototypeCode fraction_make(int64_t numerator, int64_t denominator,
    Fraction *out)
{
    if (!out || denominator == 0)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;
    if (denominator < 0)
    {
        if (denominator == INT64_MIN || numerator == INT64_MIN)
            return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
        denominator = -denominator;
        numerator = -numerator;
    }
    if (numerator == 0)
    {
        *out = (Fraction){0, 1};
        return ROOF_PROTOTYPE_SUCCESS;
    }
    uint64_t g = gcd_u64(magnitude_i64(numerator), (uint64_t)denominator);
    *out = (Fraction){numerator / (int64_t)g, denominator / (int64_t)g};
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeRational roof_fraction(Fraction f)
{
    return (RoofPrototypeRational){f.numerator, f.denominator};
}

static Fraction integer_fraction(int value)
{
    return (Fraction){value, 1};
}

static RoofPrototypeCode half_sum(int a, int b, Fraction *out)
{
    int64_t sum;
    if (!checked_add_i64(a, b, &sum))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    return fraction_make(sum, 2, out);
}

static RoofPrototypeCode rect_from_intent(const RoofPrototypeIntent *intent,
    CompoundRect *out)
{
    RoofPrototypeCode code = roof_prototype_validate_intent(intent);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    if (intent->support_vertex_count != 4)
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

    *out = (CompoundRect){xmin, xmax, ymin, ymax};
    return ROOF_PROTOTYPE_SUCCESS;
}

static int axis_is_x(RoofPrototypeDirection direction)
{
    return direction.x != 0 && direction.y == 0;
}

static int strict_inside(int low, int high, int outer_low, int outer_high)
{
    return low > outer_low && high < outer_high;
}

static int interval_contains(int low, int high, int value)
{
    return value >= low && value <= high;
}

static RoofPrototypeCode compute_junction(int join_v, int join_high,
    int wing_cross_width, int64_t main_slope, int64_t wing_slope,
    Fraction *out)
{
    int64_t rise_product, denominator, join_term, numerator;
    if (!checked_mul_i64((int64_t)wing_cross_width, wing_slope, &rise_product) ||
        !checked_mul_i64(INT64_C(2), main_slope, &denominator) ||
        !checked_mul_i64((int64_t)join_v, denominator, &join_term))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;

    if (join_high)
    {
        if (!checked_sub_i64(join_term, rise_product, &numerator))
            return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    }
    else if (!checked_add_i64(join_term, rise_product, &numerator))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;

    return fraction_make(numerator, denominator, out);
}

static RoofPrototypeCode layout_for_pair(const RoofPrototypeIntent *main,
    const CompoundRect *mr, const RoofPrototypeIntent *wing,
    const CompoundRect *wr, IntersectionLayout *layout)
{
    if (main->generation != ROOF_PROTOTYPE_OPPOSING_SLOPES ||
        wing->generation != ROOF_PROTOTYPE_OPPOSING_SLOPES ||
        axis_is_x(main->direction) == axis_is_x(wing->direction) ||
        main->reference_z_mm != wing->reference_z_mm)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    memset(layout, 0, sizeof *layout);
    layout->main_axis_x = axis_is_x(main->direction);

    if (layout->main_axis_x)
    {
        layout->u0 = mr->xmin; layout->u1 = mr->xmax;
        layout->v0 = mr->ymin; layout->v1 = mr->ymax;
        layout->w0 = wr->xmin; layout->w1 = wr->xmax;
        layout->wing_v0 = wr->ymin; layout->wing_v1 = wr->ymax;
    }
    else
    {
        layout->u0 = mr->ymin; layout->u1 = mr->ymax;
        layout->v0 = mr->xmin; layout->v1 = mr->xmax;
        layout->w0 = wr->ymin; layout->w1 = wr->ymax;
        layout->wing_v0 = wr->xmin; layout->wing_v1 = wr->xmax;
    }

    if (!strict_inside(layout->w0, layout->w1, layout->u0, layout->u1))
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    int joins_high = layout->wing_v0 < layout->v1 &&
        interval_contains(layout->wing_v0, layout->wing_v1, layout->v1) &&
        layout->wing_v1 > layout->v1;
    int joins_low = layout->wing_v0 < layout->v0 &&
        interval_contains(layout->wing_v0, layout->wing_v1, layout->v0) &&
        layout->wing_v1 > layout->v0;
    if (joins_high == joins_low)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    layout->join_high = joins_high;
    layout->join_v = joins_high ? layout->v1 : layout->v0;
    layout->outer_v = joins_high ? layout->wing_v1 : layout->wing_v0;

    RoofPrototypeCode code = half_sum(layout->v0, layout->v1,
        &layout->main_mid_v);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    code = half_sum(layout->w0, layout->w1, &layout->wing_mid_u);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    code = compute_junction(layout->join_v, layout->join_high,
        layout->w1 - layout->w0, main->slope_ppm, wing->slope_ppm,
        &layout->junction_v);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;

    /* Junction must lie between the main ridge and the joined outer edge. */
    int64_t lhs, rhs;
    if (!checked_mul_i64(layout->junction_v.numerator,
            layout->main_mid_v.denominator, &lhs) ||
        !checked_mul_i64(layout->main_mid_v.numerator,
            layout->junction_v.denominator, &rhs))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    if (layout->join_high)
    {
        if (lhs < rhs || layout->junction_v.numerator >
                (int64_t)layout->join_v * layout->junction_v.denominator)
            return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    }
    else if (lhs > rhs || layout->junction_v.numerator <
             (int64_t)layout->join_v * layout->junction_v.denominator)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    return ROOF_PROTOTYPE_SUCCESS;
}

static void uv_to_xy(const IntersectionLayout *layout, Fraction u, Fraction v,
    RoofPrototypeRational *x, RoofPrototypeRational *y)
{
    if (layout->main_axis_x)
    {
        *x = roof_fraction(u);
        *y = roof_fraction(v);
    }
    else
    {
        *x = roof_fraction(v);
        *y = roof_fraction(u);
    }
}

static RoofPrototypeCode plane_z_at(const RoofPrototypePlaneEquation *plane,
    RoofPrototypeRational x, RoofPrototypeRational y, RoofPrototypeRational *out)
{
    int64_t xy_den, a, b, c, numerator, denominator, temp;
    if (!checked_mul_i64(x.denominator, y.denominator, &xy_den) ||
        !checked_mul_i64(plane->gradient_x_ppm, x.numerator, &temp) ||
        !checked_mul_i64(temp, y.denominator, &a) ||
        !checked_mul_i64(plane->gradient_y_ppm, y.numerator, &temp) ||
        !checked_mul_i64(temp, x.denominator, &b) ||
        !checked_mul_i64(plane->offset_scaled_mm, xy_den, &c) ||
        !checked_add_i64(a, b, &numerator) ||
        !checked_add_i64(numerator, c, &numerator) ||
        !checked_mul_i64(ROOF_PROTOTYPE_SLOPE_SCALE, xy_den, &denominator))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    Fraction f;
    RoofPrototypeCode code = fraction_make(numerator, denominator, &f);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    *out = roof_fraction(f);
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode set_point_uv(const IntersectionLayout *layout,
    const RoofPrototypePlaneEquation *plane, Fraction u, Fraction v,
    RoofPrototypePoint3 *out)
{
    uv_to_xy(layout, u, v, &out->x, &out->y);
    return plane_z_at(plane, out->x, out->y, &out->z);
}

static RoofPrototypeCode allocate_geometry(RoofPrototypeGeometry *geometry)
{
    static const size_t counts[4] = {4, 7, 4, 4};
    memset(geometry, 0, sizeof *geometry);
    geometry->planes = calloc(4, sizeof *geometry->planes);
    geometry->boundaries = calloc(8, sizeof *geometry->boundaries);
    geometry->interior_edges = calloc(4, sizeof *geometry->interior_edges);
    if (!geometry->planes || !geometry->boundaries || !geometry->interior_edges)
    {
        roof_prototype_geometry_destroy(geometry);
        return ROOF_PROTOTYPE_ALLOCATION_FAILED;
    }
    geometry->plane_count = 4;
    geometry->boundary_count = 8;
    geometry->interior_edge_count = 4;
    for (size_t i = 0; i < 4; ++i)
    {
        geometry->planes[i].vertices = calloc(counts[i],
            sizeof *geometry->planes[i].vertices);
        if (!geometry->planes[i].vertices)
        {
            roof_prototype_geometry_destroy(geometry);
            return ROOF_PROTOTYPE_ALLOCATION_FAILED;
        }
        geometry->planes[i].vertex_count = counts[i];
    }
    return ROOF_PROTOTYPE_SUCCESS;
}

static PlanPosition plan_point(const IntersectionLayout *layout, int u, int v)
{
    return layout->main_axis_x ? (PlanPosition){u, v} : (PlanPosition){v, u};
}

static void set_boundary(RoofPrototypeBoundary *b, const IntersectionLayout *layout,
    int u0, int v0, int u1, int v1, RoofPrototypeBoundaryKind kind)
{
    b->start = plan_point(layout, u0, v0);
    b->end = plan_point(layout, u1, v1);
    b->kind = kind;
}

static void fill_boundaries(const IntersectionLayout *l, RoofPrototypeGeometry *g)
{
    int far_v = l->join_high ? l->v0 : l->v1;
    set_boundary(&g->boundaries[0], l, l->u0, far_v, l->u1, far_v,
        ROOF_PROTOTYPE_BOUNDARY_EAVE);
    set_boundary(&g->boundaries[1], l, l->u0, far_v, l->u0, l->join_v,
        ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE);
    set_boundary(&g->boundaries[2], l, l->u1, far_v, l->u1, l->join_v,
        ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE);
    set_boundary(&g->boundaries[3], l, l->u0, l->join_v, l->w0, l->join_v,
        ROOF_PROTOTYPE_BOUNDARY_EAVE);
    set_boundary(&g->boundaries[4], l, l->w1, l->join_v, l->u1, l->join_v,
        ROOF_PROTOTYPE_BOUNDARY_EAVE);
    set_boundary(&g->boundaries[5], l, l->w0, l->join_v, l->w0, l->outer_v,
        ROOF_PROTOTYPE_BOUNDARY_EAVE);
    set_boundary(&g->boundaries[6], l, l->w1, l->join_v, l->w1, l->outer_v,
        ROOF_PROTOTYPE_BOUNDARY_EAVE);
    set_boundary(&g->boundaries[7], l, l->w0, l->outer_v, l->w1, l->outer_v,
        ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE);
}

static RoofPrototypeCode build_pair(const RoofPrototypeIntent *main,
    const RoofPrototypeIntent *wing, const IntersectionLayout *l,
    RoofPrototypeGeometry *out)
{
    RoofPrototypeGeometry main_source = {0}, wing_source = {0}, next = {0};
    RoofPrototypeCode code = roof_prototype_build(main, &main_source);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    code = roof_prototype_build(wing, &wing_source);
    if (code != ROOF_PROTOTYPE_SUCCESS)
    {
        roof_prototype_geometry_destroy(&main_source);
        return code;
    }

    code = allocate_geometry(&next);
    if (code != ROOF_PROTOTYPE_SUCCESS) goto done;

    size_t main_near = l->join_high ? 1 : 0;
    size_t main_far = l->join_high ? 0 : 1;
    next.planes[0].equation = main_source.planes[main_far].equation;
    next.planes[1].equation = main_source.planes[main_near].equation;
    next.planes[2].equation = wing_source.planes[0].equation;
    next.planes[3].equation = wing_source.planes[1].equation;

    Fraction u0 = integer_fraction(l->u0), u1 = integer_fraction(l->u1);
    Fraction w0 = integer_fraction(l->w0), w1 = integer_fraction(l->w1);
    Fraction far_v = integer_fraction(l->join_high ? l->v0 : l->v1);
    Fraction join_v = integer_fraction(l->join_v), outer_v = integer_fraction(l->outer_v);

    /* Far main plane. */
    Fraction far_us[4] = {u0, u1, u1, u0};
    Fraction far_vs[4] = {far_v, far_v, l->main_mid_v, l->main_mid_v};
    if (!l->join_high)
    {
        far_vs[0] = l->main_mid_v; far_vs[1] = l->main_mid_v;
        far_vs[2] = far_v; far_vs[3] = far_v;
    }
    for (size_t i = 0; i < 4; ++i)
    {
        code = set_point_uv(l, &next.planes[0].equation, far_us[i], far_vs[i],
            &next.planes[0].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    }

    /* Near main plane clipped by the two valleys and wing roof. */
    Fraction near_us[7] = {u0, u1, u1, w1, l->wing_mid_u, w0, u0};
    Fraction near_vs[7] = {l->main_mid_v, l->main_mid_v, join_v, join_v,
        l->junction_v, join_v, join_v};
    if (!l->join_high)
    {
        Fraction rus[7] = {u0, w0, l->wing_mid_u, w1, u1, u1, u0};
        Fraction rvs[7] = {join_v, join_v, l->junction_v, join_v, join_v,
            l->main_mid_v, l->main_mid_v};
        memcpy(near_us, rus, sizeof near_us);
        memcpy(near_vs, rvs, sizeof near_vs);
    }
    for (size_t i = 0; i < 7; ++i)
    {
        code = set_point_uv(l, &next.planes[1].equation, near_us[i], near_vs[i],
            &next.planes[1].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    }

    Fraction wing0_us[4] = {w0, l->wing_mid_u, l->wing_mid_u, w0};
    Fraction wing1_us[4] = {l->wing_mid_u, w1, w1, l->wing_mid_u};
    Fraction wing0_vs[4] = {join_v, l->junction_v, outer_v, outer_v};
    Fraction wing1_vs[4] = {l->junction_v, join_v, outer_v, outer_v};
    if (!l->join_high)
    {
        Fraction rv0[4] = {outer_v, outer_v, l->junction_v, join_v};
        Fraction rv1[4] = {outer_v, outer_v, join_v, l->junction_v};
        memcpy(wing0_vs, rv0, sizeof wing0_vs);
        memcpy(wing1_vs, rv1, sizeof wing1_vs);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        code = set_point_uv(l, &next.planes[2].equation, wing0_us[i], wing0_vs[i],
            &next.planes[2].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
        code = set_point_uv(l, &next.planes[3].equation, wing1_us[i], wing1_vs[i],
            &next.planes[3].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    }

    fill_boundaries(l, &next);

    RoofPrototypePoint3 main_ridge_start, main_ridge_end, junction, wing_ridge_end;
    code = set_point_uv(l, &next.planes[0].equation, u0, l->main_mid_v,
        &main_ridge_start); if (code) goto done;
    code = set_point_uv(l, &next.planes[0].equation, u1, l->main_mid_v,
        &main_ridge_end); if (code) goto done;
    code = set_point_uv(l, &next.planes[2].equation, l->wing_mid_u,
        l->junction_v, &junction); if (code) goto done;
    code = set_point_uv(l, &next.planes[2].equation, l->wing_mid_u,
        outer_v, &wing_ridge_end); if (code) goto done;

    RoofPrototypePoint3 valley0_start, valley1_start;
    code = set_point_uv(l, &next.planes[2].equation, w0, join_v,
        &valley0_start); if (code) goto done;
    code = set_point_uv(l, &next.planes[3].equation, w1, join_v,
        &valley1_start); if (code) goto done;

    next.interior_edges[0] = (RoofPrototypeInteriorEdge){
        main_ridge_start, main_ridge_end, ROOF_PROTOTYPE_INTERIOR_RIDGE};
    next.interior_edges[1] = (RoofPrototypeInteriorEdge){
        junction, wing_ridge_end, ROOF_PROTOTYPE_INTERIOR_RIDGE};
    next.interior_edges[2] = (RoofPrototypeInteriorEdge){
        valley0_start, junction, ROOF_PROTOTYPE_INTERIOR_VALLEY};
    next.interior_edges[3] = (RoofPrototypeInteriorEdge){
        valley1_start, junction, ROOF_PROTOTYPE_INTERIOR_VALLEY};

    roof_prototype_geometry_destroy(out);
    *out = next;
    memset(&next, 0, sizeof next);

done:
    roof_prototype_geometry_destroy(&main_source);
    roof_prototype_geometry_destroy(&wing_source);
    roof_prototype_geometry_destroy(&next);
    return code;
}


static RoofPrototypeCode termination_plane_from_reference(int x, int y, int z,
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

static RoofPrototypeCode termination_point(const RoofPrototypePlaneEquation *plane,
    int x, int y, RoofPrototypePoint3 *out)
{
    out->x = (RoofPrototypeRational){x, 1};
    out->y = (RoofPrototypeRational){y, 1};
    return plane_z_at(plane, out->x, out->y, &out->z);
}

static RoofPrototypeCode allocate_termination_geometry(RoofPrototypeGeometry *geometry)
{
    static const size_t counts[3] = {5, 5, 4};
    memset(geometry, 0, sizeof *geometry);
    geometry->planes = calloc(3, sizeof *geometry->planes);
    geometry->boundaries = calloc(4, sizeof *geometry->boundaries);
    geometry->interior_edges = calloc(4, sizeof *geometry->interior_edges);
    if (!geometry->planes || !geometry->boundaries || !geometry->interior_edges)
    {
        roof_prototype_geometry_destroy(geometry);
        return ROOF_PROTOTYPE_ALLOCATION_FAILED;
    }
    geometry->plane_count = 3;
    geometry->boundary_count = 4;
    geometry->interior_edge_count = 4;
    for (size_t i = 0; i < 3; ++i)
    {
        geometry->planes[i].vertices = calloc(counts[i],
            sizeof *geometry->planes[i].vertices);
        if (!geometry->planes[i].vertices)
        {
            roof_prototype_geometry_destroy(geometry);
            return ROOF_PROTOTYPE_ALLOCATION_FAILED;
        }
        geometry->planes[i].vertex_count = counts[i];
    }
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode build_end_slope_termination(
    const RoofPrototypeIntent *source,
    const RoofPrototypeTermination *termination,
    RoofPrototypeGeometry *out)
{
    CompoundRect r;
    RoofPrototypeCode code = rect_from_intent(source, &r);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    if (source->generation != ROOF_PROTOTYPE_OPPOSING_SLOPES ||
        !axis_is_x(source->direction) ||
        termination->end != ROOF_PROTOTYPE_END_NEGATIVE_AXIS ||
        termination->termination_offset_mm <= 0)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    int cross = r.ymax - r.ymin;
    int axis = r.xmax - r.xmin;
    int offset = termination->termination_offset_mm;
    if ((int64_t)offset * 2 >= (int64_t)cross || offset >= axis)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    int cut_x = r.xmin + offset;
    int cut_south_y = r.ymin + offset;
    int cut_north_y = r.ymax - offset;
    int64_t mid_sum;
    if (!checked_add_i64((int64_t)r.ymin, (int64_t)r.ymax, &mid_sum))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    if ((mid_sum & 1) != 0)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    int mid_y = (int)(mid_sum / 2);

    RoofPrototypeGeometry base = {0}, next = {0};
    code = roof_prototype_build(source, &base);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        return code;
    code = allocate_termination_geometry(&next);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        goto done;

    next.planes[0].equation = base.planes[0].equation; /* south */
    next.planes[1].equation = base.planes[1].equation; /* north */
    code = termination_plane_from_reference(r.xmin, r.ymin,
        source->reference_z_mm, source->slope_ppm, 0,
        &next.planes[2].equation);
    if (code != ROOF_PROTOTYPE_SUCCESS)
        goto done;

    const int south_xy[5][2] = {
        {r.xmin, r.ymin}, {r.xmax, r.ymin}, {r.xmax, mid_y},
        {cut_x, mid_y}, {cut_x, cut_south_y}
    };
    const int north_xy[5][2] = {
        {r.xmin, r.ymax}, {cut_x, cut_north_y}, {cut_x, mid_y},
        {r.xmax, mid_y}, {r.xmax, r.ymax}
    };
    const int west_xy[4][2] = {
        {r.xmin, r.ymin}, {cut_x, cut_south_y},
        {cut_x, cut_north_y}, {r.xmin, r.ymax}
    };
    for (size_t i = 0; i < 5; ++i)
    {
        code = termination_point(&next.planes[0].equation,
            south_xy[i][0], south_xy[i][1], &next.planes[0].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
        code = termination_point(&next.planes[1].equation,
            north_xy[i][0], north_xy[i][1], &next.planes[1].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    }
    for (size_t i = 0; i < 4; ++i)
    {
        code = termination_point(&next.planes[2].equation,
            west_xy[i][0], west_xy[i][1], &next.planes[2].vertices[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    }

    next.boundaries[0] = (RoofPrototypeBoundary){
        {r.xmin, r.ymin}, {r.xmax, r.ymin}, ROOF_PROTOTYPE_BOUNDARY_EAVE};
    next.boundaries[1] = (RoofPrototypeBoundary){
        {r.xmax, r.ymin}, {r.xmax, r.ymax}, ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE};
    next.boundaries[2] = (RoofPrototypeBoundary){
        {r.xmax, r.ymax}, {r.xmin, r.ymax}, ROOF_PROTOTYPE_BOUNDARY_EAVE};
    next.boundaries[3] = (RoofPrototypeBoundary){
        {r.xmin, r.ymax}, {r.xmin, r.ymin}, ROOF_PROTOTYPE_BOUNDARY_EAVE};

    RoofPrototypePoint3 ridge_start, ridge_end, hip_south_start, hip_south_end;
    RoofPrototypePoint3 hip_north_start, hip_north_end, cut_south, cut_north;
    code = termination_point(&next.planes[0].equation, cut_x, mid_y, &ridge_start);
    if (code) goto done;
    code = termination_point(&next.planes[0].equation, r.xmax, mid_y, &ridge_end);
    if (code) goto done;
    code = termination_point(&next.planes[2].equation, r.xmin, r.ymin,
        &hip_south_start); if (code) goto done;
    code = termination_point(&next.planes[2].equation, cut_x, cut_south_y,
        &hip_south_end); if (code) goto done;
    code = termination_point(&next.planes[2].equation, r.xmin, r.ymax,
        &hip_north_start); if (code) goto done;
    code = termination_point(&next.planes[2].equation, cut_x, cut_north_y,
        &hip_north_end); if (code) goto done;
    cut_south = hip_south_end;
    cut_north = hip_north_end;

    next.interior_edges[0] = (RoofPrototypeInteriorEdge){
        ridge_start, ridge_end, ROOF_PROTOTYPE_INTERIOR_RIDGE};
    next.interior_edges[1] = (RoofPrototypeInteriorEdge){
        hip_south_start, hip_south_end, ROOF_PROTOTYPE_INTERIOR_HIP};
    next.interior_edges[2] = (RoofPrototypeInteriorEdge){
        hip_north_start, hip_north_end, ROOF_PROTOTYPE_INTERIOR_HIP};
    next.interior_edges[3] = (RoofPrototypeInteriorEdge){
        cut_south, cut_north, ROOF_PROTOTYPE_INTERIOR_TERMINATION_CUT};

    roof_prototype_geometry_destroy(out);
    *out = next;
    memset(&next, 0, sizeof next);

done:
    roof_prototype_geometry_destroy(&base);
    roof_prototype_geometry_destroy(&next);
    return code;
}


static RoofPrototypeCode clone_plane(const RoofPrototypePlane *source,
    RoofPrototypePlane *target)
{
    target->equation = source->equation;
    target->vertex_count = source->vertex_count;
    if (source->vertex_count == 0)
        return ROOF_PROTOTYPE_SUCCESS;
    target->vertices = calloc(source->vertex_count, sizeof *target->vertices);
    if (!target->vertices)
        return ROOF_PROTOTYPE_ALLOCATION_FAILED;
    memcpy(target->vertices, source->vertices,
        source->vertex_count * sizeof *target->vertices);
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode combine_unclipped_pair(const RoofPrototypeGeometry *a,
    const RoofPrototypeGeometry *b, RoofPrototypeGeometry *next)
{
    memset(next, 0, sizeof *next);
    next->plane_count = a->plane_count + b->plane_count;
    next->boundary_count = a->boundary_count + b->boundary_count;
    next->interior_edge_count = a->interior_edge_count + b->interior_edge_count;
    if (next->plane_count)
        next->planes = calloc(next->plane_count, sizeof *next->planes);
    if (next->boundary_count)
        next->boundaries = calloc(next->boundary_count, sizeof *next->boundaries);
    if (next->interior_edge_count)
        next->interior_edges = calloc(next->interior_edge_count,
            sizeof *next->interior_edges);
    if ((next->plane_count && !next->planes) ||
        (next->boundary_count && !next->boundaries) ||
        (next->interior_edge_count && !next->interior_edges))
    {
        roof_prototype_geometry_destroy(next);
        return ROOF_PROTOTYPE_ALLOCATION_FAILED;
    }

    for (size_t i = 0; i < a->plane_count; ++i)
    {
        RoofPrototypeCode code = clone_plane(&a->planes[i], &next->planes[i]);
        if (code != ROOF_PROTOTYPE_SUCCESS)
        {
            roof_prototype_geometry_destroy(next);
            return code;
        }
    }
    for (size_t i = 0; i < b->plane_count; ++i)
    {
        RoofPrototypeCode code = clone_plane(&b->planes[i],
            &next->planes[a->plane_count + i]);
        if (code != ROOF_PROTOTYPE_SUCCESS)
        {
            roof_prototype_geometry_destroy(next);
            return code;
        }
    }
    if (a->boundary_count)
        memcpy(next->boundaries, a->boundaries,
            a->boundary_count * sizeof *next->boundaries);
    if (b->boundary_count)
        memcpy(next->boundaries + a->boundary_count, b->boundaries,
            b->boundary_count * sizeof *next->boundaries);
    if (a->interior_edge_count)
        memcpy(next->interior_edges, a->interior_edges,
            a->interior_edge_count * sizeof *next->interior_edges);
    if (b->interior_edge_count)
        memcpy(next->interior_edges + a->interior_edge_count, b->interior_edges,
            b->interior_edge_count * sizeof *next->interior_edges);
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode exact_point(const RoofPrototypePlaneEquation *plane,
    Fraction x, Fraction y, RoofPrototypePoint3 *out)
{
    out->x = roof_fraction(x);
    out->y = roof_fraction(y);
    return plane_z_at(plane, out->x, out->y, &out->z);
}

static RoofPrototypeCode build_step_abutment(const RoofPrototypeIntent *main,
    const CompoundRect *mr, const RoofPrototypeIntent *lean,
    const CompoundRect *lr, RoofPrototypeGeometry *output)
{
    if (main->generation != ROOF_PROTOTYPE_OPPOSING_SLOPES ||
        !axis_is_x(main->direction) ||
        lean->generation != ROOF_PROTOTYPE_SINGLE_SLOPE ||
        lean->direction.x != 0 || lean->direction.y != -1 ||
        lean->single_slope_reference != ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE ||
        lr->ymax != mr->ymin ||
        !strict_inside(lr->xmin, lr->xmax, mr->xmin, mr->xmax))
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    RoofPrototypeGeometry main_g = {0}, lean_g = {0}, next = {0};
    RoofPrototypeCode code = roof_prototype_build(main, &main_g);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    code = roof_prototype_build(lean, &lean_g);
    if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    code = combine_unclipped_pair(&main_g, &lean_g, &next);
    if (code != ROOF_PROTOTYPE_SUCCESS) goto done;

    next.interfaces = calloc(1, sizeof *next.interfaces);
    if (!next.interfaces)
    {
        code = ROOF_PROTOTYPE_ALLOCATION_FAILED;
        goto done;
    }
    next.interface_count = 1;
    next.interfaces[0].kind = ROOF_PROTOTYPE_INTERFACE_STEP_ABUTMENT;

    Fraction x0 = integer_fraction(lr->xmin), x1 = integer_fraction(lr->xmax);
    Fraction y = integer_fraction(mr->ymin);
    code = exact_point(&main_g.planes[0].equation, x0, y,
        &next.interfaces[0].first_edge.start); if (code) goto done;
    code = exact_point(&main_g.planes[0].equation, x1, y,
        &next.interfaces[0].first_edge.end); if (code) goto done;
    code = exact_point(&lean_g.planes[0].equation, x0, y,
        &next.interfaces[0].second_edge.start); if (code) goto done;
    code = exact_point(&lean_g.planes[0].equation, x1, y,
        &next.interfaces[0].second_edge.end); if (code) goto done;
    next.interfaces[0].first_edge.kind = ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM;
    next.interfaces[0].second_edge.kind = ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM;

    roof_prototype_geometry_destroy(output);
    *output = next;
    memset(&next, 0, sizeof next);

done:
    roof_prototype_geometry_destroy(&main_g);
    roof_prototype_geometry_destroy(&lean_g);
    roof_prototype_geometry_destroy(&next);
    return code;
}

static RoofPrototypeCode horizontal_plane_intersection_y(
    const RoofPrototypePlaneEquation *a, const RoofPrototypePlaneEquation *b,
    Fraction *out)
{
    if (a->gradient_x_ppm != b->gradient_x_ppm)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    int64_t denominator, numerator;
    if (!checked_sub_i64(a->gradient_y_ppm, b->gradient_y_ppm, &denominator) ||
        !checked_sub_i64(b->offset_scaled_mm, a->offset_scaled_mm, &numerator))
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    if (denominator == 0)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    return fraction_make(numerator, denominator, out);
}

static int fraction_between(Fraction value, int low, int high)
{
    int64_t low_scaled, high_scaled;
    if (!checked_mul_i64((int64_t)low, value.denominator, &low_scaled) ||
        !checked_mul_i64((int64_t)high, value.denominator, &high_scaled))
        return 0;
    return value.numerator >= low_scaled && value.numerator <= high_scaled;
}

static RoofPrototypeCode allocate_f1_geometry(RoofPrototypeGeometry *g)
{
    static const size_t counts[3] = {4, 8, 4};
    memset(g, 0, sizeof *g);
    g->planes = calloc(3, sizeof *g->planes);
    g->boundaries = calloc(6, sizeof *g->boundaries);
    g->interior_edges = calloc(2, sizeof *g->interior_edges);
    if (!g->planes || !g->boundaries || !g->interior_edges)
    {
        roof_prototype_geometry_destroy(g);
        return ROOF_PROTOTYPE_ALLOCATION_FAILED;
    }
    g->plane_count = 3;
    g->boundary_count = 6;
    g->interior_edge_count = 2;
    for (size_t i = 0; i < 3; ++i)
    {
        g->planes[i].vertices = calloc(counts[i], sizeof *g->planes[i].vertices);
        if (!g->planes[i].vertices)
        {
            roof_prototype_geometry_destroy(g);
            return ROOF_PROTOTYPE_ALLOCATION_FAILED;
        }
        g->planes[i].vertex_count = counts[i];
    }
    return ROOF_PROTOTYPE_SUCCESS;
}

static RoofPrototypeCode set_f1_point(RoofPrototypePlane *plane, size_t index,
    Fraction x, Fraction y)
{
    return exact_point(&plane->equation, x, y, &plane->vertices[index]);
}

static RoofPrototypeCode build_gable_skillion_intersection(
    const RoofPrototypeIntent *main, const CompoundRect *mr,
    const RoofPrototypeIntent *overlap, const CompoundRect *orr,
    RoofPrototypeGeometry *output)
{
    if (main->generation != ROOF_PROTOTYPE_OPPOSING_SLOPES ||
        !axis_is_x(main->direction) ||
        overlap->generation != ROOF_PROTOTYPE_SINGLE_SLOPE ||
        overlap->direction.x != 0 || overlap->direction.y != 1 ||
        overlap->single_slope_reference != ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE ||
        !strict_inside(orr->xmin, orr->xmax, mr->xmin, mr->xmax) ||
        orr->ymin <= mr->ymin || orr->ymin >= mr->ymax || orr->ymax <= mr->ymax)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    int64_t mid_sum;
    if (!checked_add_i64(mr->ymin, mr->ymax, &mid_sum) || (mid_sum & 1) != 0)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
    int mid_y = (int)(mid_sum / 2);
    if (orr->ymin <= mid_y)
        return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;

    RoofPrototypeGeometry main_g = {0}, overlap_g = {0}, next = {0};
    RoofPrototypeCode code = roof_prototype_build(main, &main_g);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    code = roof_prototype_build(overlap, &overlap_g);
    if (code != ROOF_PROTOTYPE_SUCCESS) goto done;

    Fraction seam_y;
    code = horizontal_plane_intersection_y(&main_g.planes[1].equation,
        &overlap_g.planes[0].equation, &seam_y);
    if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    if (!fraction_between(seam_y, orr->ymin, mr->ymax))
    {
        code = ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
        goto done;
    }

    code = allocate_f1_geometry(&next);
    if (code != ROOF_PROTOTYPE_SUCCESS) goto done;
    next.planes[0].equation = main_g.planes[0].equation;
    next.planes[1].equation = main_g.planes[1].equation;
    next.planes[2].equation = overlap_g.planes[0].equation;

    Fraction xmin = integer_fraction(mr->xmin), xmax = integer_fraction(mr->xmax);
    Fraction ymin = integer_fraction(mr->ymin), ymax = integer_fraction(mr->ymax);
    Fraction mid = integer_fraction(mid_y);
    Fraction ox0 = integer_fraction(orr->xmin), ox1 = integer_fraction(orr->xmax);
    Fraction oy1 = integer_fraction(orr->ymax);

    Fraction p0x[4] = {xmin, xmax, xmax, xmin};
    Fraction p0y[4] = {ymin, ymin, mid, mid};
    for (size_t i = 0; i < 4; ++i)
    {
        code = set_f1_point(&next.planes[0], i, p0x[i], p0y[i]);
        if (code) goto done;
    }
    Fraction p1x[8] = {xmin, xmax, xmax, ox1, ox1, ox0, ox0, xmin};
    Fraction p1y[8] = {mid, mid, ymax, ymax, seam_y, seam_y, ymax, ymax};
    for (size_t i = 0; i < 8; ++i)
    {
        code = set_f1_point(&next.planes[1], i, p1x[i], p1y[i]);
        if (code) goto done;
    }
    Fraction p2x[4] = {ox0, ox1, ox1, ox0};
    Fraction p2y[4] = {seam_y, seam_y, oy1, oy1};
    for (size_t i = 0; i < 4; ++i)
    {
        code = set_f1_point(&next.planes[2], i, p2x[i], p2y[i]);
        if (code) goto done;
    }

    /* Only wholly integer exterior segments are carried by the legacy 26D
     * boundary representation. The exact clipped side-verge endpoints live in
     * the plane polygons; E3 records this as a representation limitation. */
    next.boundaries[0] = (RoofPrototypeBoundary){{mr->xmin,mr->ymin},{mr->xmax,mr->ymin},ROOF_PROTOTYPE_BOUNDARY_EAVE};
    next.boundaries[1] = (RoofPrototypeBoundary){{mr->xmax,mr->ymin},{mr->xmax,mr->ymax},ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE};
    next.boundaries[2] = (RoofPrototypeBoundary){{mr->xmax,mr->ymax},{orr->xmax,mr->ymax},ROOF_PROTOTYPE_BOUNDARY_EAVE};
    next.boundaries[3] = (RoofPrototypeBoundary){{orr->xmin,mr->ymax},{mr->xmin,mr->ymax},ROOF_PROTOTYPE_BOUNDARY_EAVE};
    next.boundaries[4] = (RoofPrototypeBoundary){{mr->xmin,mr->ymax},{mr->xmin,mr->ymin},ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE};
    next.boundaries[5] = (RoofPrototypeBoundary){{orr->xmin,orr->ymax},{orr->xmax,orr->ymax},ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE};

    next.interior_edges[0] = main_g.interior_edges[0];
    code = exact_point(&next.planes[1].equation, ox0, seam_y,
        &next.interior_edges[1].start); if (code) goto done;
    code = exact_point(&next.planes[1].equation, ox1, seam_y,
        &next.interior_edges[1].end); if (code) goto done;
    next.interior_edges[1].kind = ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM;

    roof_prototype_geometry_destroy(output);
    *output = next;
    memset(&next, 0, sizeof next);

done:
    roof_prototype_geometry_destroy(&main_g);
    roof_prototype_geometry_destroy(&overlap_g);
    roof_prototype_geometry_destroy(&next);
    return code;
}

static RoofPrototypeCode build_abutment_pair(const RoofPrototypeIntent *a,
    const CompoundRect *ar, const RoofPrototypeIntent *b,
    const CompoundRect *br, RoofPrototypeGeometry *output)
{
    RoofPrototypeCode code = build_step_abutment(a, ar, b, br, output);
    if (code == ROOF_PROTOTYPE_SUCCESS || code == ROOF_PROTOTYPE_NUMERIC_OVERFLOW ||
        code == ROOF_PROTOTYPE_ALLOCATION_FAILED)
        return code;
    return build_step_abutment(b, br, a, ar, output);
}

static RoofPrototypeCode build_intersection_pair_extended(
    const RoofPrototypeIntent *a, const CompoundRect *ar,
    const RoofPrototypeIntent *b, const CompoundRect *br,
    RoofPrototypeGeometry *output)
{
    RoofPrototypeCode code = build_gable_skillion_intersection(a, ar, b, br, output);
    if (code == ROOF_PROTOTYPE_SUCCESS || code == ROOF_PROTOTYPE_NUMERIC_OVERFLOW ||
        code == ROOF_PROTOTYPE_ALLOCATION_FAILED)
        return code;
    return build_gable_skillion_intersection(b, br, a, ar, output);
}

RoofPrototypeCode roof_prototype_build_compound(
    const RoofPrototypeCompoundIntent *intent,
    RoofPrototypeGeometry *output)
{
    if (!intent || !output || !intent->portions || intent->portion_count == 0)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    /* Priority 26E2 termination mode. It is deliberately disjoint from E1's
     * pair-composition mode so source intent cannot be guessed from overlap. */
    if (intent->termination_count == 1 && intent->composition_count == 0)
    {
        if (!intent->terminations || intent->portion_count != 1)
            return ROOF_PROTOTYPE_INVALID_ARGUMENT;
        const RoofPrototypeTermination *termination = &intent->terminations[0];
        if (termination->portion >= intent->portion_count)
            return ROOF_PROTOTYPE_INVALID_ARGUMENT;
        return build_end_slope_termination(&intent->portions[termination->portion],
            termination, output);
    }

    /* Priority 26E1 perpendicular-intersection mode. */
    if (intent->termination_count != 0 || !intent->compositions ||
        intent->portion_count != 2 || intent->composition_count != 1)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    const RoofPrototypeComposition *composition = &intent->compositions[0];
    if (composition->first_portion >= intent->portion_count ||
        composition->second_portion >= intent->portion_count ||
        composition->first_portion == composition->second_portion)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    const RoofPrototypeIntent *a = &intent->portions[composition->first_portion];
    const RoofPrototypeIntent *b = &intent->portions[composition->second_portion];
    CompoundRect ar, br;
    RoofPrototypeCode code = rect_from_intent(a, &ar);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;
    code = rect_from_intent(b, &br);
    if (code != ROOF_PROTOTYPE_SUCCESS) return code;

    if (composition->kind == ROOF_PROTOTYPE_COMPOSITION_ABUTS)
        return build_abutment_pair(a, &ar, b, &br, output);
    if (composition->kind != ROOF_PROTOTYPE_COMPOSITION_INTERSECTS)
        return ROOF_PROTOTYPE_INVALID_ARGUMENT;

    /* E3 first tries the different-level gable/skillion intersection. If that
     * does not match, preserve E1's perpendicular-gable solver unchanged. */
    code = build_intersection_pair_extended(a, &ar, b, &br, output);
    if (code == ROOF_PROTOTYPE_SUCCESS || code == ROOF_PROTOTYPE_NUMERIC_OVERFLOW ||
        code == ROOF_PROTOTYPE_ALLOCATION_FAILED)
        return code;

    IntersectionLayout layout;
    code = layout_for_pair(a, &ar, b, &br, &layout);
    if (code == ROOF_PROTOTYPE_SUCCESS)
        return build_pair(a, b, &layout, output);

    RoofPrototypeCode reverse = layout_for_pair(b, &br, a, &ar, &layout);
    if (reverse == ROOF_PROTOTYPE_SUCCESS)
        return build_pair(b, a, &layout, output);

    if (code == ROOF_PROTOTYPE_NUMERIC_OVERFLOW || reverse == ROOF_PROTOTYPE_NUMERIC_OVERFLOW)
        return ROOF_PROTOTYPE_NUMERIC_OVERFLOW;
    return ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY;
}
