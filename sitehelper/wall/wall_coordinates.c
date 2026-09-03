#include "wall_coordinates.h"

Position wall_local_to_world_position(
    const Wall *wall,
    Position local_position
)
{
    if (wall == NULL) {
        return local_position;
    }

    return (Position){
        .x = wall->definition.origin.x + local_position.x,
        .y = wall->definition.origin.y + local_position.y
    };
}

Position wall_world_to_local_position(
    const Wall *wall,
    Position world_position
)
{
    if (wall == NULL) {
        return world_position;
    }

    return (Position){
        .x = world_position.x - wall->definition.origin.x,
        .y = world_position.y - wall->definition.origin.y
    };
}
