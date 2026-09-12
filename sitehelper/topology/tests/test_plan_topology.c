#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plan_topology.h"
#include "topology_numeric_internal.h"
#include "test_support.h"
#include "wall_plan_transform.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t allocations_before_failure = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int fail_allocation(void)
{
    if (allocations_before_failure != SIZE_MAX && allocations_before_failure-- == 0) {
        allocation_failed = 1; return 1;
    }
    return 0;
}
void *__wrap_malloc(size_t size) { return fail_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return fail_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *pointer, size_t size) { return fail_allocation() ? NULL : __real_realloc(pointer, size); }
#endif

#define SOURCE(id, x1, y1, x2, y2) \
    (PlanTopologySource){PLAN_TOPOLOGY_SOURCE_WALL, id, {{x1, y1}, {x2, y2}}}

static void rectangle(PlanTopologySource *s, DomainId first, int x, int y, int size)
{
    s[0] = SOURCE(first, x, y, x + size, y);
    s[1] = SOURCE(first + 1, x + size, y, x + size, y + size);
    s[2] = SOURCE(first + 2, x + size, y + size, x, y + size);
    s[3] = SOURCE(first + 3, x, y + size, x, y);
}

static void fraction(PlanTopologyRational actual, int64_t numerator, uint64_t denominator)
{
    PlanTopologyRational expected = topology_rational(topology_int_from_i64(numerator), topology_uint_from_u64(denominator));
    assert(actual.negative == expected.negative);
    assert(actual.numerator.lo == expected.numerator.lo && actual.numerator.hi == expected.numerator.hi);
    assert(actual.denominator.lo == expected.denominator.lo && actual.denominator.hi == expected.denominator.hi);
}

static void rational_equal(PlanTopologyRational actual, PlanTopologyRational expected)
{
    assert(actual.negative == expected.negative);
    assert(actual.numerator.lo == expected.numerator.lo && actual.numerator.hi == expected.numerator.hi);
    assert(actual.denominator.lo == expected.denominator.lo && actual.denominator.hi == expected.denominator.hi);
}

static size_t find_vertex(const PlanTopology *t, int64_t xn, uint64_t xd,
    int64_t yn, uint64_t yd)
{
    PlanTopologyVertex wanted = {topology_rational(topology_int_from_i64(xn), topology_uint_from_u64(xd)), topology_rational(topology_int_from_i64(yn), topology_uint_from_u64(yd))};
    for (size_t i = 0; i < t->vertex_count; i++) {
        if (topology_vertex_compare(wanted, t->vertices[i]) == 0) { return i; }
    }
    assert(!"missing exact vertex");
    return SIZE_MAX;
}

/* Approximation is used only to independently check winding of small test
 * polygons, never as a vertex identity or construction operation. */
static long double approximate(PlanTopologyRational r)
{
    const long double base = 18446744073709551616.0L;
    long double value = (r.numerator.hi * base + r.numerator.lo) /
        (r.denominator.hi * base + r.denominator.lo);
    return r.negative ? -value : value;
}

static void consistent(const PlanTopology *t)
{
    assert(t->face_count >= 1 && !t->faces[0].bounded);
    assert(t->step_count == t->edge_count * 2);
    for (size_t i = 1; i < t->vertex_count; i++) {
        assert(topology_vertex_compare(t->vertices[i - 1], t->vertices[i]) < 0);
    }
    size_t *seen = t->step_count == 0 ? NULL : calloc(t->step_count, sizeof *seen);
    assert(t->step_count == 0 || seen);
    size_t total = 0;
    for (size_t f = 0; f < t->face_count; f++) {
        PlanTopologyFace face = t->faces[f];
        assert(face.bounded == (f != 0));
        assert(!face.bounded || face.boundary_count == 1);
        assert(face.first_boundary + face.boundary_count <= t->boundary_count);
        for (size_t j = 0; j < face.boundary_count; j++) {
            PlanTopologyBoundary boundary = t->boundaries[face.first_boundary + j];
            assert(boundary.step_count > 0 && boundary.first_step + boundary.step_count <= t->step_count);
            size_t first = SIZE_MAX, previous = SIZE_MAX;
            long double area_twice = 0;
            for (size_t k = 0; k < boundary.step_count; k++) {
                PlanTopologyBoundaryStep step = t->steps[boundary.first_step + k];
                assert(step.edge < t->edge_count);
                assert(seen[2 * step.edge + step.reversed]++ == 0);
                PlanTopologyEdge edge = t->edges[step.edge];
                assert(edge.start_vertex < edge.end_vertex && edge.end_vertex < t->vertex_count);
                assert((step.reversed ? edge.reverse_face : edge.forward_face) == f);
                size_t origin = step.reversed ? edge.end_vertex : edge.start_vertex;
                size_t target = step.reversed ? edge.start_vertex : edge.end_vertex;
                if (k == 0) { first = origin; } else { assert(previous == origin); }
                previous = target;
                PlanTopologyVertex a = t->vertices[origin], b = t->vertices[target];
                area_twice += approximate(a.x) * approximate(b.y) - approximate(a.y) * approximate(b.x);
            }
            assert(previous == first);
            if (face.bounded) { assert(area_twice > 0); }
            total += boundary.step_count;
        }
    }
    assert(total == t->step_count);
    free(seen);
}

static PlanTopology build(const PlanTopologySource *s, size_t count, size_t vertices, size_t edges, size_t faces)
{
    PlanTopology t = {0};
    PlanTopologyResult status = plan_topology_build(s, count, &t);
    assert(status.code == PLAN_TOPOLOGY_SUCCESS);
    assert(t.vertex_count == vertices && t.edge_count == edges && t.face_count == faces);
    consistent(&t);
    return t;
}

static void complementary_parameters(PlanTopologyRational original, PlanTopologyRational reversed)
{
#if defined(_MSC_VER)
    TopologyUInt n = (TopologyUInt){original.numerator.lo, original.numerator.hi};
    TopologyUInt d = (TopologyUInt){original.denominator.lo, original.denominator.hi};
#else
    TopologyUInt n = ((TopologyUInt)original.numerator.hi << 64) | original.numerator.lo;
    TopologyUInt d = ((TopologyUInt)original.denominator.hi << 64) | original.denominator.lo;
#endif
    assert(!original.negative && topology_uint_compare(n, d) <= 0);
    /* Builder source parameters have at most 65 bits, so this signed cast fits. */
    rational_equal(reversed, topology_rational(topology_uint_as_int(topology_uint_subtract(d, n)), d));
}

static void equivalent(const PlanTopology *a, const PlanTopology *b, bool spans)
{
    assert(a->vertex_count == b->vertex_count && a->edge_count == b->edge_count);
    assert(a->face_count == b->face_count && a->boundary_count == b->boundary_count && a->step_count == b->step_count);
    for (size_t i = 0; i < a->vertex_count; i++) { assert(topology_vertex_compare(a->vertices[i], b->vertices[i]) == 0); }
    for (size_t i = 0; i < a->edge_count; i++) {
        PlanTopologyEdge x = a->edges[i], y = b->edges[i];
        assert(x.start_vertex == y.start_vertex && x.end_vertex == y.end_vertex);
        assert(x.source_id == y.source_id && x.source_kind == y.source_kind);
        PlanPosition ys = spans ? y.source_segment.start : y.source_segment.end;
        PlanPosition ye = spans ? y.source_segment.end : y.source_segment.start;
        assert(x.source_segment.start.x == ys.x && x.source_segment.start.y == ys.y);
        assert(x.source_segment.end.x == ye.x && x.source_segment.end.y == ye.y);
        assert(x.forward_face == y.forward_face && x.reverse_face == y.reverse_face);
        if (spans) {
            assert(topology_rational_compare(x.source_t_start, y.source_t_start) == 0);
            assert(topology_rational_compare(x.source_t_end, y.source_t_end) == 0);
        }
        else {
            complementary_parameters(x.source_t_start, y.source_t_start);
            complementary_parameters(x.source_t_end, y.source_t_end);
        }
    }
    for (size_t i = 0; i < a->face_count; i++) {
        assert(a->faces[i].bounded == b->faces[i].bounded);
        assert(a->faces[i].first_boundary == b->faces[i].first_boundary);
        assert(a->faces[i].boundary_count == b->faces[i].boundary_count);
    }
    for (size_t i = 0; i < a->boundary_count; i++) {
        assert(a->boundaries[i].first_step == b->boundaries[i].first_step);
        assert(a->boundaries[i].step_count == b->boundaries[i].step_count);
    }
    for (size_t i = 0; i < a->step_count; i++) {
        assert(a->steps[i].edge == b->steps[i].edge && a->steps[i].reversed == b->steps[i].reversed);
    }
}

static void test_open_geometry_and_intersections(void)
{
    PlanTopology t = build(NULL, 0, 0, 0, 1);
    assert(t.faces[0].boundary_count == 0);
    plan_topology_destroy(&t);
    PlanTopologySource chain[] = {SOURCE(1, 0, 0, 10, 0), SOURCE(2, 10, 0, 20, 0), SOURCE(3, 20, 0, 20, 10)};
    t = build(chain, 1, 2, 1, 1); /* Intentionally test only the isolated first segment. */
    assert(t.faces[0].boundary_count == 1 && t.boundaries[0].step_count == 2);
    plan_topology_destroy(&t);
    t = build(chain, 3, 4, 3, 1); /* Includes collinear touching and an endpoint turn. */
    plan_topology_destroy(&t);
    PlanTopologySource tee[] = {SOURCE(1, 0, 0, 10, 0), SOURCE(2, 5, 0, 5, 5)};
    t = build(tee, 2, 4, 3, 1);
    find_vertex(&t, 5, 1, 0, 1);
    plan_topology_destroy(&t);
    PlanTopologySource crossing[] = {SOURCE(1, 0, 0, 3, 3), SOURCE(2, 0, 3, 3, 0)};
    t = build(crossing, 2, 5, 4, 1);
    find_vertex(&t, 3, 2, 3, 2);
    plan_topology_destroy(&t);
    PlanTopologySource concurrent[] = {SOURCE(1, 0, 0, 1, 2), SOURCE(2, 0, 1, 1, 0), SOURCE(3, 0, 2, 1, -2)};
    t = build(concurrent, 3, 7, 6, 1);
    find_vertex(&t, 1, 3, 2, 3); /* Three different pair constructions deduplicate exactly. */
    plan_topology_destroy(&t);
    PlanTopologySource multiple[] = {SOURCE(1, 0, 0, 8, 0), SOURCE(2, 2, -3, 2, 3),
        SOURCE(3, 4, -3, 4, 3), SOURCE(4, 6, -3, 6, 3)};
    t = build(multiple, 4, 11, 10, 1);
    size_t piece = 0;
    for (size_t i = 0; i < t.edge_count; i++) {
        if (t.edges[i].source_id == 1) {
            fraction(t.edges[i].source_t_start, (int64_t)piece, 4);
            fraction(t.edges[i].source_t_end, (int64_t)piece + 1, 4);
            piece++;
        }
    }
    assert(piece == 4);
    plan_topology_destroy(&t);
}

static void test_faces_and_determinism(void)
{
    PlanTopologySource sources[6];
    rectangle(sources, 1, 0, 0, 10);
    PlanTopology plain = build(sources, 4, 4, 4, 2);
    assert(plain.faces[1].boundary_count == 1 && plain.boundaries[plain.faces[1].first_boundary].step_count == 4);
    plan_topology_destroy(&plain);
    sources[2].kind = PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR;
    plain = build(sources, 4, 4, 4, 2);
    plan_topology_destroy(&plain);
    sources[4] = SOURCE(5, 0, 0, 10, 10);
    sources[5] = SOURCE(6, 0, 10, 10, 0);
    PlanTopology expected = build(sources, 6, 5, 8, 5);
    uint32_t random = 123;
    for (int iteration = 0; iteration < 64; iteration++) {
        PlanTopologySource copy[6]; memcpy(copy, sources, sizeof copy);
        for (size_t i = 5; i > 0; i--) {
            random = random * 1664525u + 1013904223u;
            size_t j = random % (i + 1);
            PlanTopologySource swap = copy[i]; copy[i] = copy[j]; copy[j] = swap;
        }
        PlanTopology actual = build(copy, 6, 5, 8, 5);
        equivalent(&expected, &actual, true);
        plan_topology_destroy(&actual);
    }
    for (size_t i = 0; i < 6; i++) {
        PlanPosition p = sources[i].segment.start;
        sources[i].segment.start = sources[i].segment.end; sources[i].segment.end = p;
    }
    PlanTopology reversed = build(sources, 6, 5, 8, 5);
    equivalent(&expected, &reversed, false);
    plan_topology_destroy(&expected); plan_topology_destroy(&reversed);
    PlanTopologySource bow[] = {SOURCE(1, 0, 0, 4, 4), SOURCE(2, 4, 4, 0, 4),
        SOURCE(3, 0, 4, 4, 0), SOURCE(4, 4, 0, 0, 0)};
    plain = build(bow, 4, 5, 6, 3);
    plan_topology_destroy(&plain);
    rectangle(sources, 1, 0, 0, 10);
    sources[4] = SOURCE(5, 5, 0, 5, 10);
    plain = build(sources, 5, 6, 7, 3);
    plan_topology_destroy(&plain);
    sources[4] = SOURCE(5, 0, 5, -5, 5); /* Exterior branch, extremum at a leaf. */
    plain = build(sources, 5, 6, 6, 2);
    plan_topology_destroy(&plain);
    PlanTopologySource disjoint[8];
    rectangle(disjoint, 1, 0, 0, 10); rectangle(disjoint + 4, 5, 20, 0, 10);
    plain = build(disjoint, 8, 8, 8, 3);
    assert(plain.faces[0].boundary_count == 2);
    plan_topology_destroy(&plain);
    rectangle(disjoint + 4, 5, 10, 10, 10); /* Articulation only in exterior walk. */
    plain = build(disjoint, 8, 7, 8, 3);
    plan_topology_destroy(&plain);
}

/* All eight square symmetries: rotations 0/90/180/270, with/without an
 * X-axis reflection. Includes reflection across Y. Fixtures use small ints. */
static PlanPosition symmetry(PlanPosition p, unsigned operation)
{
    if (operation & 4) { p.y = -p.y; }
    for (unsigned i = 0; i < (operation & 3); i++) { p = (PlanPosition){-p.y, p.x}; }
    return p;
}

static void symmetric_fixture(const PlanTopologySource *sources, size_t count,
    size_t vertices, size_t edges, size_t faces, size_t bounded_steps)
{
    assert(count <= 8);
    for (unsigned operation = 0; operation < 8; operation++) {
        PlanTopologySource copy[8];
        for (size_t i = 0; i < count; i++) {
            copy[i] = sources[i];
            copy[i].segment.start = symmetry(copy[i].segment.start, operation);
            copy[i].segment.end = symmetry(copy[i].segment.end, operation);
        }
        PlanTopology expected = build(copy, count, vertices, edges, faces);
        assert(expected.faces[0].boundary_count == 1);
        /* Independent expected polygon lengths: rectangle perimeter (possibly
         * split at a T) or the two triangles of the junction fixture. */
        for (size_t f = 1; f < faces; f++) {
            assert(expected.boundaries[expected.faces[f].first_boundary].step_count == bounded_steps);
        }
        for (size_t shift = 0; shift < count; shift++) {
            PlanTopologySource permuted[8];
            for (size_t i = 0; i < count; i++) { permuted[i] = copy[(i + shift) % count]; }
            PlanTopology actual = build(permuted, count, vertices, edges, faces);
            equivalent(&expected, &actual, true);
            for (size_t i = 0; i < count; i++) {
                PlanPosition p = permuted[i].segment.start;
                permuted[i].segment.start = permuted[i].segment.end;
                permuted[i].segment.end = p;
            }
            assert(plan_topology_build(permuted, count, &actual).code == PLAN_TOPOLOGY_SUCCESS);
            consistent(&actual); /* Reflections must still yield CCW bounded walks. */
            equivalent(&expected, &actual, false);
            plan_topology_destroy(&actual);
        }
        plan_topology_destroy(&expected);
    }
}

static void test_exterior_symmetries(void)
{
    PlanTopologySource sources[6];
    rectangle(sources, 1, 0, 0, 10);
    symmetric_fixture(sources, 4, 4, 4, 2, 4); /* Least vertex is a polygon corner. */
    const PlanSegment branches[] = {
        {{0, 0}, {-5, -5}}, {{10, 0}, {15, -5}},
        {{10, 10}, {15, 15}}, {{0, 10}, {-5, 15}},
        {{0, 5}, {-5, 5}}, {{5, 0}, {5, -5}},
        {{10, 5}, {15, 5}}, {{5, 10}, {5, 15}}
    };
    for (size_t i = 0; i < sizeof branches / sizeof *branches; i++) {
        sources[4] = (PlanTopologySource){PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR, 5, branches[i]};
        size_t split = i >= 4;
        symmetric_fixture(sources, 5, 5 + split, 5 + split, 2, 4 + split);
    }
    /* Lexicographically least vertex (0,0) has four outgoing rays, both
     * positive and negative slopes, plus a vertical exterior branch. */
    PlanTopologySource junction[] = {
        SOURCE(1, 0, 0, 10, -10), SOURCE(2, 0, 0, 10, 0), SOURCE(3, 0, 0, 10, 10),
        SOURCE(4, 10, -10, 10, 0), SOURCE(5, 10, 0, 10, 10), SOURCE(6, 0, 0, 0, 15)
    };
    symmetric_fixture(junction, 6, 5, 6, 3, 3);
}

static void test_provenance_and_wall_local_span(void)
{
    PlanTopologySource sources[] = {SOURCE(17, 4000, 6000, 1000, 2000),
        {PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR, 18, {{2500, 3000}, {2500, 5000}}}};
    PlanTopology t = build(sources, 2, 5, 4, 1);
    size_t count = 0;
    for (size_t i = 0; i < t.edge_count; i++) {
        PlanTopologyEdge edge = t.edges[i];
        if (edge.source_id != 17) { assert(edge.source_id == 18 && edge.source_kind == PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR); continue; }
        assert(edge.source_kind == PLAN_TOPOLOGY_SOURCE_WALL);
        fraction(edge.source_t_start, 1, count == 0 ? 1 : 2);
        fraction(edge.source_t_end, count == 0 ? 1 : 0, count == 0 ? 2 : 1);
        count++;
    }
    assert(count == 2);
    size_t midpoint = find_vertex(&t, 2500, 1, 4000, 1);
    WallPlanSegment wall = {sources[0].segment.start, sources[0].segment.end};
    assert(wall_plan_segment_length_mm(wall) == 5000);
    PlanPoint point;
    assert(wall_plan_segment_u_to_plan(wall, 2500, &point));
    fraction(t.vertices[midpoint].x, (int)point.x, 1);
    fraction(t.vertices[midpoint].y, (int)point.y, 1);
    plan_topology_destroy(&t);
}

static void expect_failure(const PlanTopologySource *sources, size_t count, PlanTopologyCode code)
{
    PlanTopologySource one = SOURCE(100, -1, 0, 1, 0);
    PlanTopology t = build(&one, 1, 2, 1, 1), expected = build(&one, 1, 2, 1, 1);
    PlanTopology original = t;
    PlanTopologyResult status = plan_topology_build(sources, count, &t);
    assert(status.code == code);
    assert(t.vertices == original.vertices && t.edges == original.edges && t.faces == original.faces);
    assert(t.boundaries == original.boundaries && t.steps == original.steps);
    equivalent(&t, &expected, true);
    plan_topology_destroy(&t); plan_topology_destroy(&expected);
}

static void symmetric_failure_fixture(const PlanTopologySource *sources, size_t count, PlanTopologyCode code)
{
    assert(count <= 9);
    for (unsigned operation = 0; operation < 8; operation++) {
        PlanTopologySource copy[9];
        for (size_t i = 0; i < count; i++) {
            copy[i] = sources[count - 1 - i]; /* Permute and reverse all sources. */
            copy[i].segment.start = symmetry(sources[count - 1 - i].segment.end, operation);
            copy[i].segment.end = symmetry(sources[count - 1 - i].segment.start, operation);
        }
        expect_failure(copy, count, code);
    }
}

static void test_explicit_failures_and_numeric_limits(void)
{
    PlanTopologySource invalid = SOURCE(1, 0, 0, 0, 0);
    expect_failure(&invalid, 1, PLAN_TOPOLOGY_INVALID_SOURCE);
    invalid = SOURCE(0, 0, 0, 1, 1);
    expect_failure(&invalid, 1, PLAN_TOPOLOGY_INVALID_SOURCE);
    invalid = SOURCE(1, 0, 0, 1, 1); invalid.kind = (PlanTopologySourceKind)99;
    expect_failure(&invalid, 1, PLAN_TOPOLOGY_INVALID_SOURCE);
    expect_failure(NULL, 1, PLAN_TOPOLOGY_INVALID_ARGUMENT);
    assert(plan_topology_build(NULL, 0, NULL).code == PLAN_TOPOLOGY_INVALID_ARGUMENT);
    invalid = SOURCE(1, 0, 0, 1, 1);
    expect_failure(&invalid, SIZE_MAX, PLAN_TOPOLOGY_NUMERIC_OVERFLOW);
    PlanTopologySource overlap[] = {SOURCE(1, 0, 0, 10, 0), SOURCE(2, 5, 0, 15, 0)};
    expect_failure(overlap, 2, PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    symmetric_failure_fixture(overlap, 2, PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    overlap[1] = SOURCE(2, 10, 0, 0, 0);
    expect_failure(overlap, 2, PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    overlap[1] = SOURCE(2, 0, 0, 10, 0);
    expect_failure(overlap, 2, PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    symmetric_failure_fixture(overlap, 2, PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP);
    overlap[1] = SOURCE(1, 20, 0, 30, 0);
    expect_failure(overlap, 2, PLAN_TOPOLOGY_INVALID_SOURCE);
    PlanTopologySource nested[9];
    rectangle(nested, 1, 0, 0, 10); rectangle(nested + 4, 5, 2, 2, 2);
    expect_failure(nested, 8, PLAN_TOPOLOGY_UNSUPPORTED_NESTING);
    symmetric_failure_fixture(nested, 8, PLAN_TOPOLOGY_UNSUPPORTED_NESTING);
    nested[8] = SOURCE(9, 0, 0, 2, 2);
    expect_failure(nested, 9, PLAN_TOPOLOGY_UNSUPPORTED_NON_SIMPLE_FACE);
    symmetric_failure_fixture(nested, 9, PLAN_TOPOLOGY_UNSUPPORTED_NON_SIMPLE_FACE);
    nested[4] = SOURCE(5, 0, 5, 5, 5);
    expect_failure(nested, 5, PLAN_TOPOLOGY_UNSUPPORTED_NON_SIMPLE_FACE);
    nested[4] = SOURCE(5, 2, 2, 3, 3);
    expect_failure(nested, 5, PLAN_TOPOLOGY_UNSUPPORTED_NESTING);

    PlanTopologySource extreme[] = {SOURCE(1, INT_MIN, INT_MIN, INT_MAX, INT_MAX),
        SOURCE(2, INT_MIN, INT_MAX, INT_MAX, INT_MIN)};
    PlanTopology t = build(extreme, 2, 5, 4, 1);
    find_vertex(&t, -1, 2, -1, 2);
    plan_topology_destroy(&t);
    PlanTopologySource box[] = {SOURCE(1, INT_MIN, INT_MIN, INT_MAX, INT_MIN),
        SOURCE(2, INT_MAX, INT_MIN, INT_MAX, INT_MAX), SOURCE(3, INT_MAX, INT_MAX, INT_MIN, INT_MAX),
        SOURCE(4, INT_MIN, INT_MAX, INT_MIN, INT_MIN)};
    t = build(box, 4, 4, 4, 2);
    plan_topology_destroy(&t);
    extreme[0] = SOURCE(1, INT_MIN, INT_MIN, INT_MAX, INT_MAX - 1);
    extreme[1] = SOURCE(2, INT_MIN, INT_MAX, INT_MAX - 1, INT_MIN);
    t = build(extreme, 2, 5, 4, 1);
    int wide_denominator = 0;
    for (size_t i = 0; i < t.vertex_count; i++) {
        if (t.vertices[i].x.denominator.hi || t.vertices[i].y.denominator.hi) { wide_denominator = 1; }
    }
    assert(wide_denominator);
    plan_topology_destroy(&t);
    TopologyInt large = topology_uint_as_int(topology_uint_shift_left(topology_uint_from_u64(1), 126)), out;
    assert(!topology_checked_multiply(large, topology_int_from_i64(2), &out));
    assert(!topology_checked_add(large, large, &out));
    assert(topology_checked_multiply(large, topology_int_from_i64(1), &out) && topology_int_compare(out, large) == 0);
    assert(topology_rational_compare(topology_rational(topology_int_subtract(large, topology_int_from_i64(1)), topology_int_as_uint(large)),
        topology_rational(topology_int_subtract(large, topology_int_from_i64(2)), topology_uint_subtract(topology_int_as_uint(large), topology_uint_from_u64(1)))) > 0);
    assert(topology_rational_compare(topology_rational(topology_int_from_i64(-1), topology_uint_from_u64(3)), topology_rational(topology_int_from_i64(-1), topology_uint_from_u64(2))) > 0);
    assert(topology_rational_compare(topology_rational(topology_int_from_i64(0), topology_uint_from_u64(999)), topology_rational(topology_int_from_i64(0), topology_uint_from_u64(1))) == 0);
    /* Explicit limb expectations exercise reduction and the minimum signed
     * value without using the rational constructor to construct the oracle. */
    PlanTopologyRational reduced = topology_rational(topology_int_from_i64(-6), topology_uint_from_u64(8));
    assert(reduced.negative && reduced.numerator.lo == 3 && reduced.numerator.hi == 0);
    assert(reduced.denominator.lo == 4 && reduced.denominator.hi == 0);
    /* -2^126 - 2^126 is representable; +2^126 + 2^126 is not. */
    TopologyInt minimum = topology_int_subtract(topology_int_negate(large), large);
    reduced = topology_rational(minimum, topology_uint_from_u64(1));
    assert(reduced.negative && reduced.numerator.lo == 0 && reduced.numerator.hi == (UINT64_C(1) << 63));
    assert(reduced.denominator.lo == 1 && reduced.denominator.hi == 0);
    TopologyUInt maximum = topology_uint_complement(topology_uint_from_u64(0));
    assert(topology_rational_compare(topology_rational(topology_int_from_i64(1), maximum), topology_rational(topology_int_from_i64(1), topology_uint_subtract(maximum, topology_uint_from_u64(1)))) < 0);
}

static void project_rectangle(SiteHelperProject *project)
{
    sitehelper_project_init(project);
    assert(sitehelper_project_add_storey(project, 0));
    assert(sitehelper_project_add_wall(project, project->storeys[0].id, (WallPlanSegment){{0, 0}, {6000, 0}}));
    assert(sitehelper_project_add_wall(project, project->storeys[0].id, (WallPlanSegment){{6000, 0}, {6000, 4000}}));
    assert(sitehelper_project_add_room_separator(project, project->storeys[0].id, (PlanSegment){{6000, 4000}, {0, 4000}}));
    assert(sitehelper_project_add_room_separator(project, project->storeys[0].id, (PlanSegment){{0, 4000}, {0, 0}}));
}

static void test_project_adapter_ignores_rooms_and_derived_state(void)
{
    SiteHelperProject project, before;
    project_rectangle(&project);
    PlanTopology t = {0}, expected = {0};
    assert(plan_topology_build_from_storey(&project.storeys[0], &expected).code == PLAN_TOPOLOGY_SUCCESS);
    assert(expected.face_count == 2);
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_add_room(&project, project.storeys[0].id));
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){INT_MIN, INT_MAX}));
    test_clone_project_authoritative(&project, &before);
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    equivalent(&expected, &t, true);
    test_assert_project_authoritative_equal(&before, &project);
    Wall swap = project.storeys[0].structure.walls[0]; project.storeys[0].structure.walls[0] = project.storeys[0].structure.walls[1]; project.storeys[0].structure.walls[1] = swap;
    RoomSeparator sep = project.storeys[0].structure.room_separators[0];
    project.storeys[0].structure.room_separators[0] = project.storeys[0].structure.room_separators[1]; project.storeys[0].structure.room_separators[1] = sep;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    equivalent(&expected, &t, true);
    /* Deliberately unreadable unrelated metadata proves no Room/definition
     * validator or generator is invoked by this geometry-only query. */
    BuildStructure saved = project.storeys[0].structure;
    Wall saved_wall = project.storeys[0].structure.walls[0];
    project.storeys[0].structure.rooms = NULL; project.storeys[0].structure.room_count = SIZE_MAX;
    project.storeys[0].structure.walls[0].definition.opening_count = SIZE_MAX;
    project.storeys[0].structure.walls[0].framing.stud_count = SIZE_MAX;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
    equivalent(&expected, &t, true);
    project.storeys[0].structure = saved;
    project.storeys[0].structure.walls[0] = saved_wall; /* Restore nested mutations, not just the array pointer. */
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    project.storeys[0].structure.walls = NULL;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_INVALID_SOURCE);
    project.storeys[0].structure = saved;
    equivalent(&expected, &t, true);
    project.storeys[0].structure.wall_count = project.storeys[0].structure.wall_capacity + 1;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_INVALID_SOURCE);
    project.storeys[0].structure = saved;
    project.storeys[0].structure.room_separators = NULL;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_INVALID_SOURCE);
    project.storeys[0].structure = saved;
    project.storeys[0].structure.room_separator_count = project.storeys[0].structure.room_separator_capacity + 1;
    assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_INVALID_SOURCE);
    project.storeys[0].structure = saved;
    equivalent(&expected, &t, true);
    assert(plan_topology_build_from_storey(NULL, &t).code == PLAN_TOPOLOGY_INVALID_ARGUMENT);
    assert(plan_topology_build_from_storey(&project.storeys[0], NULL).code == PLAN_TOPOLOGY_INVALID_ARGUMENT);
    /* The previous topology owns its coordinates after all source storage dies. */
    sitehelper_project_destroy(&project); sitehelper_project_destroy(&before);
    equivalent(&expected, &t, true);
    plan_topology_destroy(&t); plan_topology_destroy(&t); plan_topology_destroy(NULL);
    plan_topology_destroy(&expected);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failure_transactions(void)
{
    SiteHelperProject project, before;
    project_rectangle(&project);
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 0}, {6000, 4000}}));
    test_clone_project_authoritative(&project, &before);
    PlanTopologySource one = SOURCE(100, -1, 0, 1, 0);
    size_t failures = 0;
    for (size_t fail_at = 0; fail_at < 100; fail_at++) {
        PlanTopology t = build(&one, 1, 2, 1, 1), expected = build(&one, 1, 2, 1, 1);
        PlanTopology saved = t;
        allocation_failed = 0;
        allocations_before_failure = fail_at;
        PlanTopologyResult status = plan_topology_build_from_storey(&project.storeys[0], &t);
        allocations_before_failure = SIZE_MAX;
        test_assert_project_authoritative_equal(&before, &project);
        if (allocation_failed) {
            failures++;
            assert(status.code == PLAN_TOPOLOGY_ALLOCATION_FAILED);
            assert(t.vertices == saved.vertices && t.edges == saved.edges && t.faces == saved.faces);
            assert(t.boundaries == saved.boundaries && t.steps == saved.steps);
            equivalent(&expected, &t, true);
            assert(plan_topology_build_from_storey(&project.storeys[0], &t).code == PLAN_TOPOLOGY_SUCCESS);
        }
        else { assert(status.code == PLAN_TOPOLOGY_SUCCESS); }
        consistent(&t);
        plan_topology_destroy(&t); plan_topology_destroy(&expected);
        if (!allocation_failed) { break; }
    }
    assert(failures >= 15);
    printf("topology allocation failures checked: %zu\n", failures);
    sitehelper_project_destroy(&project); sitehelper_project_destroy(&before);
}
#endif

int main(void)
{
    test_open_geometry_and_intersections();
    test_faces_and_determinism();
    test_exterior_symmetries();
    test_provenance_and_wall_local_span();
    test_explicit_failures_and_numeric_limits();
    test_project_adapter_ignores_rooms_and_derived_state();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failure_transactions();
#endif
    puts("plan topology tests passed");
    return 0;
}
