#ifndef WALL_COORDINATES_H
#define WALL_COORDINATES_H

#include "wall.h"

Position wall_local_to_world_position(
    const Wall *wall,
    Position local_position
);

Position wall_world_to_local_position(
    const Wall *wall,
    Position world_position
);

#endif
