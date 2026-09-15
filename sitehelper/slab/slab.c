#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"

static int add_checked(int64_t a, int64_t b, int64_t *out)
{
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) { return 0; }
    *out = a + b;
    return 1;
}

static int multiply_checked(int64_t a, int64_t b, int64_t *out)
{
    uint64_t x = a < 0 ? (uint64_t)(-(a + 1)) + 1 : (uint64_t)a;
    uint64_t y = b < 0 ? (uint64_t)(-(b + 1)) + 1 : (uint64_t)b;
    if (x != 0 && y > (uint64_t)INT64_MAX / x) { return 0; }
    int64_t magnitude = (int64_t)(x * y);
    *out = (a < 0) != (b < 0) ? -magnitude : magnitude;
    return 1;
}

static int cross(PlanPosition a, PlanPosition b, PlanPosition c, int64_t *out)
{
    int64_t p, q;
    return multiply_checked((int64_t)b.x - a.x, (int64_t)c.y - a.y, &p) &&
        multiply_checked((int64_t)b.y - a.y, (int64_t)c.x - a.x, &q) && add_checked(p, -q, out);
}

static int same(PlanPosition a, PlanPosition b) { return a.x == b.x && a.y == b.y; }
static int between(int a, int b, int p) { return (a <= p && p <= b) || (b <= p && p <= a); }
static int in_bounds(PlanPosition a, PlanPosition b, PlanPosition p)
{
    return between(a.x, b.x, p.x) && between(a.y, b.y, p.y);
}
static int opposite(int64_t a, int64_t b) { return (a < 0 && b > 0) || (a > 0 && b < 0); }

/* Inclusive intersection: crossing, touching and shared edges all count. */
static SlabCode edges_intersect(PlanPosition a, PlanPosition b, PlanPosition c,
    PlanPosition d, int *intersects)
{
    int64_t ab_c, ab_d, cd_a, cd_b;
    if (!cross(a,b,c,&ab_c) || !cross(a,b,d,&ab_d) ||
        !cross(c,d,a,&cd_a) || !cross(c,d,b,&cd_b)) { return SLAB_NUMERIC_OVERFLOW; }
    *intersects = (opposite(ab_c,ab_d) && opposite(cd_a,cd_b)) ||
        (ab_c == 0 && in_bounds(a,b,c)) || (ab_d == 0 && in_bounds(a,b,d)) ||
        (cd_a == 0 && in_bounds(c,d,a)) || (cd_b == 0 && in_bounds(c,d,b));
    return SLAB_SUCCESS;
}

static SlabCode signed_outline_area(const PlanPosition *v, size_t n, int64_t *output)
{
    /* Translate shoelace triangles to vertex 0 to avoid large absolute-origin
     * products for small outlines located at extreme plan coordinates. */
    int64_t sum = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        int64_t triangle;
        if (!cross(v[0], v[i], v[i + 1], &triangle) || !add_checked(sum, triangle, &sum)) {
            return SLAB_NUMERIC_OVERFLOW;
        }
    }
    *output = sum;
    return SLAB_SUCCESS;
}

static SlabCode outline_area(const PlanPosition *v, size_t n, uint64_t *area2)
{
    if (n > SIZE_MAX / sizeof *v) { return SLAB_NUMERIC_OVERFLOW; }
    if (v == NULL || n < 3) { return SLAB_INVALID_OUTLINE; }
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (same(v[i], v[j])) { return SLAB_INVALID_OUTLINE; }
        }
    }
    int64_t sum;
    SlabCode area_code = signed_outline_area(v,n,&sum);
    if (area_code != SLAB_SUCCESS) { return area_code; }
    if (sum == 0) { return SLAB_INVALID_OUTLINE; }
    for (size_t i = 0; i < n; i++) {
        size_t next = i + 1 == n ? 0 : i + 1;
        size_t after = next + 1 == n ? 0 : next + 1;
        int64_t turn;
        if (!cross(v[i], v[next], v[after], &turn)) { return SLAB_NUMERIC_OVERFLOW; }
        if (turn == 0 && (in_bounds(v[i], v[next], v[after]) || in_bounds(v[next], v[after], v[i]))) {
            return SLAB_SELF_INTERSECTION;
        }
        for (size_t j = i + 1; j < n; j++) {
            size_t end = j + 1 == n ? 0 : j + 1;
            if (next == j || end == i) { continue; } /* Adjacent pair handled above. */
            int intersects;
            SlabCode code = edges_intersect(v[i],v[next],v[j],v[end],&intersects);
            if (code != SLAB_SUCCESS) { return code; }
            if (intersects) { return SLAB_SELF_INTERSECTION; }
        }
    }
    *area2 = sum < 0 ? (uint64_t)(-(sum + 1)) + 1 : (uint64_t)sum;
    return SLAB_SUCCESS;
}

static SlabCode outline_metadata(const SlabOutline *o)
{
    if (o->vertex_count > o->vertex_capacity ||
        (o->vertex_capacity == 0 && o->vertices != NULL) ||
        (o->vertex_capacity != 0 && o->vertices == NULL)) { return SLAB_INVALID_OUTLINE; }
    return o->vertex_capacity > SIZE_MAX / sizeof *o->vertices ? SLAB_NUMERIC_OVERFLOW : SLAB_SUCCESS;
}

static SlabCode penetration_metadata(const SlabPenetrationCollection *p)
{
    if (p->count > p->capacity || (p->capacity == 0 && p->items != NULL) ||
        (p->capacity != 0 && p->items == NULL)) { return SLAB_INVALID_PENETRATION_COLLECTION; }
    return p->capacity > SIZE_MAX / sizeof *p->items ? SLAB_NUMERIC_OVERFLOW : SLAB_SUCCESS;
}

static SlabCode penetration_area(const SlabPenetration *p, uint64_t *area2)
{
    SlabCode code = outline_metadata(&p->outline);
    if (code == SLAB_INVALID_OUTLINE) { return SLAB_INVALID_PENETRATION_OUTLINE_COLLECTION; }
    if (code != SLAB_SUCCESS) { return code; }
    code = outline_area(p->outline.vertices,p->outline.vertex_count,area2);
    return code == SLAB_INVALID_OUTLINE || code == SLAB_SELF_INTERSECTION ? SLAB_INVALID_PENETRATION_OUTLINE : code;
}

SlabCode slab_penetration_validate(const SlabPenetration *penetration)
{
    if (penetration == NULL) { return SLAB_INVALID_ARGUMENT; }
    uint64_t area2;
    return penetration_area(penetration,&area2);
}

typedef enum { POINT_OUTSIDE, POINT_INSIDE, POINT_BOUNDARY } PointLocation;

/* Half-open horizontal ray parity. Cross-product signs replace division, and
 * boundary classification is explicit. Works in either winding. */
static SlabCode point_location(const SlabOutline *o, PlanPosition p, PointLocation *location)
{
    int inside = 0;
    for (size_t i = 0; i < o->vertex_count; i++) {
        PlanPosition a = o->vertices[i], b = o->vertices[(i+1) % o->vertex_count];
        int64_t turn;
        if (!cross(a,b,p,&turn)) { return SLAB_NUMERIC_OVERFLOW; }
        if (turn == 0 && in_bounds(a,b,p)) { *location = POINT_BOUNDARY; return SLAB_SUCCESS; }
        if ((a.y > p.y) != (b.y > p.y) && ((b.y > a.y && turn > 0) || (b.y < a.y && turn < 0))) {
            inside = !inside;
        }
    }
    *location = inside ? POINT_INSIDE : POINT_OUTSIDE;
    return SLAB_SUCCESS;
}

/* Once every boundary-edge pair is disjoint, one point determines containment
 * for simple polygons. This edge pass is essential for concave boundaries. */
static SlabCode boundaries_intersect(const SlabOutline *a, const SlabOutline *b, int *intersects)
{
    *intersects = 0;
    for (size_t i = 0; i < a->vertex_count; i++) {
        for (size_t j = 0; j < b->vertex_count; j++) {
            SlabCode code = edges_intersect(a->vertices[i],a->vertices[(i+1)%a->vertex_count],
                b->vertices[j],b->vertices[(j+1)%b->vertex_count],intersects);
            if (code != SLAB_SUCCESS || *intersects) { return code; }
        }
    }
    return SLAB_SUCCESS;
}

static SlabCode penetration_relationships(const SlabDefinition *d, const SlabOutline *outline, size_t prior_count)
{
    int intersects;
    SlabCode code = boundaries_intersect(&d->outline,outline,&intersects);
    if (code != SLAB_SUCCESS) { return code; }
    if (intersects) { return SLAB_PENETRATION_OUTSIDE; }
    PointLocation location;
    code = point_location(&d->outline,outline->vertices[0],&location);
    if (code != SLAB_SUCCESS) { return code; }
    if (location != POINT_INSIDE) { return SLAB_PENETRATION_OUTSIDE; }
    for (size_t i = 0; i < prior_count; i++) {
        const SlabOutline *other = &d->penetrations.items[i].outline;
        code = boundaries_intersect(other,outline,&intersects);
        if (code != SLAB_SUCCESS) { return code; }
        if (intersects) { return SLAB_PENETRATION_OVERLAP; }
        code = point_location(other,outline->vertices[0],&location);
        if (code != SLAB_SUCCESS) { return code; }
        if (location != POINT_OUTSIDE) { return SLAB_PENETRATION_OVERLAP; }
        code = point_location(outline,other->vertices[0],&location);
        if (code != SLAB_SUCCESS) { return code; }
        if (location != POINT_OUTSIDE) { return SLAB_PENETRATION_OVERLAP; }
    }
    return SLAB_SUCCESS;
}

static SlabCode region_metadata(const SlabRegionCollection *r)
{
    if (r->count > r->capacity || (r->capacity == 0 && r->items != NULL) ||
        (r->capacity != 0 && r->items == NULL)) { return SLAB_INVALID_REGION_COLLECTION; }
    return r->capacity > SIZE_MAX / sizeof *r->items ? SLAB_NUMERIC_OVERFLOW : SLAB_SUCCESS;
}

static SlabCode region_area(const SlabRegion *r, uint64_t *area)
{
    SlabCode code = outline_metadata(&r->outline);
    if (code == SLAB_INVALID_OUTLINE) { return SLAB_INVALID_REGION_OUTLINE_COLLECTION; }
    if (code != SLAB_SUCCESS) { return code; }
    if (r->thickness_mm <= 0) { return SLAB_INVALID_REGION_THICKNESS; }
    code = outline_area(r->outline.vertices,r->outline.vertex_count,area);
    return code == SLAB_INVALID_OUTLINE || code == SLAB_SELF_INTERSECTION ? SLAB_INVALID_REGION_OUTLINE : code;
}

SlabCode slab_region_validate(const SlabRegion *region)
{
    if (region == NULL) { return SLAB_INVALID_ARGUMENT; }
    uint64_t area;
    return region_area(region,&area);
}

/* Exact location of (p+q)/2. Only signs are needed: same-sign determinants need
 * not be added (which could overflow); opposite-sign addition is always safe.
 * Doubled coordinates are used only for comparisons, not products. */
static SlabCode midpoint_location(const SlabOutline *o, PlanPosition p, PlanPosition q, PointLocation *location)
{
    int inside = 0;
    int64_t x2 = (int64_t)p.x + q.x, y2 = (int64_t)p.y + q.y;
    for (size_t i = 0; i < o->vertex_count; i++) {
        PlanPosition a = o->vertices[i], b = o->vertices[(i+1)%o->vertex_count];
        int64_t u, v;
        if (!cross(a,b,p,&u) || !cross(a,b,q,&v)) { return SLAB_NUMERIC_OVERFLOW; }
        int64_t turn = ((u > 0 && v > 0) || (u < 0 && v < 0)) ? u : u + v;
        int64_t ax = (int64_t)a.x*2, bx = (int64_t)b.x*2;
        int64_t ay = (int64_t)a.y*2, by = (int64_t)b.y*2;
        if (turn == 0 && ((ax <= x2 && x2 <= bx) || (bx <= x2 && x2 <= ax)) &&
            ((ay <= y2 && y2 <= by) || (by <= y2 && y2 <= ay))) {
            *location = POINT_BOUNDARY; return SLAB_SUCCESS;
        }
        if ((ay > y2) != (by > y2) && ((by > ay && turn > 0) || (by < ay && turn < 0))) { inside = !inside; }
    }
    *location = inside ? POINT_INSIDE : POINT_OUTSIDE;
    return SLAB_SUCCESS;
}

/* Proper edge crossings have already been excluded. Split each edge at every
 * other polygon vertex lying on it, then classify every open interval. This
 * handles T-junctions, collinear subdivisions and concave vertex crossings
 * without clipping, floating point, allocation or rational intersection points. */
static SlabCode boundary_locations(const SlabOutline *a, const SlabOutline *b, int *inside, int *outside)
{
    for (size_t i = 0; i < a->vertex_count; i++) {
        PlanPosition start = a->vertices[i], end = a->vertices[(i+1)%a->vertex_count], current = start;
        int use_x = start.x != end.x;
        while (!same(current,end)) {
            PlanPosition next = end;
            for (size_t j = 0; j < b->vertex_count; j++) {
                PlanPosition p = b->vertices[j];
                int64_t turn;
                if (!cross(start,end,p,&turn)) { return SLAB_NUMERIC_OVERFLOW; }
                int value = use_x ? p.x : p.y;
                int from = use_x ? current.x : current.y, to = use_x ? next.x : next.y;
                if (turn == 0 && value != from && between(from,to,value)) { next = p; }
            }
            PointLocation location;
            SlabCode code = midpoint_location(b,current,next,&location);
            if (code != SLAB_SUCCESS) { return code; }
            *inside |= location == POINT_INSIDE;
            *outside |= location == POINT_OUTSIDE;
            current = next;
        }
    }
    return SLAB_SUCCESS;
}

typedef struct {
    int a_inside, a_outside, b_inside, b_outside;
    int proper_crossing, shared_interior_side;
} PolygonRelation;

static SlabCode polygon_relation(const SlabOutline *a, const SlabOutline *b, PolygonRelation *r)
{
    *r = (PolygonRelation){0};
    int64_t area_a, area_b;
    SlabCode code = signed_outline_area(a->vertices,a->vertex_count,&area_a);
    if (code != SLAB_SUCCESS) { return code; }
    code = signed_outline_area(b->vertices,b->vertex_count,&area_b);
    if (code != SLAB_SUCCESS) { return code; }
    for (size_t i = 0; i < a->vertex_count; i++) {
        PlanPosition p = a->vertices[i], q = a->vertices[(i+1)%a->vertex_count];
        for (size_t j = 0; j < b->vertex_count; j++) {
            PlanPosition u = b->vertices[j], v = b->vertices[(j+1)%b->vertex_count];
            int64_t c, d, e, f;
            if (!cross(p,q,u,&c) || !cross(p,q,v,&d) || !cross(u,v,p,&e) || !cross(u,v,q,&f)) {
                return SLAB_NUMERIC_OVERFLOW;
            }
            if (opposite(c,d) && opposite(e,f)) { r->proper_crossing = 1; return SLAB_SUCCESS; }
            if (c == 0 && d == 0) {
                int p0 = p.x != q.x ? p.x : p.y, p1 = p.x != q.x ? q.x : q.y;
                int u0 = p.x != q.x ? u.x : u.y, u1 = p.x != q.x ? v.x : v.y;
                int low_a = p0 < p1 ? p0 : p1, high_a = p0 > p1 ? p0 : p1;
                int low_b = u0 < u1 ? u0 : u1, high_b = u0 > u1 ? u0 : u1;
                if (low_a < high_b && low_b < high_a) {
                    int same_direction = (p1 > p0) == (u1 > u0);
                    if (((area_a > 0) == (area_b > 0)) == same_direction) { r->shared_interior_side = 1; }
                }
            }
        }
    }
    code = boundary_locations(a,b,&r->a_inside,&r->a_outside);
    if (code != SLAB_SUCCESS) { return code; }
    return boundary_locations(b,a,&r->b_inside,&r->b_outside);
}

static int interiors_overlap(PolygonRelation r)
{
    return r.proper_crossing || r.a_inside || r.b_inside || r.shared_interior_side;
}

/* Regions must cover material: containment in a void (including coincidence)
 * is invalid. A void contained by a region is subtracted. Contact alone is legal. */
static SlabCode region_void_area(const SlabDefinition *d, const SlabOutline *region,
    uint64_t region_area2, uint64_t *void_area)
{
    *void_area = 0;
    for (size_t i = 0; i < d->penetrations.count; i++) {
        const SlabOutline *hole = &d->penetrations.items[i].outline;
        PolygonRelation relation;
        SlabCode code = polygon_relation(region,hole,&relation);
        if (code != SLAB_SUCCESS) { return code; }
        if (relation.proper_crossing) { return SLAB_REGION_PENETRATION_INTERSECTION; }
        if (!relation.a_outside) { return SLAB_REGION_PENETRATION_INTERSECTION; }
        if (!relation.b_outside) {
            uint64_t area;
            code = outline_area(hole->vertices,hole->vertex_count,&area);
            if (code != SLAB_SUCCESS) { return code; }
            if (area > UINT64_MAX - *void_area) { return SLAB_NUMERIC_OVERFLOW; }
            *void_area += area;
        } else if (interiors_overlap(relation)) { return SLAB_REGION_PENETRATION_INTERSECTION; }
    }
    return *void_area >= region_area2 ? SLAB_REGION_PENETRATION_INTERSECTION : SLAB_SUCCESS;
}

static SlabCode region_relationships(const SlabDefinition *d, const SlabRegion *region,
    uint64_t area, size_t prior_count)
{
    PolygonRelation relation;
    SlabCode code = polygon_relation(&region->outline,&d->outline,&relation);
    if (code != SLAB_SUCCESS) { return code; }
    if (relation.proper_crossing || relation.a_outside) { return SLAB_REGION_OUTSIDE; }
    uint64_t void_area;
    code = region_void_area(d,&region->outline,area,&void_area);
    if (code != SLAB_SUCCESS) { return code; }
    for (size_t i = 0; i < prior_count; i++) {
        code = polygon_relation(&region->outline,&d->regions.items[i].outline,&relation);
        if (code != SLAB_SUCCESS) { return code; }
        if (interiors_overlap(relation)) { return SLAB_REGION_OVERLAP; }
    }
    return SLAB_SUCCESS;
}

static SlabCode regions_validate(const SlabDefinition *d)
{
    SlabCode code = region_metadata(&d->regions);
    if (code != SLAB_SUCCESS) { return code; }
    for (size_t i = 0; i < d->regions.count; i++) {
        uint64_t area;
        code = region_area(&d->regions.items[i],&area);
        if (code != SLAB_SUCCESS) { return code; }
        code = region_relationships(d,&d->regions.items[i],area,i);
        if (code != SLAB_SUCCESS) { return code; }
    }
    return SLAB_SUCCESS;
}

static SlabCode edge_rebate_metadata(const SlabEdgeRebateCollection *r)
{
    if (r->count > r->capacity || (r->capacity == 0 && r->items != NULL) ||
        (r->capacity != 0 && r->items == NULL)) {
        return SLAB_INVALID_EDGE_REBATE_COLLECTION;
    }
    return r->capacity > SIZE_MAX / sizeof *r->items ?
        SLAB_NUMERIC_OVERFLOW : SLAB_SUCCESS;
}

/* Same integer local-length convention as WallPlanSegment. The endpoint remains
 * U=edge_length exactly even when a diagonal's Euclidean length is fractional. */
static SlabCode edge_local_length(const SlabOutline *outline, size_t index, int *output)
{
    if (index >= outline->vertex_count) { return SLAB_EDGE_REBATE_INVALID_EDGE; }
    PlanPosition a = outline->vertices[index];
    PlanPosition b = outline->vertices[index + 1 == outline->vertex_count ? 0 : index + 1];
    double dx = (double)b.x - a.x, dy = (double)b.y - a.y;
    double length = round(hypot(dx,dy));
    if (!isfinite(length) || length < 1.0 || length > (double)INT_MAX) {
        return SLAB_NUMERIC_OVERFLOW;
    }
    *output = (int)length;
    return SLAB_SUCCESS;
}

static SlabCode edge_rebate_basic(const SlabDefinition *d, const SlabEdgeRebate *rebate)
{
    if (rebate->edge_index >= d->outline.vertex_count) { return SLAB_EDGE_REBATE_INVALID_EDGE; }
    if (rebate->width_mm <= 0 || rebate->depth_mm <= 0) {
        return SLAB_EDGE_REBATE_INVALID_DIMENSIONS;
    }
    int edge_length;
    SlabCode code = edge_local_length(&d->outline,rebate->edge_index,&edge_length);
    if (code != SLAB_SUCCESS) { return code; }
    if (rebate->start_offset_mm < 0 || rebate->start_offset_mm >= rebate->end_offset_mm ||
        rebate->end_offset_mm > edge_length) { return SLAB_EDGE_REBATE_INVALID_INTERVAL; }
    return SLAB_SUCCESS;
}

static int rebate_intervals_overlap(const SlabEdgeRebate *a, const SlabEdgeRebate *b)
{
    return a->edge_index == b->edge_index &&
        a->start_offset_mm < b->end_offset_mm && b->start_offset_mm < a->end_offset_mm;
}

static SlabCode edge_rebates_validate(const SlabDefinition *d)
{
    SlabCode code = edge_rebate_metadata(&d->edge_rebates);
    if (code != SLAB_SUCCESS) { return code; }
    for (size_t i = 0; i < d->edge_rebates.count; i++) {
        code = edge_rebate_basic(d,&d->edge_rebates.items[i]);
        if (code != SLAB_SUCCESS) { return code; }
        for (size_t j = 0; j < i; j++) {
            if (rebate_intervals_overlap(&d->edge_rebates.items[i],&d->edge_rebates.items[j])) {
                return SLAB_EDGE_REBATE_OVERLAP;
            }
        }
    }
    return SLAB_SUCCESS;
}

static SlabCode definition_areas(const SlabDefinition *definition, uint64_t *gross, uint64_t *void_area)
{
    if (definition == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = outline_metadata(&definition->outline);
    if (code != SLAB_SUCCESS) { return code; }
    if (definition->thickness_mm <= 0) { return SLAB_INVALID_THICKNESS; }
    code = outline_area(definition->outline.vertices,definition->outline.vertex_count,gross);
    if (code != SLAB_SUCCESS) { return code; }
    code = penetration_metadata(&definition->penetrations);
    if (code != SLAB_SUCCESS) { return code; }
    *void_area = 0;
    for (size_t i = 0; i < definition->penetrations.count; i++) {
        const SlabPenetration *p = &definition->penetrations.items[i];
        uint64_t area;
        code = penetration_area(p,&area);
        if (code != SLAB_SUCCESS) { return code; }
        code = penetration_relationships(definition,&p->outline,i);
        if (code != SLAB_SUCCESS) { return code; }
        if (area > UINT64_MAX - *void_area) { return SLAB_NUMERIC_OVERFLOW; }
        *void_area += area;
    }
    /* Positive separation guarantees material remains. Also guard subtraction. */
    if (*void_area >= *gross) { return SLAB_PENETRATION_OVERLAP; }
    code = regions_validate(definition);
    return code == SLAB_SUCCESS ? edge_rebates_validate(definition) : code;
}

SlabCode slab_definition_validate(const SlabDefinition *definition)
{
    uint64_t gross, void_area;
    return definition_areas(definition,&gross,&void_area);
}

SlabCode slab_validate(const Slab *slab)
{
    if (slab == NULL) { return SLAB_INVALID_ARGUMENT; }
    if (slab->id == DOMAIN_ID_INVALID) { return SLAB_INVALID_ID; }
    return slab_definition_validate(&slab->definition);
}

static double edge_length(const SlabOutline *o, size_t i)
{
    PlanPosition a = o->vertices[i], b = o->vertices[i + 1 == o->vertex_count ? 0 : i + 1];
    return hypot((double)b.x - a.x, (double)b.y - a.y);
}

SlabCode slab_measure(const SlabDefinition *definition, SlabQuantities *output)
{
    if (output == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabQuantities candidate = {0};
    uint64_t void_area;
    SlabCode code = definition_areas(definition, &candidate.area2_mm2, &void_area);
    if (code != SLAB_SUCCESS) { return code; }
    if (candidate.area2_mm2 > UINT64_MAX / (uint64_t)definition->thickness_mm) { return SLAB_NUMERIC_OVERFLOW; }
    candidate.volume2_mm3 = candidate.area2_mm2 * (uint64_t)definition->thickness_mm;
    for (size_t i = 0; i < definition->outline.vertex_count; i++) {
        candidate.perimeter_mm += edge_length(&definition->outline, i);
    }
    if (!isfinite(candidate.perimeter_mm)) { return SLAB_NUMERIC_OVERFLOW; }
    *output = candidate;
    return SLAB_SUCCESS;
}

SlabCode slab_measure_material(const SlabDefinition *definition, SlabMaterialQuantities *output)
{
    if (output == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabMaterialQuantities q = {0};
    SlabCode code = definition_areas(definition,&q.gross_area2_mm2,&q.void_area2_mm2);
    if (code != SLAB_SUCCESS) { return code; }
    uint64_t thickness = (uint64_t)definition->thickness_mm;
    if (q.gross_area2_mm2 > UINT64_MAX / thickness) { return SLAB_NUMERIC_OVERFLOW; }
    q.net_area2_mm2 = q.gross_area2_mm2 - q.void_area2_mm2;
    /* Void/net are bounded by gross, so this check covers all products. */
    q.gross_volume2_mm3 = q.gross_area2_mm2 * thickness;
    q.void_volume2_mm3 = q.void_area2_mm2 * thickness;
    q.net_volume2_mm3 = q.net_area2_mm2 * thickness;
    *output = q;
    return SLAB_SUCCESS;
}

static SlabCode region_quantities(const SlabDefinition *d, size_t index, SlabRegionQuantities *q)
{
    const SlabRegion *region = &d->regions.items[index];
    SlabCode code = region_area(region,&q->polygon_area2_mm2);
    if (code != SLAB_SUCCESS) { return code; }
    code = region_void_area(d,&region->outline,q->polygon_area2_mm2,&q->void_area2_mm2);
    if (code != SLAB_SUCCESS) { return code; }
    q->material_area2_mm2 = q->polygon_area2_mm2 - q->void_area2_mm2;
    q->thickness_mm = region->thickness_mm;
    if (q->material_area2_mm2 > UINT64_MAX / (uint64_t)region->thickness_mm) { return SLAB_NUMERIC_OVERFLOW; }
    q->volume2_mm3 = q->material_area2_mm2 * (uint64_t)region->thickness_mm;
    return SLAB_SUCCESS;
}

SlabCode slab_region_measure(const SlabDefinition *d, size_t index, SlabRegionQuantities *output)
{
    if (d == NULL || output == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = slab_definition_validate(d);
    if (code != SLAB_SUCCESS) { return code; }
    if (index >= d->regions.count) { return SLAB_INVALID_ARGUMENT; }
    SlabRegionQuantities candidate = {0};
    code = region_quantities(d,index,&candidate);
    if (code == SLAB_SUCCESS) { *output = candidate; }
    return code;
}

SlabCode slab_measure_construction(const SlabDefinition *d, SlabConstructionQuantities *output)
{
    if (output == NULL) { return SLAB_INVALID_ARGUMENT; }
    uint64_t gross, void_area;
    SlabCode code = definition_areas(d,&gross,&void_area);
    if (code != SLAB_SUCCESS) { return code; }
    SlabConstructionQuantities q = {.net_area2_mm2 = gross - void_area};
    for (size_t i = 0; i < d->regions.count; i++) {
        SlabRegionQuantities region = {0};
        code = region_quantities(d,i,&region);
        if (code != SLAB_SUCCESS) { return code; }
        if (region.material_area2_mm2 > q.net_area2_mm2 - q.region_material_area2_mm2 ||
            region.volume2_mm3 > UINT64_MAX - q.total_volume2_mm3) { return SLAB_NUMERIC_OVERFLOW; }
        q.region_material_area2_mm2 += region.material_area2_mm2;
        q.total_volume2_mm3 += region.volume2_mm3;
    }
    q.base_material_area2_mm2 = q.net_area2_mm2 - q.region_material_area2_mm2;
    if (q.base_material_area2_mm2 > UINT64_MAX / (uint64_t)d->thickness_mm) { return SLAB_NUMERIC_OVERFLOW; }
    uint64_t base_volume = q.base_material_area2_mm2 * (uint64_t)d->thickness_mm;
    if (base_volume > UINT64_MAX - q.total_volume2_mm3) { return SLAB_NUMERIC_OVERFLOW; }
    q.total_volume2_mm3 += base_volume;
    *output = q;
    return SLAB_SUCCESS;
}

SlabCode slab_properties_at_plan_position(const Slab *slab, PlanPosition point,
    SlabPointProperties *output)
{
    if (output == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    const SlabDefinition *d = &slab->definition;
    SlabPointProperties q = {.kind=SLAB_POINT_OUTSIDE,.region_index=SIZE_MAX};
    PointLocation location;
    code = point_location(&d->outline,point,&location);
    if (code != SLAB_SUCCESS) { return code; }
    if (location != POINT_INSIDE) {
        q.kind = location == POINT_BOUNDARY ? SLAB_POINT_OUTER_BOUNDARY : SLAB_POINT_OUTSIDE;
        *output = q; return SLAB_SUCCESS;
    }
    for (size_t i = 0; i < d->penetrations.count; i++) {
        code = point_location(&d->penetrations.items[i].outline,point,&location);
        if (code != SLAB_SUCCESS) { return code; }
        if (location != POINT_OUTSIDE) {
            q.kind = location == POINT_BOUNDARY ? SLAB_POINT_PENETRATION_BOUNDARY : SLAB_POINT_PENETRATION;
            *output = q; return SLAB_SUCCESS;
        }
    }
    q.kind = SLAB_POINT_BASE;
    int top_offset = d->top_level_offset_mm;
    q.thickness_mm = d->thickness_mm;
    for (size_t i = 0; i < d->regions.count; i++) {
        code = point_location(&d->regions.items[i].outline,point,&location);
        if (code != SLAB_SUCCESS) { return code; }
        if (location == POINT_BOUNDARY) {
            *output = (SlabPointProperties){.kind=SLAB_POINT_REGION_BOUNDARY,.region_index=SIZE_MAX};
            return SLAB_SUCCESS;
        }
        if (location == POINT_INSIDE) {
            q.kind = SLAB_POINT_REGION; q.region_index = i;
            q.thickness_mm = d->regions.items[i].thickness_mm;
            top_offset = d->regions.items[i].top_level_offset_mm;
            break;
        }
    }
    q.top_level_offset_mm = top_offset;
    q.bottom_level_offset_mm = (int64_t)top_offset - q.thickness_mm;
    *output = q;
    return SLAB_SUCCESS;
}

SlabCode slab_edge_length_mm(const SlabDefinition *definition, size_t index, double *output)
{
    if (definition == NULL || output == NULL || index >= definition->outline.vertex_count) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = slab_definition_validate(definition);
    if (code != SLAB_SUCCESS) { return code; }
    *output = edge_length(&definition->outline, index);
    return SLAB_SUCCESS;
}

SlabCode slab_edge_local_length_mm(const SlabDefinition *definition, size_t index, int *output)
{
    if (definition == NULL || output == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = slab_definition_validate(definition);
    if (code != SLAB_SUCCESS) { return code; }
    int candidate;
    code = edge_local_length(&definition->outline,index,&candidate);
    if (code == SLAB_SUCCESS) { *output = candidate; }
    return code;
}

SlabCode slab_absolute_top_elevation_mm(const Slab *slab, int storey_elevation_mm, int64_t *output)
{
    if (slab == NULL || output == NULL) { return SLAB_INVALID_ARGUMENT; }
    *output = (int64_t)storey_elevation_mm + slab->definition.top_level_offset_mm;
    return SLAB_SUCCESS;
}

void slab_outline_destroy(SlabOutline *outline)
{
    if (outline == NULL) { return; }
    free(outline->vertices);
    *outline = (SlabOutline){0};
}
void slab_destroy(Slab *slab)
{
    if (slab == NULL) { return; }
    slab_outline_destroy(&slab->definition.outline);
    SlabPenetrationCollection *p = &slab->definition.penetrations;
    for (size_t i = 0; i < p->count; i++) { slab_outline_destroy(&p->items[i].outline); }
    free(p->items);
    SlabRegionCollection *r = &slab->definition.regions;
    for (size_t i = 0; i < r->count; i++) { slab_outline_destroy(&r->items[i].outline); }
    free(r->items);
    free(slab->definition.edge_rebates.items);
    *slab = (Slab){0};
}
void slab_collection_destroy(SlabCollection *collection)
{
    if (collection == NULL) { return; }
    for (size_t i = 0; i < collection->count; i++) { slab_destroy(&collection->items[i]); }
    free(collection->items);
    *collection = (SlabCollection){0};
}

SlabCode slab_build(DomainId id, const PlanPosition *vertices, size_t count,
    int thickness_mm, int top_level_offset_mm, Slab *output)
{
    if (output == NULL) { return SLAB_INVALID_ARGUMENT; }
    if (id == DOMAIN_ID_INVALID) { return SLAB_INVALID_ID; }
    if (thickness_mm <= 0) { return SLAB_INVALID_THICKNESS; }
    uint64_t area2;
    SlabCode code = outline_area(vertices, count, &area2);
    if (code != SLAB_SUCCESS) { return code; }
    Slab candidate = {.id = id, .definition = {.thickness_mm = thickness_mm, .top_level_offset_mm = top_level_offset_mm}};
    candidate.definition.outline.vertices = malloc(count * sizeof *vertices);
    if (candidate.definition.outline.vertices == NULL) { return SLAB_ALLOCATION_FAILED; }
    memcpy(candidate.definition.outline.vertices, vertices, count * sizeof *vertices);
    candidate.definition.outline.vertex_count = candidate.definition.outline.vertex_capacity = count;
    slab_destroy(output);
    *output = candidate;
    return SLAB_SUCCESS;
}

SlabCode slab_add_penetration(Slab *slab, const PlanPosition *vertices, size_t count)
{
    return slab_insert_penetration_at(slab,
        slab == NULL ? 0 : slab->definition.penetrations.count,vertices,count);
}

SlabCode slab_insert_penetration_at(Slab *slab, size_t index,
    const PlanPosition *vertices, size_t count)
{
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    SlabPenetrationCollection *p = &slab->definition.penetrations;
    if (index > p->count) { return SLAB_INVALID_ARGUMENT; }
    uint64_t area;
    code = outline_area(vertices,count,&area);
    if (code != SLAB_SUCCESS) {
        return code == SLAB_NUMERIC_OVERFLOW ? code : SLAB_INVALID_PENETRATION_OUTLINE;
    }
    /* Borrow only while validating; the committed outline is an independent copy. */
    SlabOutline outline = {(PlanPosition *)vertices,count,count};
    code = penetration_relationships(&slab->definition,&outline,p->count);
    if (code != SLAB_SUCCESS) { return code; }
    for (size_t i = 0; i < slab->definition.regions.count; i++) {
        PolygonRelation relation;
        code = polygon_relation(&slab->definition.regions.items[i].outline,&outline,&relation);
        if (code != SLAB_SUCCESS) { return code; }
        if (relation.proper_crossing || !relation.a_outside ||
            (relation.b_outside && interiors_overlap(relation))) {
            return SLAB_REGION_PENETRATION_INTERSECTION;
        }
    }
    size_t maximum = SIZE_MAX / sizeof *p->items;
    if (p->count == maximum) { return SLAB_NUMERIC_OVERFLOW; }
    PlanPosition *copy = malloc(count * sizeof *copy);
    if (copy == NULL) { return SLAB_ALLOCATION_FAILED; }
    memcpy(copy,vertices,count * sizeof *copy);
    if (p->count == p->capacity) {
        size_t grown = p->capacity == 0 ? 1 : p->capacity > maximum/2 ? maximum : p->capacity*2;
        SlabPenetration *items = realloc(p->items,grown * sizeof *items);
        if (items == NULL) { free(copy); return SLAB_ALLOCATION_FAILED; }
        p->items = items; p->capacity = grown;
    }
    if (index < p->count) {
        memmove(&p->items[index+1],&p->items[index],
            (p->count-index)*sizeof *p->items);
    }
    p->items[index] = (SlabPenetration){.outline={copy,count,count}};
    p->count++;
    return SLAB_SUCCESS;
}

SlabCode slab_remove_penetration(Slab *slab, size_t index)
{
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    SlabPenetrationCollection *p = &slab->definition.penetrations;
    if (index >= p->count) { return SLAB_INVALID_ARGUMENT; }
    slab_outline_destroy(&p->items[index].outline);
    memmove(&p->items[index],&p->items[index+1],(p->count-index-1)*sizeof *p->items);
    p->items[--p->count] = (SlabPenetration){0};
    return SLAB_SUCCESS;
}

const SlabPenetration *slab_penetration_at(const Slab *slab, size_t index)
{
    if (slab == NULL || penetration_metadata(&slab->definition.penetrations) != SLAB_SUCCESS ||
        index >= slab->definition.penetrations.count) { return NULL; }
    return &slab->definition.penetrations.items[index];
}

SlabCode slab_add_region(Slab *slab, const PlanPosition *vertices, size_t count,
    int top_level_offset_mm, int thickness_mm)
{
    return slab_insert_region_at(slab,
        slab == NULL ? 0 : slab->definition.regions.count,vertices,count,
        top_level_offset_mm,thickness_mm);
}

SlabCode slab_insert_region_at(Slab *slab, size_t index,
    const PlanPosition *vertices, size_t count, int top_level_offset_mm,
    int thickness_mm)
{
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    SlabRegionCollection *r = &slab->definition.regions;
    if (index > r->count) { return SLAB_INVALID_ARGUMENT; }
    if (count < 3 || vertices == NULL) { return SLAB_INVALID_REGION_OUTLINE; }
    SlabRegion candidate = {.outline={(PlanPosition *)vertices,count,count},
        .top_level_offset_mm=top_level_offset_mm,.thickness_mm=thickness_mm};
    uint64_t area;
    code = region_area(&candidate,&area);
    if (code != SLAB_SUCCESS) { return code; }
    code = region_relationships(&slab->definition,&candidate,area,r->count);
    if (code != SLAB_SUCCESS) { return code; }
    size_t maximum = SIZE_MAX / sizeof *r->items;
    if (r->count == maximum) { return SLAB_NUMERIC_OVERFLOW; }
    PlanPosition *copy = malloc(count * sizeof *copy);
    if (copy == NULL) { return SLAB_ALLOCATION_FAILED; }
    memcpy(copy,vertices,count * sizeof *copy);
    if (r->count == r->capacity) {
        size_t grown = r->capacity == 0 ? 1 : r->capacity > maximum/2 ? maximum : r->capacity*2;
        SlabRegion *items = realloc(r->items,grown * sizeof *items);
        if (items == NULL) { free(copy); return SLAB_ALLOCATION_FAILED; }
        r->items = items; r->capacity = grown;
    }
    candidate.outline.vertices = copy;
    if (index < r->count) {
        memmove(&r->items[index+1],&r->items[index],
            (r->count-index)*sizeof *r->items);
    }
    r->items[index] = candidate;
    r->count++;
    return SLAB_SUCCESS;
}

SlabCode slab_remove_region(Slab *slab, size_t index)
{
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    SlabRegionCollection *r = &slab->definition.regions;
    if (index >= r->count) { return SLAB_INVALID_ARGUMENT; }
    slab_outline_destroy(&r->items[index].outline);
    memmove(&r->items[index],&r->items[index+1],(r->count-index-1)*sizeof *r->items);
    r->items[--r->count] = (SlabRegion){0};
    return SLAB_SUCCESS;
}

const SlabRegion *slab_region_at(const Slab *slab, size_t index)
{
    if (slab == NULL || region_metadata(&slab->definition.regions) != SLAB_SUCCESS ||
        index >= slab->definition.regions.count) { return NULL; }
    return &slab->definition.regions.items[index];
}

SlabCode slab_edge_rebate_validate(const SlabDefinition *definition,
    const SlabEdgeRebate *rebate)
{
    if (definition == NULL || rebate == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = outline_metadata(&definition->outline);
    if (code != SLAB_SUCCESS) { return code; }
    uint64_t area;
    code = outline_area(definition->outline.vertices,definition->outline.vertex_count,&area);
    return code == SLAB_SUCCESS ? edge_rebate_basic(definition,rebate) : code;
}

SlabCode slab_add_edge_rebate(Slab *slab, size_t edge_index,
    int start_offset_mm, int end_offset_mm, int width_mm, int depth_mm)
{
    return slab_insert_edge_rebate_at(slab,
        slab == NULL ? 0 : slab->definition.edge_rebates.count,edge_index,
        start_offset_mm,end_offset_mm,width_mm,depth_mm);
}

SlabCode slab_insert_edge_rebate_at(Slab *slab, size_t index,
    size_t edge_index, int start_offset_mm, int end_offset_mm,
    int width_mm, int depth_mm)
{
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    SlabEdgeRebate candidate = {edge_index,start_offset_mm,end_offset_mm,width_mm,depth_mm};
    code = edge_rebate_basic(&slab->definition,&candidate);
    if (code != SLAB_SUCCESS) { return code; }
    SlabEdgeRebateCollection *r = &slab->definition.edge_rebates;
    if (index > r->count) { return SLAB_INVALID_ARGUMENT; }
    for (size_t i = 0; i < r->count; i++) {
        if (rebate_intervals_overlap(&candidate,&r->items[i])) { return SLAB_EDGE_REBATE_OVERLAP; }
    }
    size_t maximum = SIZE_MAX / sizeof *r->items;
    if (r->count == maximum) { return SLAB_NUMERIC_OVERFLOW; }
    if (r->count == r->capacity) {
        size_t grown = r->capacity == 0 ? 1 : r->capacity > maximum/2 ? maximum : r->capacity*2;
        SlabEdgeRebate *items = realloc(r->items,grown * sizeof *items);
        if (items == NULL) { return SLAB_ALLOCATION_FAILED; }
        r->items=items; r->capacity=grown;
    }
    if (index < r->count) {
        memmove(&r->items[index+1],&r->items[index],
            (r->count-index)*sizeof *r->items);
    }
    r->items[index]=candidate;
    r->count++;
    return SLAB_SUCCESS;
}

SlabCode slab_remove_edge_rebate(Slab *slab, size_t index)
{
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    SlabEdgeRebateCollection *r=&slab->definition.edge_rebates;
    if (index >= r->count) { return SLAB_INVALID_ARGUMENT; }
    memmove(&r->items[index],&r->items[index+1],(r->count-index-1)*sizeof *r->items);
    r->items[--r->count]=(SlabEdgeRebate){0};
    return SLAB_SUCCESS;
}

const SlabEdgeRebate *slab_edge_rebate_at(const Slab *slab, size_t index)
{
    if (slab == NULL || edge_rebate_metadata(&slab->definition.edge_rebates) != SLAB_SUCCESS ||
        index >= slab->definition.edge_rebates.count) { return NULL; }
    return &slab->definition.edge_rebates.items[index];
}

SlabCode slab_edge_rebate_length_mm(const SlabEdgeRebate *rebate, int *output)
{
    if (rebate == NULL || output == NULL) { return SLAB_INVALID_ARGUMENT; }
    if (rebate->start_offset_mm < 0 || rebate->start_offset_mm >= rebate->end_offset_mm) {
        return SLAB_EDGE_REBATE_INVALID_INTERVAL;
    }
    *output=rebate->end_offset_mm-rebate->start_offset_mm;
    return SLAB_SUCCESS;
}

SlabCode slab_set_base_properties(Slab *slab, int thickness_mm,
    int top_level_offset_mm)
{
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    int old_thickness=slab->definition.thickness_mm;
    int old_top=slab->definition.top_level_offset_mm;
    slab->definition.thickness_mm=thickness_mm;
    slab->definition.top_level_offset_mm=top_level_offset_mm;
    code=slab_validate(slab);
    if (code != SLAB_SUCCESS) {
        slab->definition.thickness_mm=old_thickness;
        slab->definition.top_level_offset_mm=old_top;
    }
    return code;
}

SlabCode slab_set_region_properties(Slab *slab, size_t index,
    int top_level_offset_mm, int thickness_mm)
{
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    if (index >= slab->definition.regions.count) { return SLAB_INVALID_ARGUMENT; }
    SlabRegion *region=&slab->definition.regions.items[index];
    int old_top=region->top_level_offset_mm;
    int old_thickness=region->thickness_mm;
    region->top_level_offset_mm=top_level_offset_mm;
    region->thickness_mm=thickness_mm;
    code=slab_validate(slab);
    if (code != SLAB_SUCCESS) {
        region->top_level_offset_mm=old_top;
        region->thickness_mm=old_thickness;
    }
    return code;
}

SlabCode slab_set_edge_rebate_properties(Slab *slab, size_t index,
    int start_offset_mm, int end_offset_mm, int width_mm, int depth_mm)
{
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    if (index >= slab->definition.edge_rebates.count) { return SLAB_INVALID_ARGUMENT; }
    SlabEdgeRebate *rebate=&slab->definition.edge_rebates.items[index];
    SlabEdgeRebate old=*rebate;
    rebate->start_offset_mm=start_offset_mm;
    rebate->end_offset_mm=end_offset_mm;
    rebate->width_mm=width_mm;
    rebate->depth_mm=depth_mm;
    code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { *rebate=old; }
    return code;
}

static SlabCode set_outline_vertex_and_validate(Slab *slab, SlabOutline *outline,
    size_t vertex_index, PlanPosition position)
{
    if (slab == NULL || outline == NULL || vertex_index >= outline->vertex_count) {
        return SLAB_INVALID_ARGUMENT;
    }
    PlanPosition old=outline->vertices[vertex_index];
    outline->vertices[vertex_index]=position;
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { outline->vertices[vertex_index]=old; }
    return code;
}

SlabCode slab_set_outline_vertex(Slab *slab, size_t vertex_index,
    PlanPosition position)
{
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    return set_outline_vertex_and_validate(slab,&slab->definition.outline,
        vertex_index,position);
}

SlabCode slab_set_penetration_vertex(Slab *slab, size_t feature_index,
    size_t vertex_index, PlanPosition position)
{
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    if (feature_index >= slab->definition.penetrations.count) {
        return SLAB_INVALID_ARGUMENT;
    }
    return set_outline_vertex_and_validate(slab,
        &slab->definition.penetrations.items[feature_index].outline,
        vertex_index,position);
}

SlabCode slab_set_region_vertex(Slab *slab, size_t feature_index,
    size_t vertex_index, PlanPosition position)
{
    SlabCode code=slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    if (feature_index >= slab->definition.regions.count) {
        return SLAB_INVALID_ARGUMENT;
    }
    return set_outline_vertex_and_validate(slab,
        &slab->definition.regions.items[feature_index].outline,
        vertex_index,position);
}

SlabCode slab_clone(const Slab *source, Slab *output)
{
    if (output == NULL) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = slab_validate(source);
    if (code != SLAB_SUCCESS) { return code; }
    Slab candidate = {0};
    const SlabDefinition *d = &source->definition;
    code = slab_build(source->id,d->outline.vertices,d->outline.vertex_count,
        d->thickness_mm,d->top_level_offset_mm,&candidate);
    if (code != SLAB_SUCCESS) { return code; }
    SlabPenetrationCollection *p = &candidate.definition.penetrations;
    if (d->penetrations.count != 0) {
        p->items = calloc(d->penetrations.count,sizeof *p->items);
        if (p->items == NULL) { slab_destroy(&candidate); return SLAB_ALLOCATION_FAILED; }
        p->capacity = d->penetrations.count;
        for (size_t i = 0; i < d->penetrations.count; i++) {
            const SlabOutline *o = &d->penetrations.items[i].outline;
            PlanPosition *copy = malloc(o->vertex_count * sizeof *copy);
            if (copy == NULL) { slab_destroy(&candidate); return SLAB_ALLOCATION_FAILED; }
            memcpy(copy,o->vertices,o->vertex_count * sizeof *copy);
            p->items[p->count++] = (SlabPenetration){.outline={copy,o->vertex_count,o->vertex_count}};
        }
    }
    SlabRegionCollection *r = &candidate.definition.regions;
    if (d->regions.count != 0) {
        r->items = calloc(d->regions.count,sizeof *r->items);
        if (r->items == NULL) { slab_destroy(&candidate); return SLAB_ALLOCATION_FAILED; }
        r->capacity = d->regions.count;
        for (size_t i = 0; i < d->regions.count; i++) {
            const SlabRegion *source_region = &d->regions.items[i];
            const SlabOutline *o = &source_region->outline;
            PlanPosition *copy = malloc(o->vertex_count * sizeof *copy);
            if (copy == NULL) { slab_destroy(&candidate); return SLAB_ALLOCATION_FAILED; }
            memcpy(copy,o->vertices,o->vertex_count * sizeof *copy);
            r->items[r->count++] = (SlabRegion){.outline={copy,o->vertex_count,o->vertex_count},
                .top_level_offset_mm=source_region->top_level_offset_mm,.thickness_mm=source_region->thickness_mm};
        }
    }
    SlabEdgeRebateCollection *rebates=&candidate.definition.edge_rebates;
    if (d->edge_rebates.count != 0) {
        rebates->items=malloc(d->edge_rebates.count * sizeof *rebates->items);
        if (rebates->items == NULL) { slab_destroy(&candidate); return SLAB_ALLOCATION_FAILED; }
        memcpy(rebates->items,d->edge_rebates.items,d->edge_rebates.count * sizeof *rebates->items);
        rebates->count=rebates->capacity=d->edge_rebates.count;
    }
    slab_destroy(output);
    *output = candidate;
    return SLAB_SUCCESS;
}

static int collection_metadata_valid(const SlabCollection *collection)
{
    return collection != NULL && collection->count <= collection->capacity &&
        collection->capacity <= SIZE_MAX / sizeof *collection->items &&
        ((collection->capacity == 0 && collection->items == NULL) ||
         (collection->capacity != 0 && collection->items != NULL));
}

Slab *slab_collection_find_by_id(SlabCollection *collection, DomainId id)
{
    if (!collection_metadata_valid(collection) || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < collection->count; i++) { if (collection->items[i].id == id) { return &collection->items[i]; } }
    return NULL;
}
const Slab *slab_collection_find_by_id_const(const SlabCollection *collection, DomainId id)
{
    if (!collection_metadata_valid(collection) || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < collection->count; i++) { if (collection->items[i].id == id) { return &collection->items[i]; } }
    return NULL;
}

SlabCode slab_collection_insert(SlabCollection *collection, Slab *candidate, size_t index)
{
    if (collection == NULL || candidate == NULL) { return SLAB_INVALID_ARGUMENT; }
    if (collection->count > collection->capacity ||
        (collection->capacity == 0 && collection->items != NULL) ||
        (collection->capacity != 0 && collection->items == NULL) || index > collection->count) { return SLAB_INVALID_COLLECTION; }
    size_t maximum = SIZE_MAX / sizeof *collection->items;
    if (collection->capacity > maximum) { return SLAB_NUMERIC_OVERFLOW; }
    SlabCode code = slab_validate(candidate);
    if (code != SLAB_SUCCESS) { return code; }
    if (slab_collection_find_by_id(collection, candidate->id)) { return SLAB_INVALID_ID; }
    if (collection->count == collection->capacity) {
        if (collection->capacity == maximum) { return SLAB_NUMERIC_OVERFLOW; }
        size_t grown = collection->capacity == 0 ? 1 : collection->capacity > maximum / 2 ? maximum : collection->capacity * 2;
        Slab *items = realloc(collection->items, grown * sizeof *items);
        if (items == NULL) { return SLAB_ALLOCATION_FAILED; }
        collection->items = items; collection->capacity = grown;
    }
    if (index < collection->count) {
        memmove(&collection->items[index + 1], &collection->items[index],
            (collection->count - index) * sizeof *collection->items);
    }
    collection->items[index] = *candidate;
    collection->count++;
    *candidate = (Slab){0};
    return SLAB_SUCCESS;
}

SlabCode slab_collection_append(SlabCollection *collection, Slab *candidate)
{
    return collection == NULL ? SLAB_INVALID_ARGUMENT :
        slab_collection_insert(collection, candidate, collection->count);
}

int slab_collection_remove_by_id(SlabCollection *collection, DomainId id)
{
    Slab *slab = slab_collection_find_by_id(collection, id);
    if (slab == NULL) { return 0; }
    size_t index = (size_t)(slab - collection->items);
    slab_destroy(slab);
    memmove(slab, slab + 1, (collection->count - index - 1) * sizeof *slab);
    collection->items[--collection->count] = (Slab){0};
    return 1;
}
