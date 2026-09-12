#include "plan_snap.h"
#include "wall_plan_transform.h"

#include <math.h>
#ifdef SITEHELPER_HAS_TOPOLOGY
#include <stdlib.h>
#include "wall_junctions.h"
#endif

typedef struct {
    int found;
    Vec2 position;
    double distance_squared;
} NearestPoint;

static void consider(NearestPoint *best, Vec2 pointer, Vec2 point)
{
    double dx = pointer.x - point.x, dy = pointer.y - point.y;
    double distance = dx * dx + dy * dy;
    if (!isfinite(point.x) || !isfinite(point.y) || !isfinite(distance)) { return; }
    if (!best->found || distance < best->distance_squared ||
        (distance == best->distance_squared &&
            (point.x < best->position.x ||
                (point.x == best->position.x && point.y < best->position.y)))) {
        *best = (NearestPoint){1, point, distance};
    }
}

static void consider_centreline(NearestPoint *best, Vec2 pointer, WallPlanSegment segment)
{
    double u;
    PlanPoint point;
    if (!wall_plan_segment_plan_to_u(segment, (PlanPoint){pointer.x, pointer.y}, &u)) { return; }
    int length = wall_plan_segment_length_mm(segment);
    /* Preserve authoritative endpoints exactly when the projection clamps. */
    if (u <= 0) { point = (PlanPoint){segment.start.x, segment.start.y}; }
    else if (u >= length) { point = (PlanPoint){segment.end.x, segment.end.y}; }
    else if (!wall_plan_segment_u_to_plan(segment, u, &point)) { return; }
    consider(best, pointer, (Vec2){point.x, point.y});
}

#ifdef SITEHELPER_HAS_TOPOLOGY
/* The exact topology stays exact. Only this transient presentation/query
 * boundary converts its derived millimetres to floating-point Plan space. */
static double rational_to_double(PlanTopologyRational value)
{
    double numerator = ldexp((double)value.numerator.hi, 64) + (double)value.numerator.lo;
    double denominator = ldexp((double)value.denominator.hi, 64) + (double)value.denominator.lo;
    double result = numerator / denominator;
    return value.negative ? -result : result;
}

static void collect_intersection(const BuildStructure *structure, Vec2 pointer, NearestPoint *best)
{
    size_t count = structure->wall_count;
    if (count < 2 || count > SIZE_MAX / sizeof(PlanTopologySource)) { return; }
    PlanTopologySource *sources = malloc(count * sizeof *sources);
    if (sources == NULL) { return; }
    for (size_t i = 0; i < count; i++) {
        const Wall *wall = &structure->walls[i];
        sources[i] = (PlanTopologySource){PLAN_TOPOLOGY_SOURCE_WALL, wall->id,
            {wall->definition.segment.start, wall->definition.segment.end}};
    }
    PlanTopology topology = {0};
    WallJunctionSet junctions = {0};
    if (plan_topology_build(sources, count, &topology).code == PLAN_TOPOLOGY_SUCCESS &&
        wall_junctions_build(&topology, &junctions) == WALL_JUNCTION_SUCCESS) {
        for (size_t i = 0; i < junctions.junction_count; i++) {
            PlanTopologyVertex point = junctions.junctions[i].position;
            consider(best, pointer, (Vec2){rational_to_double(point.x), rational_to_double(point.y)});
        }
    }
    wall_junctions_destroy(&junctions);
    plan_topology_destroy(&topology);
    free(sources);
}
#endif

size_t plan_collect_snap_candidates(const Storey *storey, Vec2 position,
    const SnapSettings *settings, SnapCandidate candidates[PLAN_SNAP_CANDIDATE_CAPACITY])
{
    if (storey == NULL || settings == NULL || candidates == NULL ||
        !isfinite(position.x) || !isfinite(position.y)) { return 0; }
    const BuildStructure *structure = &storey->structure;
    if (structure->wall_count > structure->wall_capacity ||
        (structure->wall_count != 0 && structure->walls == NULL)) { return 0; }

    NearestPoint endpoint = {0}, centreline = {0}, intersection = {0};
    for (size_t i = 0; i < structure->wall_count; i++) {
        WallPlanSegment segment = structure->walls[i].definition.segment;
        if (settings->endpoint_enabled) {
            consider(&endpoint, position, (Vec2){segment.start.x, segment.start.y});
            consider(&endpoint, position, (Vec2){segment.end.x, segment.end.y});
        }
        if (settings->wall_centreline_enabled) { consider_centreline(&centreline, position, segment); }
    }
#ifdef SITEHELPER_HAS_TOPOLOGY
    if (settings->intersection_enabled) { collect_intersection(structure, position, &intersection); }
#endif
    size_t count = 0;
    if (endpoint.found) { candidates[count++] = (SnapCandidate){endpoint.position, SNAP_ENDPOINT}; }
    if (centreline.found) { candidates[count++] = (SnapCandidate){centreline.position, SNAP_WALL_CENTRELINE}; }
    if (intersection.found) { candidates[count++] = (SnapCandidate){intersection.position, SNAP_INTERSECTION}; }
    return count;
}
