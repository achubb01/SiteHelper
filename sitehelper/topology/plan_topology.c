#include <stdlib.h>
#include <string.h>
#include "topology_numeric_internal.h"

typedef struct {
    size_t source;
    PlanTopologyRational t;
    PlanTopologyVertex point;
} Cut;
typedef struct { PlanTopologyEdge value; size_t source; } DerivedEdge;
typedef struct { size_t origin, target, next, walk; int64_t dx, dy; } Direction;
typedef struct { size_t vertex, direction; int64_t dx, dy; } Ray;
typedef struct { size_t first, count, component; bool exterior; } Walk;
typedef struct {
    PlanTopologySource *sources;
    Cut *cuts;
    size_t cut_count, cut_capacity;
    DerivedEdge *edges;
    Direction *directions;
    Ray *rays;
    Walk *walks;
    size_t walk_count;
    size_t *roots, *seen;
} Workspace;

static PlanTopologyResult result(PlanTopologyCode code, DomainId id, DomainId related)
{
    return (PlanTopologyResult){code, id, related};
}

static void *allocate_array(size_t count, size_t size, PlanTopologyResult *status)
{
    if (count == 0) { return NULL; }
    if (count > SIZE_MAX / size) {
        *status = result(PLAN_TOPOLOGY_NUMERIC_OVERFLOW, 0, 0);
        return NULL;
    }
    void *storage = calloc(count, size);
    if (storage == NULL) { *status = result(PLAN_TOPOLOGY_ALLOCATION_FAILED, 0, 0); }
    return storage;
}

static int append_cut(Workspace *w, Cut cut, PlanTopologyResult *status)
{
    if (w->cut_count == w->cut_capacity) {
        size_t maximum = SIZE_MAX / sizeof *w->cuts;
        if (w->cut_capacity >= maximum) {
            *status = result(PLAN_TOPOLOGY_NUMERIC_OVERFLOW, 0, 0);
            return 0;
        }
        size_t grown = w->cut_capacity == 0 ? 8 :
            w->cut_capacity > maximum / 2 ? maximum : w->cut_capacity * 2;
        Cut *storage = realloc(w->cuts, grown * sizeof *storage);
        if (storage == NULL) {
            *status = result(PLAN_TOPOLOGY_ALLOCATION_FAILED, 0, 0);
            return 0;
        }
        w->cuts = storage;
        w->cut_capacity = grown;
    }
    w->cuts[w->cut_count++] = cut;
    return 1;
}

static void workspace_destroy(Workspace *w)
{
    free(w->sources); free(w->cuts); free(w->edges); free(w->directions);
    free(w->rays); free(w->walks); free(w->roots); free(w->seen);
}

void plan_topology_destroy(PlanTopology *topology)
{
    if (topology == NULL) { return; }
    free(topology->vertices); free(topology->edges); free(topology->faces);
    free(topology->boundaries); free(topology->steps);
    *topology = (PlanTopology){0};
}

static int source_compare(const void *pa, const void *pb)
{
    const PlanTopologySource *a = pa, *b = pb;
    if (a->kind != b->kind) { return a->kind < b->kind ? -1 : 1; }
    return (a->source_id > b->source_id) - (a->source_id < b->source_id);
}

static int cut_compare(const void *pa, const void *pb)
{
    const Cut *a = pa, *b = pb;
    if (a->source != b->source) { return a->source < b->source ? -1 : 1; }
    return topology_rational_compare(a->t, b->t);
}

static int vertex_compare(const void *a, const void *b)
{
    return topology_vertex_compare(*(const PlanTopologyVertex *)a, *(const PlanTopologyVertex *)b);
}

static int edge_compare(const void *pa, const void *pb)
{
    const DerivedEdge *a = pa, *b = pb;
    if (a->value.start_vertex != b->value.start_vertex) {
        return a->value.start_vertex < b->value.start_vertex ? -1 : 1;
    }
    if (a->value.end_vertex != b->value.end_vertex) {
        return a->value.end_vertex < b->value.end_vertex ? -1 : 1;
    }
    return (a->source > b->source) - (a->source < b->source);
}

static int ray_compare(const void *pa, const void *pb)
{
    const Ray *a = pa, *b = pb;
    if (a->vertex != b->vertex) { return a->vertex < b->vertex ? -1 : 1; }
    int ah = a->dy < 0 || (a->dy == 0 && a->dx < 0);
    int bh = b->dy < 0 || (b->dy == 0 && b->dx < 0);
    if (ah != bh) { return ah < bh ? -1 : 1; }
    TopologyInt cross = topology_cross(a->dx, a->dy, b->dx, b->dy);
    if (!topology_int_is_zero(cross)) { return topology_int_is_negative(cross) ? 1 : -1; }
    return (a->direction > b->direction) - (a->direction < b->direction);
}

static int intersection_coordinate(int origin, int64_t delta, TopologyInt t,
    TopologyInt denominator, PlanTopologyRational *coordinate)
{
    TopologyInt a, b, numerator;
    if (!topology_checked_multiply(topology_int_from_i64(origin), denominator, &a) ||
        !topology_checked_multiply(topology_int_from_i64(delta), t, &b) || !topology_checked_add(a, b, &numerator)) {
        return 0;
    }
    *coordinate = topology_rational(numerator, topology_int_magnitude(denominator));
    return 1;
}

static int intersect(Workspace *w, size_t i, size_t j, PlanTopologyResult *status)
{
    PlanSegment a = w->sources[i].segment, b = w->sources[j].segment;
    int64_t rx = (int64_t)a.end.x - a.start.x, ry = (int64_t)a.end.y - a.start.y;
    int64_t sx = (int64_t)b.end.x - b.start.x, sy = (int64_t)b.end.y - b.start.y;
    int64_t qx = (int64_t)b.start.x - a.start.x, qy = (int64_t)b.start.y - a.start.y;
    TopologyInt denominator = topology_cross(rx, ry, sx, sy);
    TopologyInt u = topology_cross(qx, qy, rx, ry);
    if (topology_int_is_zero(denominator)) {
        if (!topology_int_is_zero(u)) { return 1; } /* Parallel distinct supporting lines. */
        int64_t a0 = rx != 0 ? a.start.x : a.start.y;
        int64_t a1 = rx != 0 ? a.end.x : a.end.y;
        int64_t b0 = rx != 0 ? b.start.x : b.start.y;
        int64_t b1 = rx != 0 ? b.end.x : b.end.y;
        if (a0 > a1) { int64_t swap = a0; a0 = a1; a1 = swap; }
        if (b0 > b1) { int64_t swap = b0; b0 = b1; b1 = swap; }
        int64_t low = a0 > b0 ? a0 : b0, high = a1 < b1 ? a1 : b1;
        if (low < high) {
            *status = result(PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP,
                w->sources[i].source_id, w->sources[j].source_id);
            return 0;
        }
        /* A single touching point is already an endpoint in both cut lists. */
        return 1;
    }
    TopologyInt t = topology_cross(qx, qy, sx, sy);
    if (topology_int_is_negative(denominator)) { denominator = topology_int_negate(denominator); t = topology_int_negate(t); u = topology_int_negate(u); }
    if (topology_int_compare(t, topology_int_from_i64(0)) < 0 || topology_int_compare(t, denominator) > 0 || topology_int_compare(u, topology_int_from_i64(0)) < 0 || topology_int_compare(u, denominator) > 0) { return 1; }
    PlanTopologyVertex point;
    if (!intersection_coordinate(a.start.x, rx, t, denominator, &point.x) ||
        !intersection_coordinate(a.start.y, ry, t, denominator, &point.y)) {
        *status = result(PLAN_TOPOLOGY_NUMERIC_OVERFLOW, w->sources[i].source_id, w->sources[j].source_id);
        return 0;
    }
    return append_cut(w, (Cut){i, topology_rational(t, topology_int_magnitude(denominator)), point}, status) &&
        append_cut(w, (Cut){j, topology_rational(u, topology_int_magnitude(denominator)), point}, status);
}

static size_t vertex_index(const PlanTopology *topology, PlanTopologyVertex point)
{
    size_t low = 0, high = topology->vertex_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (topology_vertex_compare(topology->vertices[middle], point) < 0) { low = middle + 1; }
        else { high = middle; }
    }
    return low;
}

static int subdivide(Workspace *w, PlanTopology *topology, PlanTopologyResult *status)
{
    topology->vertices = allocate_array(w->cut_count, sizeof *topology->vertices, status);
    if (w->cut_count != 0 && topology->vertices == NULL) { return 0; }
    for (size_t i = 0; i < w->cut_count; i++) { topology->vertices[i] = w->cuts[i].point; }
    if (w->cut_count > 1) {
        qsort(topology->vertices, w->cut_count, sizeof *topology->vertices, vertex_compare);
        qsort(w->cuts, w->cut_count, sizeof *w->cuts, cut_compare);
    }
    for (size_t i = 0; i < w->cut_count; i++) {
        if (topology->vertex_count == 0 || topology_vertex_compare(topology->vertices[i],
                topology->vertices[topology->vertex_count - 1]) != 0) {
            topology->vertices[topology->vertex_count++] = topology->vertices[i];
        }
    }
    w->edges = allocate_array(w->cut_count, sizeof *w->edges, status);
    if (w->cut_count != 0 && w->edges == NULL) { return 0; }
    for (size_t i = 1; i < w->cut_count; i++) {
        Cut a = w->cuts[i - 1], b = w->cuts[i];
        if (a.source != b.source || topology_rational_compare(a.t, b.t) == 0) { continue; }
        size_t from = vertex_index(topology, a.point), to = vertex_index(topology, b.point);
        if (from > to) { Cut swap = a; a = b; b = swap; size_t v = from; from = to; to = v; }
        w->edges[topology->edge_count++] = (DerivedEdge){
            .value = {.start_vertex = from, .end_vertex = to,
                .source_kind = w->sources[a.source].kind, .source_id = w->sources[a.source].source_id,
                .source_segment = w->sources[a.source].segment,
                .source_t_start = a.t, .source_t_end = b.t}, .source = a.source
        };
    }
    if (topology->edge_count > 1) { qsort(w->edges, topology->edge_count, sizeof *w->edges, edge_compare); }
    return 1;
}

static size_t component_root(size_t *roots, size_t vertex)
{
    while (roots[vertex] != vertex) {
        roots[vertex] = roots[roots[vertex]];
        vertex = roots[vertex];
    }
    return vertex;
}

static int connect(Workspace *w, PlanTopology *topology, PlanTopologyResult *status)
{
    if (topology->edge_count > SIZE_MAX / 2) {
        *status = result(PLAN_TOPOLOGY_NUMERIC_OVERFLOW, 0, 0); return 0;
    }
    size_t count = 2 * topology->edge_count;
    w->directions = allocate_array(count, sizeof *w->directions, status);
    if (count != 0 && w->directions == NULL) { return 0; }
    w->rays = allocate_array(count, sizeof *w->rays, status);
    if (count != 0 && w->rays == NULL) { return 0; }
    w->walks = allocate_array(count, sizeof *w->walks, status);
    if (count != 0 && w->walks == NULL) { return 0; }
    w->roots = allocate_array(topology->vertex_count, sizeof *w->roots, status);
    if (topology->vertex_count != 0 && w->roots == NULL) { return 0; }
    for (size_t i = 0; i < topology->vertex_count; i++) { w->roots[i] = i; }
    for (size_t i = 0; i < topology->edge_count; i++) {
        PlanTopologyEdge edge = w->edges[i].value;
        PlanSegment segment = w->sources[w->edges[i].source].segment;
        int64_t dx = (int64_t)segment.end.x - segment.start.x, dy = (int64_t)segment.end.y - segment.start.y;
        if (topology_rational_compare(edge.source_t_start, edge.source_t_end) > 0) { dx = -dx; dy = -dy; }
        w->directions[2 * i] = (Direction){edge.start_vertex, edge.end_vertex, 0, SIZE_MAX, dx, dy};
        w->directions[2 * i + 1] = (Direction){edge.end_vertex, edge.start_vertex, 0, SIZE_MAX, -dx, -dy};
        size_t a = component_root(w->roots, edge.start_vertex), b = component_root(w->roots, edge.end_vertex);
        if (a < b) { w->roots[b] = a; } else { w->roots[a] = b; }
    }
    for (size_t i = 0; i < count; i++) {
        Direction d = w->directions[i];
        w->rays[i] = (Ray){d.origin, i, d.dx, d.dy};
    }
    if (count > 1) { qsort(w->rays, count, sizeof *w->rays, ray_compare); }
    for (size_t first = 0; first < count;) {
        size_t end = first + 1;
        while (end < count && w->rays[end].vertex == w->rays[first].vertex) { end++; }
        for (size_t i = first; i < end; i++) {
            /* At the destination, take the clockwise predecessor of the twin
             * to keep the traversed face on the left. */
            w->directions[w->rays[i].direction ^ 1].next = w->rays[i == first ? end - 1 : i - 1].direction;
        }
        first = end;
    }
    for (size_t i = 0; i < count; i++) {
        if (w->directions[i].walk != SIZE_MAX) { continue; }
        Walk walk = {.first = i, .component = component_root(w->roots, w->directions[i].origin)};
        size_t step = i;
        do {
            w->directions[step].walk = w->walk_count;
            walk.count++;
            step = w->directions[step].next;
        } while (step != i);
        w->walks[w->walk_count++] = walk;
    }
    /* Each connected embedding has exactly one exterior walk. At its
     * lexicographically least vertex, the uppermost outgoing ray has the
     * exterior on its left. All outgoing vectors there have dx >= 0, and
     * dx == 0 implies dy > 0 (the vertex is also least in y at that x).
     * Their angles therefore lie in (-pi/2, pi/2], a range strictly less
     * than pi: cross(best, candidate) > 0 selects the uppermost ray even
     * across ray_compare's 0/2pi cut. Its left sector reaches the empty
     * half-plane x < minimum_x, hence the unbounded face of this component.
     * This also holds at a branch leaf or a high-degree extremal junction.
     * Nesting between components is checked separately below. No rational
     * area accumulation or floating-point orientation is required. */
    for (size_t first = 0; first < count;) {
        size_t end = first + 1, best = first;
        while (end < count && w->rays[end].vertex == w->rays[first].vertex) { end++; }
        if (component_root(w->roots, w->rays[first].vertex) == w->rays[first].vertex) {
            for (size_t i = first + 1; i < end; i++) {
                if (topology_int_compare(topology_cross(w->rays[best].dx, w->rays[best].dy, w->rays[i].dx, w->rays[i].dy), topology_int_from_i64(0)) > 0) { best = i; }
            }
            w->walks[w->directions[w->rays[best].direction].walk].exterior = true;
        }
        first = end;
    }
    return 1;
}

/* Only integer source endpoints are needed as component representatives.
 * Ray tests compare rational endpoint heights exactly and use the original
 * integer supporting line for orientation, so no rational determinant grows. */
static int point_inside_walk(const Workspace *w, const PlanTopology *topology,
    Walk walk, PlanPosition point)
{
    int inside = 0;
    PlanTopologyRational y = topology_rational(topology_int_from_i64(point.y), topology_uint_from_u64(1));
    size_t step = walk.first;
    do {
        Direction d = w->directions[step];
        int a = topology_rational_compare(topology->vertices[d.origin].y, y);
        int b = topology_rational_compare(topology->vertices[d.target].y, y);
        if ((a > 0) != (b > 0)) {
            PlanSegment source = w->sources[w->edges[step / 2].source].segment;
            TopologyInt side = topology_cross(d.dx, d.dy,
                (int64_t)point.x - source.start.x, (int64_t)point.y - source.start.y);
            if ((d.dy > 0 && topology_int_compare(side, topology_int_from_i64(0)) > 0) || (d.dy < 0 && topology_int_compare(side, topology_int_from_i64(0)) < 0)) { inside = !inside; }
        }
        step = d.next;
    } while (step != walk.first);
    return inside;
}

static int supported_faces(Workspace *w, const PlanTopology *topology, PlanTopologyResult *status)
{
    w->seen = allocate_array(topology->vertex_count, sizeof *w->seen, status);
    if (topology->vertex_count != 0 && w->seen == NULL) { return 0; }
    for (size_t i = 0; i < topology->vertex_count; i++) { w->seen[i] = SIZE_MAX; }
    for (size_t i = 0; i < w->walk_count; i++) {
        Walk walk = w->walks[i];
        if (walk.exterior) { continue; }
        size_t step = walk.first;
        do {
            size_t vertex = w->directions[step].origin;
            if (w->seen[vertex] == i) {
                *status = result(PLAN_TOPOLOGY_UNSUPPORTED_NON_SIMPLE_FACE,
                    w->edges[step / 2].value.source_id, 0);
                return 0;
            }
            w->seen[vertex] = i;
            step = w->directions[step].next;
        } while (step != walk.first);
        for (size_t j = 0; j < w->walk_count; j++) {
            Walk other = w->walks[j];
            if (!other.exterior || other.component == walk.component) { continue; }
            PlanTopologySource source = w->sources[w->edges[other.first / 2].source];
            if (point_inside_walk(w, topology, walk, source.segment.start)) {
                *status = result(PLAN_TOPOLOGY_UNSUPPORTED_NESTING, source.source_id,
                    w->edges[walk.first / 2].value.source_id);
                return 0;
            }
        }
    }
    return 1;
}

static int export_faces(Workspace *w, PlanTopology *topology, PlanTopologyResult *status)
{
    size_t bounded = 0, exterior_boundaries = 0;
    for (size_t i = 0; i < w->walk_count; i++) {
        if (w->walks[i].exterior) { exterior_boundaries++; } else { bounded++; }
    }
    if (bounded == SIZE_MAX) { *status = result(PLAN_TOPOLOGY_NUMERIC_OVERFLOW, 0, 0); return 0; }
    topology->face_count = bounded + 1;
    topology->boundary_count = w->walk_count;
    topology->step_count = 2 * topology->edge_count;
    topology->faces = allocate_array(topology->face_count, sizeof *topology->faces, status);
    if (topology->faces == NULL) { return 0; }
    topology->boundaries = allocate_array(topology->boundary_count, sizeof *topology->boundaries, status);
    if (topology->boundary_count != 0 && topology->boundaries == NULL) { return 0; }
    topology->steps = allocate_array(topology->step_count, sizeof *topology->steps, status);
    if (topology->step_count != 0 && topology->steps == NULL) { return 0; }
    topology->edges = allocate_array(topology->edge_count, sizeof *topology->edges, status);
    if (topology->edge_count != 0 && topology->edges == NULL) { return 0; }
    for (size_t i = 0; i < topology->edge_count; i++) { topology->edges[i] = w->edges[i].value; }
    topology->faces[0] = (PlanTopologyFace){false, 0, exterior_boundaries};
    size_t boundary = 0, offset = 0, face = 1;
    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = 0; i < w->walk_count; i++) {
            Walk walk = w->walks[i];
            if (walk.exterior != (pass == 0)) { continue; }
            size_t face_id = walk.exterior ? 0 : face++;
            if (!walk.exterior) { topology->faces[face_id] = (PlanTopologyFace){true, boundary, 1}; }
            topology->boundaries[boundary++] = (PlanTopologyBoundary){offset, walk.count};
            size_t step = walk.first;
            do {
                topology->steps[offset++] = (PlanTopologyBoundaryStep){step / 2, (step & 1) != 0};
                if ((step & 1) != 0) { topology->edges[step / 2].reverse_face = face_id; }
                else { topology->edges[step / 2].forward_face = face_id; }
                step = w->directions[step].next;
            } while (step != walk.first);
        }
    }
    return 1;
}

PlanTopologyResult plan_topology_build(const PlanTopologySource *sources,
    size_t source_count, PlanTopology *output)
{
    if (output == NULL || (source_count != 0 && sources == NULL)) {
        return result(PLAN_TOPOLOGY_INVALID_ARGUMENT, 0, 0);
    }
    Workspace w = {0};
    PlanTopology candidate = {0};
    PlanTopologyResult status = result(PLAN_TOPOLOGY_SUCCESS, 0, 0);
    w.sources = allocate_array(source_count, sizeof *w.sources, &status);
    if (source_count != 0 && w.sources == NULL) { goto cleanup; }
    if (source_count != 0) { memcpy(w.sources, sources, source_count * sizeof *sources); }
    if (source_count > 1) { qsort(w.sources, source_count, sizeof *w.sources, source_compare); }
    for (size_t i = 0; i < source_count; i++) {
        PlanTopologySource source = w.sources[i];
        if ((source.kind != PLAN_TOPOLOGY_SOURCE_WALL && source.kind != PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR) ||
            source.source_id == DOMAIN_ID_INVALID || !plan_segment_valid(source.segment)) {
            status = result(PLAN_TOPOLOGY_INVALID_SOURCE, source.source_id, 0); goto cleanup;
        }
        for (size_t j = 0; j < i; j++) {
            if (source.source_id == w.sources[j].source_id) {
                status = result(PLAN_TOPOLOGY_INVALID_SOURCE, source.source_id, source.source_id); goto cleanup;
            }
        }
        if (!append_cut(&w, (Cut){i, topology_rational(topology_int_from_i64(0), topology_uint_from_u64(1)), topology_integer_point(source.segment.start)}, &status) ||
            !append_cut(&w, (Cut){i, topology_rational(topology_int_from_i64(1), topology_uint_from_u64(1)), topology_integer_point(source.segment.end)}, &status)) { goto cleanup; }
    }
    for (size_t i = 0; i < source_count; i++) {
        for (size_t j = i + 1; j < source_count; j++) {
            if (!intersect(&w, i, j, &status)) { goto cleanup; }
        }
    }
    if (!subdivide(&w, &candidate, &status) || !connect(&w, &candidate, &status) ||
        !supported_faces(&w, &candidate, &status) || !export_faces(&w, &candidate, &status)) { goto cleanup; }
    plan_topology_destroy(output);
    *output = candidate;
    candidate = (PlanTopology){0};
cleanup:
    plan_topology_destroy(&candidate);
    workspace_destroy(&w);
    return status;
}
