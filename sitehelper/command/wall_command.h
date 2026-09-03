#ifndef WALL_COMMAND_H
#define WALL_COMMAND_H

#include "sitehelper_project.h"

typedef struct
{
    DomainId room_id;
    Position origin;
    int length;
} WallCommand;

int wall_command_create(
    DomainId room_id,
    Position origin,
    int length,
    WallCommand *command
);

int wall_command_execute(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId *wall_id_out
);

int wall_command_undo(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId wall_id
);

int wall_command_redo(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId wall_id
);

#endif
