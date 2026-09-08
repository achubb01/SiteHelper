#include "move_wall_endpoint_command.h"
#include "wall.h"

int move_wall_endpoint_command_create(DomainId wall_id, WallEndpoint endpoint,
    PlanPosition new_position, MoveWallEndpointCommand *command)
{
    if (command == NULL || wall_id == DOMAIN_ID_INVALID ||
        (endpoint != WALL_ENDPOINT_START && endpoint != WALL_ENDPOINT_END)) {
        return 0;
    }
    *command = (MoveWallEndpointCommand){
        .wall_id = wall_id, .endpoint = endpoint, .new_position = new_position
    };
    return 1;
}

int move_wall_endpoint_command_execute(SiteHelperProject *project,
    const MoveWallEndpointCommand *command)
{
    if (project == NULL || command == NULL) {
        return 0;
    }
    Wall *wall = build_find_wall_by_id(&project->structure, command->wall_id);
    if (wall == NULL) {
        return 0;
    }
    WallPlanSegment candidate = wall->definition.segment;
    switch (command->endpoint) {
        case WALL_ENDPOINT_START:
            candidate.start = command->new_position;
            break;
        case WALL_ENDPOINT_END:
            candidate.end = command->new_position;
            break;
        default:
            return 0;
    }
    return wall_apply_plan_segment(wall, &project->settings, candidate);
}
