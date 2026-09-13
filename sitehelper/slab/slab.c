#include <math.h>
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

static SlabCode outline_area(const PlanPosition *v, size_t n, uint64_t *area2)
{
    if (n > SIZE_MAX / sizeof *v) { return SLAB_NUMERIC_OVERFLOW; }
    if (v == NULL || n < 3) { return SLAB_INVALID_OUTLINE; }
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            if (same(v[i], v[j])) { return SLAB_INVALID_OUTLINE; }
        }
    }
    /* Translate shoelace triangles to vertex 0 to avoid large absolute-origin
     * products for small outlines located at extreme plan coordinates. */
    int64_t sum = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        int64_t triangle;
        if (!cross(v[0], v[i], v[i + 1], &triangle) || !add_checked(sum, triangle, &sum)) {
            return SLAB_NUMERIC_OVERFLOW;
        }
    }
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
    return *void_area >= *gross ? SLAB_PENETRATION_OVERLAP : SLAB_SUCCESS;
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

SlabCode slab_edge_length_mm(const SlabDefinition *definition, size_t index, double *output)
{
    if (definition == NULL || output == NULL || index >= definition->outline.vertex_count) { return SLAB_INVALID_ARGUMENT; }
    SlabCode code = slab_definition_validate(definition);
    if (code != SLAB_SUCCESS) { return code; }
    *output = edge_length(&definition->outline, index);
    return SLAB_SUCCESS;
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
    SlabCode code = slab_validate(slab);
    if (code != SLAB_SUCCESS) { return code; }
    uint64_t area;
    code = outline_area(vertices,count,&area);
    if (code != SLAB_SUCCESS) {
        return code == SLAB_NUMERIC_OVERFLOW ? code : SLAB_INVALID_PENETRATION_OUTLINE;
    }
    /* Borrow only while validating; the committed outline is an independent copy. */
    SlabOutline outline = {(PlanPosition *)vertices,count,count};
    SlabPenetrationCollection *p = &slab->definition.penetrations;
    code = penetration_relationships(&slab->definition,&outline,p->count);
    if (code != SLAB_SUCCESS) { return code; }
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
    p->items[p->count++] = (SlabPenetration){.outline={copy,count,count}};
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

SlabCode slab_collection_append(SlabCollection *collection, Slab *candidate)
{
    if (collection == NULL || candidate == NULL) { return SLAB_INVALID_ARGUMENT; }
    if (collection->count > collection->capacity ||
        (collection->capacity == 0 && collection->items != NULL) ||
        (collection->capacity != 0 && collection->items == NULL)) { return SLAB_INVALID_COLLECTION; }
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
    collection->items[collection->count++] = *candidate;
    *candidate = (Slab){0};
    return SLAB_SUCCESS;
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
