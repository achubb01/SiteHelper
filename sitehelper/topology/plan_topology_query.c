#include "topology_numeric_internal.h"

static PlanTopologyPointResult point_result(PlanTopologyPointState state, size_t face)
{
    return (PlanTopologyPointResult){PLAN_TOPOLOGY_SUCCESS, state, face};
}

/* Using the retained integer supporting line avoids subtracting/multiplying
 * rational vertex coordinates. All differences are <= 2^32-1, so the cross
 * product magnitude is < 2^65, as in the builder's exact predicates. */
static TopologyInt source_side(const PlanTopologyEdge *edge, PlanPosition p)
{
    PlanSegment s = edge->source_segment;
    return topology_cross((int64_t)s.end.x - s.start.x, (int64_t)s.end.y - s.start.y,
        (int64_t)p.x - s.start.x, (int64_t)p.y - s.start.y);
}

static int between(PlanTopologyRational p, PlanTopologyRational a, PlanTopologyRational b)
{
    int pa = topology_rational_compare(p, a), pb = topology_rational_compare(p, b);
    return (pa >= 0 && pb <= 0) || (pa <= 0 && pb >= 0);
}

PlanTopologyPointResult plan_topology_find_face_at_plan_position(
    const PlanTopology *topology, PlanPosition position)
{
    if (topology == NULL || topology->face_count == 0 || topology->faces == NULL ||
        topology->faces[0].bounded ||
        (topology->vertex_count != 0 && topology->vertices == NULL) ||
        (topology->edge_count != 0 && topology->edges == NULL) ||
        (topology->boundary_count != 0 && topology->boundaries == NULL) ||
        (topology->step_count != 0 && topology->steps == NULL)) {
        return (PlanTopologyPointResult){PLAN_TOPOLOGY_INVALID_ARGUMENT,
            PLAN_TOPOLOGY_POINT_UNCLASSIFIED, SIZE_MAX};
    }
    PlanTopologyVertex p = topology_integer_point(position);
    for (size_t i = 0; i < topology->edge_count; i++) {
        const PlanTopologyEdge *edge = &topology->edges[i];
        PlanTopologyVertex a = topology->vertices[edge->start_vertex];
        PlanTopologyVertex b = topology->vertices[edge->end_vertex];
        if (topology_int_is_zero(source_side(edge, position)) && between(p.x, a.x, b.x) && between(p.y, a.y, b.y)) {
            return point_result(PLAN_TOPOLOGY_POINT_ON_BOUNDARY, SIZE_MAX);
        }
    }
    for (size_t f = 1; f < topology->face_count; f++) {
        PlanTopologyFace face = topology->faces[f];
        int inside = 0;
        for (size_t j = 0; j < face.boundary_count; j++) {
            PlanTopologyBoundary boundary = topology->boundaries[face.first_boundary + j];
            for (size_t k = 0; k < boundary.step_count; k++) {
                PlanTopologyBoundaryStep step = topology->steps[boundary.first_step + k];
                const PlanTopologyEdge *edge = &topology->edges[step.edge];
                size_t from = step.reversed ? edge->end_vertex : edge->start_vertex;
                size_t to = step.reversed ? edge->start_vertex : edge->end_vertex;
                int ay = topology_rational_compare(topology->vertices[from].y, p.y);
                int by = topology_rational_compare(topology->vertices[to].y, p.y);
                /* Half-open height test counts a shared vertex only once and
                 * excludes horizontal edges. Boundary equality was handled
                 * above, so a rightward ray crossing is strictly to one side. */
                if ((ay > 0) == (by > 0)) { continue; }
                TopologyInt side = source_side(edge, position);
                if (topology_rational_compare(edge->source_t_start, edge->source_t_end) > 0) { side = topology_int_negate(side); }
                if (step.reversed) { side = topology_int_negate(side); }
                if ((by > ay && topology_int_compare(side, topology_int_from_i64(0)) > 0) || (by < ay && topology_int_compare(side, topology_int_from_i64(0)) < 0)) { inside = !inside; }
            }
        }
        if (inside) { return point_result(PLAN_TOPOLOGY_POINT_BOUNDED, f); }
    }
    return point_result(PLAN_TOPOLOGY_POINT_UNBOUNDED, 0);
}
