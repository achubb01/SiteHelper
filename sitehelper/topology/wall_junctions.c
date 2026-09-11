#include <stdlib.h>
#include "wall_junctions.h"
#include "topology_numeric_internal.h"

/* Temporary indices only; no incidence workspace escapes into the result. */
typedef struct {
    size_t vertex, edge;
    DomainId wall_id;
} Incidence;

static int incidence_compare(const void *pa, const void *pb)
{
    const Incidence *a = pa, *b = pb;
    if (a->vertex != b->vertex) { return a->vertex < b->vertex ? -1 : 1; }
    if (a->wall_id != b->wall_id) { return a->wall_id < b->wall_id ? -1 : 1; }
    return (a->edge > b->edge) - (a->edge < b->edge);
}

static void *allocate_array(size_t count, size_t size, WallJunctionCode *status)
{
    if (count == 0) { return NULL; }
    if (count > SIZE_MAX / size) {
        *status = WALL_JUNCTION_NUMERIC_OVERFLOW;
        return NULL;
    }
    void *storage = calloc(count, size);
    if (storage == NULL) { *status = WALL_JUNCTION_ALLOCATION_FAILED; }
    return storage;
}

void wall_junctions_destroy(WallJunctionSet *junctions)
{
    if (junctions == NULL) { return; }
    free(junctions->junctions);
    free(junctions->participants);
    *junctions = (WallJunctionSet){0};
}

static WallJunctionParticipant participant(const PlanTopology *topology, Incidence incidence)
{
    const PlanTopologyEdge *edge = &topology->edges[incidence.edge];
    /* Canonical edge direction can oppose the original Wall direction.
     * The parameters are paired with vertices, not increasing t order. */
    PlanTopologyRational t = incidence.vertex == edge->start_vertex ?
        edge->source_t_start : edge->source_t_end;
    WallJunctionParticipantPosition position = WALL_JUNCTION_PARTICIPANT_INTERIOR;
    if (topology_rational_compare(t, topology_rational(topology_int_from_i64(0), topology_uint_from_u64(1))) == 0) {
        position = WALL_JUNCTION_PARTICIPANT_START;
    } else if (topology_rational_compare(t, topology_rational(topology_int_from_i64(1), topology_uint_from_u64(1))) == 0) {
        position = WALL_JUNCTION_PARTICIPANT_END;
    }
    return (WallJunctionParticipant){incidence.wall_id, position, t};
}

static WallJunctionKind classify(const PlanTopology *topology, const Incidence *incidences,
    const WallJunctionParticipant *participants, size_t count)
{
    if (count > 2) { return WALL_JUNCTION_MULTIWAY; }
    int interiors = (participants[0].position == WALL_JUNCTION_PARTICIPANT_INTERIOR) +
        (participants[1].position == WALL_JUNCTION_PARTICIPANT_INTERIOR);
    if (interiors == 2) { return WALL_JUNCTION_CROSS; }
    if (interiors == 1) { return WALL_JUNCTION_T; }
    PlanSegment a = topology->edges[incidences[0].edge].source_segment;
    PlanSegment b = topology->edges[incidences[1].edge].source_segment;
    /* Topology already establishes the shared point. Only supporting-line
     * direction is needed here, using the existing exact numeric kernel.
     * Integer endpoint differences are <= 2^32-1, cross magnitude < 2^65. */
    return topology_int_is_zero(topology_cross((int64_t)a.end.x - a.start.x, (int64_t)a.end.y - a.start.y,
        (int64_t)b.end.x - b.start.x, (int64_t)b.end.y - b.start.y)) ?
        WALL_JUNCTION_CONTINUOUS : WALL_JUNCTION_CORNER;
}

WallJunctionCode wall_junctions_build(const PlanTopology *topology, WallJunctionSet *output)
{
    if (output == NULL || topology == NULL || topology->face_count == 0 ||
        topology->faces == NULL || topology->faces[0].bounded ||
        (topology->vertex_count != 0 && topology->vertices == NULL) ||
        (topology->edge_count != 0 && topology->edges == NULL)) {
        return WALL_JUNCTION_INVALID_ARGUMENT;
    }
    WallJunctionCode status = WALL_JUNCTION_SUCCESS;
    WallJunctionSet candidate = {0};
    Incidence *incidences = NULL;
    size_t wall_edges = 0;
    for (size_t i = 0; i < topology->edge_count; i++) {
        if (topology->edges[i].source_kind == PLAN_TOPOLOGY_SOURCE_WALL) { wall_edges++; }
    }
    if (wall_edges > SIZE_MAX / 2) { return WALL_JUNCTION_NUMERIC_OVERFLOW; }
    size_t count = 2 * wall_edges;
    incidences = allocate_array(count, sizeof *incidences, &status);
    if (count != 0 && incidences == NULL) { goto cleanup; }
    size_t offset = 0;
    for (size_t i = 0; i < topology->edge_count; i++) {
        const PlanTopologyEdge *edge = &topology->edges[i];
        if (edge->source_kind != PLAN_TOPOLOGY_SOURCE_WALL) { continue; }
        incidences[offset++] = (Incidence){edge->start_vertex, i, edge->source_id};
        incidences[offset++] = (Incidence){edge->end_vertex, i, edge->source_id};
    }
    if (count > 1) { qsort(incidences, count, sizeof *incidences, incidence_compare); }
    size_t unique = 0;
    for (size_t i = 0; i < count; i++) {
        if (unique == 0 || incidences[i].vertex != incidences[unique - 1].vertex ||
            incidences[i].wall_id != incidences[unique - 1].wall_id) {
            incidences[unique++] = incidences[i];
        }
    }
    /* Count exact owned sizes; both totals are bounded by unique <= count. */
    for (size_t first = 0; first < unique;) {
        size_t end = first + 1;
        while (end < unique && incidences[end].vertex == incidences[first].vertex) { end++; }
        if (end - first >= 2) {
            candidate.junction_count++;
            candidate.participant_count += end - first;
        }
        first = end;
    }
    candidate.junctions = allocate_array(candidate.junction_count, sizeof *candidate.junctions, &status);
    if (candidate.junction_count != 0 && candidate.junctions == NULL) { goto cleanup; }
    candidate.participants = allocate_array(candidate.participant_count, sizeof *candidate.participants, &status);
    if (candidate.participant_count != 0 && candidate.participants == NULL) { goto cleanup; }
    size_t junction = 0;
    offset = 0;
    for (size_t first = 0; first < unique;) {
        size_t end = first + 1;
        while (end < unique && incidences[end].vertex == incidences[first].vertex) { end++; }
        if (end - first >= 2) {
            for (size_t i = first; i < end; i++) {
                candidate.participants[offset + i - first] = participant(topology, incidences[i]);
            }
            candidate.junctions[junction++] = (WallJunction){
                .kind = classify(topology, incidences + first, candidate.participants + offset, end - first),
                .position = topology->vertices[incidences[first].vertex],
                .first_participant = offset, .participant_count = end - first
            };
            offset += end - first;
        }
        first = end;
    }
    wall_junctions_destroy(output);
    *output = candidate;
    candidate = (WallJunctionSet){0};
cleanup:
    free(incidences);
    wall_junctions_destroy(&candidate);
    return status;
}
