#include "wall_tool.h"

#include <stddef.h>

void wall_tool_init(WallTool *tool)
{
    if (tool != NULL) {
        *tool = (WallTool){0};
    }
}

void wall_tool_activate(WallTool *tool)
{
    if (tool != NULL) {
        tool->active = 1;
    }
}

void wall_tool_cancel(WallTool *tool)
{
    if (tool != NULL) {
        tool->has_start = 0;
    }
}

void wall_tool_update(WallTool *tool, Vec2 endpoint)
{
    if (tool != NULL && tool->active && tool->has_start) {
        tool->endpoint = endpoint;
    }
}

int wall_tool_begin(WallTool *tool, Vec2 start)
{
    if (tool == NULL || !tool->active) {
        return 0;
    }

    tool->start = start;
    tool->endpoint = start;
    tool->has_start = 1;
    return 1;
}

int wall_tool_command_data(
    const WallTool *tool,
    Position *origin,
    int *length
)
{
    if (tool == NULL || origin == NULL || length == NULL ||
        !tool->active || !tool->has_start) {

        return 0;
    }

    double left = tool->start.x < tool->endpoint.x
        ? tool->start.x
        : tool->endpoint.x;
    double difference = tool->endpoint.x - tool->start.x;
    double width = difference < 0.0 ? -difference : difference;

    if (width < 1.0) {
        return 0;
    }

    *origin = (Position){
        .x = (int)left,
        .y = (int)tool->start.y
    };
    *length = (int)width;
    return *length > 0;
}

int wall_tool_preview_rect(const WallTool *tool, Rect2 *rect)
{
    Position origin;
    int length;

    if (!wall_tool_command_data(tool, &origin, &length)) {
        return 0;
    }

    *rect = (Rect2){
        .position = { .x = origin.x, .y = origin.y },
        .width = length,
        .height = 20.0
    };
    return 1;
}
