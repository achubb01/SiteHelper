#include <stdlib.h>

#include "opening_tool.h"

void opening_tool_init(
    OpeningTool *tool
)
{
    if (tool == NULL) {
        return;
    }

    *tool = (OpeningTool){
        .active = 0,
        .type = OPENING_WINDOW,
        .width = 1200,
        .height = 1200,
        .bottom = 900,
        .preview_position = {0.0, 0.0},
        .preview_valid = 0
    };
}

void opening_tool_activate(
    OpeningTool *tool
)
{
    if (tool == NULL) {
        return;
    }

    tool->active = 1;
}

void opening_tool_cancel(
    OpeningTool *tool
)
{
    if (tool == NULL) {
        return;
    }

    tool->active = 0;
    tool->preview_valid = 0;
}

void opening_tool_update_preview(
    OpeningTool *tool,
    Vec2 position
)
{
    if (
        tool == NULL
        || !tool->active
    ) {
        return;
    }

    tool->preview_position = position;
    tool->preview_valid = 1;
}

int opening_tool_preview_rect(
    const OpeningTool *tool,
    Rect2 *rect
)
{
    if (
        tool == NULL
        || rect == NULL
        || !tool->active
        || !tool->preview_valid
    ) {
        return 0;
    }

    *rect = (Rect2){
        .position = {
            .x = tool->preview_position.x,
            .y = (double)tool->bottom
        },
        .width = (double)tool->width,
        .height = (double)tool->height
    };

    return 1;
}