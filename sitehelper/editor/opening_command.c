#include "opening_command.h"

int opening_command_create(
    const OpeningPlacement *placement,
    const OpeningTool *tool,
    OpeningCommand *command
)
{
    if (
        placement == NULL
        || tool == NULL
        || command == NULL
        || !placement->valid
        || placement->width <= 0
        || placement->height <= 0
    ) {
        return 0;
    }

    *command = (OpeningCommand){
        .opening = {
            .type = tool->type,

            .frame_position =
                (int)placement->left,

            .frame_bottom =
                (int)placement->bottom,

            .width =
                placement->width,

            .height =
                placement->height,

            .width_allowance = 0,
            .height_allowance = 0,

            .custom_allowance = false
        }
    };

    return 1;
}

int opening_command_execute(
    Wall *wall,
    const BuildSettings *settings,
    const OpeningCommand *command
)
{
    if (
        wall == NULL
        || settings == NULL
        || command == NULL
    ) {
        return 0;
    }

    return wall_add_opening(
        wall,
        settings,
        command->opening.type,
        command->opening.frame_position,
        command->opening.frame_bottom,
        command->opening.width,
        command->opening.height
    );
}