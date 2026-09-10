#ifndef ROOM_REGION_H
#define ROOM_REGION_H

#include "plan_topology.h"

typedef enum {
    ROOM_REGION_UNPLACED = 0,
    ROOM_REGION_BOUNDED,
    ROOM_REGION_UNBOUNDED,
    ROOM_REGION_ON_BOUNDARY,
    ROOM_REGION_INVALID_ARGUMENT,
    ROOM_REGION_ROOM_NOT_FOUND,
    ROOM_REGION_TOPOLOGY_FAILED
} RoomRegionCode;

typedef struct {
    RoomRegionCode code;
    DomainId room_id;
    /* Borrowed, only for spatial answers. No allocations or ownership here.
     * face_index is 0 for exterior, SIZE_MAX for boundary/unplaced/failure.
     * Both expire when the caller destroys/rebuilds the topology. */
    const PlanTopology *topology;
    size_t face_index;
    PlanTopologyResult topology_result; /* Original failure details, or SUCCESS. */
} RoomRegionResult;

/* Read-only, allocation-free. Resolve Room by stable ID, then query the
 * caller's geometry snapshot. Does not rebuild or check snapshot freshness.
 * UNPLACED does not inspect topology (NULL is allowed in that case).
 * A BOUNDED result directly traverses topology->faces[face_index] using its
 * existing boundary/step ranges. It is not a persistent RoomBoundary or ID.
 * Changing Room placement requires another query; changing source geometry
 * requires a successful rebuild before querying the new geometry.
 * Multiple Rooms may resolve to the same face; no uniqueness rule is applied. */
RoomRegionResult room_region_resolve(const SiteHelperProject *project,
    DomainId room_id, const PlanTopology *topology);

/* Explicit convenience operation for one Room. Output is caller-owned and
 * follows plan_topology_build's initialization/transaction contract. UNPLACED
 * leaves it untouched without examining geometry. Placed Rooms build once;
 * build failures return TOPOLOGY_FAILED with the exact subsystem result and
 * preserve existing output. No stale-output fallback classification occurs.
 * For multiple Rooms, build once and call room_region_resolve for each. */
RoomRegionResult room_region_build_and_resolve(const SiteHelperProject *project,
    DomainId room_id, PlanTopology *output);

#endif
