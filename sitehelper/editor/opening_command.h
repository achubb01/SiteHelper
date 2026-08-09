#ifndef OPENING_COMMAND_H
#define OPENING_COMMAND_H

#include "opening_placement.h"
#include "opening_tool.h"
#include "wall.h"

typedef struct
{
    Opening opening;
} OpeningCommand;

int opening_command_create(
    const OpeningPlacement *placement,
    const OpeningTool *tool,
    OpeningCommand *command
);

int opening_command_execute(
    Wall *wall,
    const BuildSettings *settings,
    const OpeningCommand *command
);

#endif