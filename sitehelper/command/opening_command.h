#ifndef OPENING_COMMAND_H
#define OPENING_COMMAND_H

#include "sitehelper_project.h"
#include "opening.h"

typedef struct
{

    DomainId wall_id;

    OpeningType type;

    int frame_position;
    int frame_bottom;

    int width;
    int height;
} OpeningCommand;

int opening_command_create(
    DomainId wall_id,
    OpeningType type,
    int frame_position,
    int frame_bottom,
    int width,
    int height,
    OpeningCommand *command
);

int opening_command_execute(
    SiteHelperProject *project,
    const OpeningCommand *command,
    DomainId *opening_id_out
);

int opening_command_undo(
    SiteHelperProject *project,
    const OpeningCommand *command,
    DomainId opening_id
);

int opening_command_redo(
    SiteHelperProject *project,
    const OpeningCommand *command,
    DomainId opening_id
);

#endif
