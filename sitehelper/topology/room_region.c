#include "room_region.h"
#include "wall.h"

static RoomRegionResult initial_result(DomainId room_id)
{
    return (RoomRegionResult){.code = ROOM_REGION_INVALID_ARGUMENT,
        .room_id = room_id, .face_index = SIZE_MAX,
        .topology_result = {PLAN_TOPOLOGY_SUCCESS, 0, 0}};
}

static const Room *resolve_room(const SiteHelperProject *project, DomainId room_id,
    RoomRegionResult *result)
{
    if (project == NULL || project->structure.room_count > project->structure.room_capacity ||
        (project->structure.room_count != 0 && project->structure.rooms == NULL)) { return NULL; }
    const Room *room = build_find_room_by_id_const(&project->structure, room_id);
    if (room == NULL) { result->code = ROOM_REGION_ROOM_NOT_FOUND; }
    else if (!room->has_location) { result->code = ROOM_REGION_UNPLACED; }
    return room;
}

static RoomRegionResult resolve_placed(const Room *room, const PlanTopology *topology)
{
    RoomRegionResult result = initial_result(room->id);
    PlanTopologyPointResult point = plan_topology_find_face_at_plan_position(topology, room->location);
    if (point.code != PLAN_TOPOLOGY_SUCCESS) {
        result.code = ROOM_REGION_TOPOLOGY_FAILED;
        result.topology_result.code = point.code;
        return result;
    }
    switch (point.state) {
    case PLAN_TOPOLOGY_POINT_BOUNDED: result.code = ROOM_REGION_BOUNDED; break;
    case PLAN_TOPOLOGY_POINT_UNBOUNDED: result.code = ROOM_REGION_UNBOUNDED; break;
    case PLAN_TOPOLOGY_POINT_ON_BOUNDARY: result.code = ROOM_REGION_ON_BOUNDARY; break;
    case PLAN_TOPOLOGY_POINT_UNCLASSIFIED: return result; /* No successful query produces this. */
    }
    result.topology = topology;
    result.face_index = point.face_index;
    return result;
}

RoomRegionResult room_region_resolve(const SiteHelperProject *project,
    DomainId room_id, const PlanTopology *topology)
{
    RoomRegionResult result = initial_result(room_id);
    const Room *room = resolve_room(project, room_id, &result);
    if (room == NULL || !room->has_location) { return result; }
    return resolve_placed(room, topology);
}

RoomRegionResult room_region_build_and_resolve(const SiteHelperProject *project,
    DomainId room_id, PlanTopology *output)
{
    RoomRegionResult result = initial_result(room_id);
    const Room *room = resolve_room(project, room_id, &result);
    if (room == NULL || !room->has_location) { return result; }
    result.topology_result = plan_topology_build_from_project(project, output);
    if (result.topology_result.code != PLAN_TOPOLOGY_SUCCESS) {
        result.code = ROOM_REGION_TOPOLOGY_FAILED;
        return result;
    }
    return resolve_placed(room, output);
}
