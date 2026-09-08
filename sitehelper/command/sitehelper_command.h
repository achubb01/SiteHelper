#ifndef SITEHELPER_COMMAND_H
#define SITEHELPER_COMMAND_H

#include "opening_command.h"
#include "wall_command.h"
#include "delete_wall_command.h"
#include "move_wall_endpoint_command.h"
#include "sitehelper_project.h"

typedef enum
{
    SITEHELPER_COMMAND_NONE = 0,

    SITEHELPER_COMMAND_ADD_OPENING,
    SITEHELPER_COMMAND_ADD_WALL,
    SITEHELPER_COMMAND_DELETE_WALL,
    SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,

    SITEHELPER_COMMAND_COUNT
} SiteHelperCommandType;

typedef struct
{
    SiteHelperCommandType type;

    union
    {
        OpeningCommand opening;
        WallCommand wall;
        DeleteWallCommand delete_wall;
        MoveWallEndpointCommand move_wall_endpoint;
    } data;
} SiteHelperCommand;

typedef struct
{
    SiteHelperCommandType type;

    union
    {
        struct
        {
            DomainId room_id;
            DomainId wall_id;
            DomainId opening_id;
        } add_opening;

        struct
        {
            DomainId room_id;
            DomainId wall_id;
        } add_wall;

        struct
        {
            DomainId wall_id;
        } delete_wall;

        struct
        {
            DomainId wall_id;
        } move_wall_endpoint;

    } data;

} SiteHelperCommandResult;

int sitehelper_command_from_opening(
    const OpeningCommand *opening,
    SiteHelperCommand *command
);

int sitehelper_command_from_wall(
    const WallCommand *wall,
    SiteHelperCommand *command
);

int sitehelper_command_execute(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
);

int sitehelper_command_from_delete_wall(
    const DeleteWallCommand *deletion,
    SiteHelperCommand *command
);

int sitehelper_command_from_move_wall_endpoint(
    const MoveWallEndpointCommand *move, SiteHelperCommand *command);

/* Compact add-command undo. DELETE_WALL and MOVE_WALL_ENDPOINT require state
 * owned by command history; use history execute/undo for reversible edits. */
int sitehelper_command_undo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
);

int sitehelper_command_redo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
);

#endif
