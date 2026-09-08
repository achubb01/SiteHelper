#ifndef MOVE_WALL_ENDPOINT_COMMAND_H
#define MOVE_WALL_ENDPOINT_COMMAND_H

#include "sitehelper_project.h"

typedef enum
{
    WALL_ENDPOINT_START,
    WALL_ENDPOINT_END
} WallEndpoint;

typedef struct
{
    DomainId wall_id;
    WallEndpoint endpoint;
    PlanPosition new_position;
} MoveWallEndpointCommand;

int move_wall_endpoint_command_create(DomainId wall_id, WallEndpoint endpoint,
    PlanPosition new_position, MoveWallEndpointCommand *command);
int move_wall_endpoint_command_execute(SiteHelperProject *project,
    const MoveWallEndpointCommand *command);

#endif
