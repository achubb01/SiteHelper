#include <stdlib.h>
#include "plan_topology.h"
#include "storey.h"

PlanTopologyResult plan_topology_build_from_storey(const Storey *storey,
    PlanTopology *output)
{
    if (storey == NULL || output == NULL) { return (PlanTopologyResult){PLAN_TOPOLOGY_INVALID_ARGUMENT, 0, 0}; }
    const BuildStructure *structure = &storey->structure;
    if (structure->wall_count > structure->wall_capacity ||
        structure->room_separator_count > structure->room_separator_capacity ||
        (structure->wall_count != 0 && structure->walls == NULL) ||
        (structure->room_separator_count != 0 && structure->room_separators == NULL)) {
        return (PlanTopologyResult){PLAN_TOPOLOGY_INVALID_SOURCE, 0, 0};
    }
    if (structure->wall_count > SIZE_MAX - structure->room_separator_count) {
        return (PlanTopologyResult){PLAN_TOPOLOGY_NUMERIC_OVERFLOW, 0, 0};
    }
    size_t count = structure->wall_count + structure->room_separator_count;
    if (count > SIZE_MAX / sizeof(PlanTopologySource)) {
        return (PlanTopologyResult){PLAN_TOPOLOGY_NUMERIC_OVERFLOW, 0, 0};
    }
    PlanTopologySource *sources = count == 0 ? NULL : malloc(count * sizeof *sources);
    if (count != 0 && sources == NULL) { return (PlanTopologyResult){PLAN_TOPOLOGY_ALLOCATION_FAILED, 0, 0}; }
    for (size_t i = 0; i < structure->wall_count; i++) {
        const Wall *wall = &structure->walls[i];
        sources[i] = (PlanTopologySource){PLAN_TOPOLOGY_SOURCE_WALL, wall->id,
            {wall->definition.segment.start, wall->definition.segment.end}};
    }
    for (size_t i = 0; i < structure->room_separator_count; i++) {
        const RoomSeparator *separator = &structure->room_separators[i];
        sources[structure->wall_count + i] = (PlanTopologySource){
            PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR, separator->id, separator->segment};
    }
    PlanTopologyResult status = plan_topology_build(sources, count, output);
    free(sources);
    return status;
}
