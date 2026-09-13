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
            int64_t a, b, c, d;
            if (!cross(v[i], v[next], v[j], &a) || !cross(v[i], v[next], v[end], &b) ||
                !cross(v[j], v[end], v[i], &c) || !cross(v[j], v[end], v[next], &d)) {
                return SLAB_NUMERIC_OVERFLOW;
            }
            if ((opposite(a, b) && opposite(c, d)) ||
                (a == 0 && in_bounds(v[i], v[next], v[j])) ||
                (b == 0 && in_bounds(v[i], v[next], v[end])) ||
                (c == 0 && in_bounds(v[j], v[end], v[i])) ||
                (d == 0 && in_bounds(v[j], v[end], v[next]))) {
                return SLAB_SELF_INTERSECTION;
            }
        }
    }
    *area2 = sum < 0 ? (uint64_t)(-(sum + 1)) + 1 : (uint64_t)sum;
    return SLAB_SUCCESS;
}

static SlabCode definition_area(const SlabDefinition *definition, uint64_t *area2)
{
    if (definition == NULL) { return SLAB_INVALID_ARGUMENT; }
    const SlabOutline *o = &definition->outline;
    if (o->vertex_count > o->vertex_capacity ||
        (o->vertex_capacity == 0 && o->vertices != NULL) || (o->vertex_capacity != 0 && o->vertices == NULL)) {
        return SLAB_INVALID_OUTLINE;
    }
    if (o->vertex_capacity > SIZE_MAX / sizeof *o->vertices) { return SLAB_NUMERIC_OVERFLOW; }
    if (definition->thickness_mm <= 0) { return SLAB_INVALID_THICKNESS; }
    return outline_area(o->vertices, o->vertex_count, area2);
}

SlabCode slab_definition_validate(const SlabDefinition *definition)
{
    uint64_t area2;
    return definition_area(definition, &area2);
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
    SlabCode code = definition_area(definition, &candidate.area2_mm2);
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
