#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "slab.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
void *__real_malloc(size_t n);
void *__real_realloc(void *p, size_t n);
void *__real_calloc(size_t n, size_t s);
static int fail(void) { return fail_after != SIZE_MAX && fail_after-- == 0; }
void *__wrap_malloc(size_t n) { return fail() ? NULL : __real_malloc(n); }
void *__wrap_realloc(void *p, size_t n) { return fail() ? NULL : __real_realloc(p, n); }
void *__wrap_calloc(size_t n, size_t s) { return fail() ? NULL : __real_calloc(n, s); }
#endif

static const PlanPosition rectangle[] = {{0,0}, {10000,0}, {10000,8000}, {0,8000}};

static void valid_shape(const PlanPosition *v, size_t n, uint64_t area2, int thickness,
    double perimeter)
{
    Slab slab = {0};
    assert(slab_build(42, v, n, thickness, -50, &slab) == SLAB_SUCCESS);
    assert(slab_validate(&slab) == SLAB_SUCCESS);
    assert(slab.definition.outline.vertices != v);
    assert(memcmp(slab.definition.outline.vertices, v, n * sizeof *v) == 0);
    SlabQuantities q = {0};
    assert(slab_measure(&slab.definition, &q) == SLAB_SUCCESS);
    assert(q.area2_mm2 == area2 && q.volume2_mm3 == area2 * (uint64_t)thickness);
    assert(fabs(q.perimeter_mm - perimeter) < 1e-8);
    double sum = 0;
    for (size_t i = 0; i < n; i++) {
        double edge = -1;
        size_t j = (i + 1) % n;
        assert(slab_edge_length_mm(&slab.definition, i, &edge) == SLAB_SUCCESS);
        assert(fabs(edge - hypot((double)v[j].x - v[i].x, (double)v[j].y - v[i].y)) < 1e-8);
        sum += edge;
    }
    assert(fabs(sum - perimeter) < 1e-8);
    double untouched = 12;
    assert(slab_edge_length_mm(&slab.definition, n, &untouched) == SLAB_INVALID_ARGUMENT && untouched == 12);
    int64_t top;
    assert(slab_absolute_top_elevation_mm(&slab, 3000, &top) == SLAB_SUCCESS && top == 2950);
    slab.definition.top_level_offset_mm = INT_MAX;
    assert(slab_absolute_top_elevation_mm(&slab, INT_MAX, &top) == SLAB_SUCCESS && top == (int64_t)INT_MAX * 2);
    slab.definition.top_level_offset_mm = INT_MIN;
    assert(slab_absolute_top_elevation_mm(&slab, INT_MIN, &top) == SLAB_SUCCESS && top == (int64_t)INT_MIN * 2);
    /* Replacement can read vertices from the previous owned output. */
    assert(slab_build(43, slab.definition.outline.vertices, n, thickness, 0, &slab) == SLAB_SUCCESS);
    assert(slab.id == 43 && memcmp(slab.definition.outline.vertices, v, n * sizeof *v) == 0);
    slab_destroy(&slab);
    assert(!slab.id && !slab.definition.outline.vertices && !slab.definition.outline.vertex_capacity);
    slab_destroy(&slab);
}

static void test_shapes(void)
{
    valid_shape(rectangle, 4, 160000000, 100, 36000);
    const PlanPosition triangle[] = {{0,0},{3,0},{0,1}};
    valid_shape(triangle, 3, 3, 3, 4 + sqrt(10)); /* 1.5 mm², 4.5 mm³. */
    const PlanPosition concave[] = {{0,0},{4,0},{4,2},{2,2},{2,4},{0,4}};
    valid_shape(concave, 6, 24, 50, 16);
    PlanPosition reversed[6];
    for (size_t i = 0; i < 6; i++) { reversed[i] = concave[5 - i]; }
    valid_shape(reversed, 6, 24, 50, 16);
    const PlanPosition forward[] = {{0,0},{2,0},{4,0},{4,4},{0,4}};
    valid_shape(forward, 5, 32, 1, 16);
    const PlanPosition translated[] = {{INT_MAX-4,INT_MIN},{INT_MAX,INT_MIN},
        {INT_MAX,INT_MIN+4},{INT_MAX-4,INT_MIN+4}};
    valid_shape(translated, 4, 32, 1, 16);
}

static void test_invalid_and_overflow(void)
{
    const PlanPosition cases[][6] = {
        {{0,0},{4,0},{4,0},{0,4}},
        {{0,0},{4,0},{4,4},{0,0}},
        {{0,0},{4,0},{4,4},{4,0},{0,4}},
        {{0,0},{1,1},{2,2}},
        {{0,0},{4,0},{2,0},{4,4},{0,4}},
        {{0,0},{4,4},{0,4},{4,0}},
        {{0,0},{4,0},{4,4},{2,0},{0,4}},
        {{0,0},{4,3},{0,4},{5,0}}
    };
    const size_t counts[] = {4,4,5,3,5,4,5,4};
    const SlabCode codes[] = {SLAB_INVALID_OUTLINE,SLAB_INVALID_OUTLINE,SLAB_INVALID_OUTLINE,
        SLAB_INVALID_OUTLINE,SLAB_SELF_INTERSECTION,SLAB_INVALID_OUTLINE,
        SLAB_SELF_INTERSECTION,SLAB_SELF_INTERSECTION};
    Slab slab = {0};
    assert(slab_build(1, rectangle, 4, 100, 0, &slab) == SLAB_SUCCESS);
    unsigned char before[sizeof slab]; memcpy(before, &slab, sizeof slab);
    for (size_t i = 0; i < sizeof counts / sizeof *counts; i++) {
        assert(slab_build(2, cases[i], counts[i], 100, 0, &slab) == codes[i]);
        assert(memcmp(before, &slab, sizeof slab) == 0);
    }
    for (size_t n = 0; n < 3; n++) {
        assert(slab_build(2, rectangle, n, 100, 0, &slab) == SLAB_INVALID_OUTLINE);
    }
    assert(slab_build(2, NULL, 4, 100, 0, &slab) == SLAB_INVALID_OUTLINE);
    assert(slab_build(0, rectangle, 4, 100, 0, &slab) == SLAB_INVALID_ID);
    assert(slab_build(2, rectangle, 4, 0, 0, &slab) == SLAB_INVALID_THICKNESS);
    assert(slab_build(2, rectangle, 4, -1, 0, &slab) == SLAB_INVALID_THICKNESS);
    assert(slab_build(2, rectangle, SIZE_MAX, 100, 0, &slab) == SLAB_NUMERIC_OVERFLOW);
    assert(slab_build(2, rectangle, 4, 100, 0, NULL) == SLAB_INVALID_ARGUMENT);
    const PlanPosition extreme[] = {{INT_MIN,INT_MIN},{INT_MAX,INT_MIN},{INT_MAX,INT_MAX},{INT_MIN,INT_MAX}};
    assert(slab_build(2, extreme, 4, 1, 0, &slab) == SLAB_NUMERIC_OVERFLOW);
    assert(memcmp(before, &slab, sizeof slab) == 0);
    assert(memcmp(slab.definition.outline.vertices, rectangle, sizeof rectangle) == 0);
    const PlanPosition large[] = {{0,0},{INT_MAX,0},{INT_MAX,INT_MAX},{0,INT_MAX}};
    assert(slab_build(2, large, 4, INT_MAX, 0, &slab) == SLAB_SUCCESS);
    SlabQuantities q = {.area2_mm2=123, .volume2_mm3=456, .perimeter_mm=789};
    unsigned char saved[sizeof q]; memcpy(saved, &q, sizeof q);
    assert(slab_measure(&slab.definition, &q) == SLAB_NUMERIC_OVERFLOW);
    assert(memcmp(saved, &q, sizeof q) == 0);
    assert(slab_validate(NULL) == SLAB_INVALID_ARGUMENT);
    assert(slab_definition_validate(NULL) == SLAB_INVALID_ARGUMENT);
    assert(slab_measure(NULL, &q) == SLAB_INVALID_ARGUMENT);
    assert(slab_measure(&slab.definition, NULL) == SLAB_INVALID_ARGUMENT);
    slab_destroy(&slab);
}

static void test_collection(void)
{
    SlabCollection c = {0}; Slab s = {0};
    for (DomainId id = 1; id <= 4; id++) {
        assert(slab_build(id, rectangle, 4, 100, 0, &s) == SLAB_SUCCESS);
        PlanPosition *vertices = s.definition.outline.vertices;
        assert(slab_collection_append(&c, &s) == SLAB_SUCCESS);
        assert(!s.id && !s.definition.outline.vertices);
        assert(c.items[c.count-1].definition.outline.vertices == vertices);
    }
    assert(slab_collection_append(&c, &c.items[0]) == SLAB_INVALID_ID);
    assert(slab_collection_find_by_id_const(&c, 3) == &c.items[2]);
    assert(slab_collection_remove_by_id(&c, 2));
    assert(c.count == 3 && c.items[1].id == 3 && c.items[2].id == 4 && c.items[3].id == 0);
    assert(slab_collection_remove_by_id(&c, 1) && slab_collection_remove_by_id(&c, 4));
    assert(!slab_collection_remove_by_id(&c, 0) && !slab_collection_remove_by_id(&c, 2));
    assert(slab_build(5, rectangle, 4, 100, 0, &s) == SLAB_SUCCESS);
    SlabCollection malformed[] = {{NULL,1,1},{c.items,2,1},{c.items,0,0},{c.items,1,SIZE_MAX}};
    for (size_t i = 0; i < sizeof malformed / sizeof *malformed; i++) {
        assert(!slab_collection_find_by_id(&malformed[i], 3));
        assert(!slab_collection_find_by_id_const(&malformed[i], 3));
        assert(!slab_collection_remove_by_id(&malformed[i], 3));
        assert(slab_collection_append(&malformed[i], &s) == (i == 3 ? SLAB_NUMERIC_OVERFLOW : SLAB_INVALID_COLLECTION));
        assert(s.id == 5);
    }
    slab_destroy(&s); slab_collection_destroy(&c);
    assert(!c.items && !c.count && !c.capacity);
    slab_collection_destroy(&c); slab_collection_destroy(NULL); slab_destroy(NULL); slab_outline_destroy(NULL);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    Slab s = {0}; SlabCollection c = {0};
    assert(slab_build(1, rectangle, 4, 100, 0, &s) == SLAB_SUCCESS);
    unsigned char before[sizeof s]; memcpy(before, &s, sizeof s);
    fail_after = 0;
    assert(slab_build(2, s.definition.outline.vertices, 4, 200, 0, &s) == SLAB_ALLOCATION_FAILED);
    fail_after = SIZE_MAX;
    assert(memcmp(before, &s, sizeof s) == 0);
    for (size_t n = 0; n < 4; n++) {
        unsigned char collection_before[sizeof c]; memcpy(collection_before, &c, sizeof c);
        fail_after = 0;
        assert(slab_collection_append(&c, &s) == SLAB_ALLOCATION_FAILED);
        fail_after = SIZE_MAX;
        assert(memcmp(before, &s, sizeof s) == 0);
        assert(memcmp(collection_before, &c, sizeof c) == 0);
        assert(c.count == c.capacity);
        assert(slab_collection_append(&c, &s) == SLAB_SUCCESS);
        /* Fill spare capacity to exercise every realloc transition. */
        do {
            assert(slab_build((DomainId)(c.count + 1), rectangle, 4, 100, 0, &s) == SLAB_SUCCESS);
            if (c.count == c.capacity) { break; }
            assert(slab_collection_append(&c, &s) == SLAB_SUCCESS);
        } while (1);
        memcpy(before, &s, sizeof s);
    }
    slab_destroy(&s); slab_collection_destroy(&c);
}
#endif

int main(void)
{
    test_shapes(); test_invalid_and_overflow(); test_collection();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("slab geometry and ownership tests passed");
    return 0;
}
