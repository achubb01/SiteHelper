#include "wall_tool.h"

#include <stddef.h>
#include <limits.h>
#include <math.h>

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
    WallPlanSegment *segment
)
{
    if (tool == NULL || segment == NULL ||
        !tool->active || !tool->has_start) {

        return 0;
    }

    const double coordinates[] = {
        tool->start.x, tool->start.y, tool->endpoint.x, tool->endpoint.y
    };
    for (size_t i = 0; i < sizeof coordinates / sizeof coordinates[0]; i++) {
        if (!isfinite(coordinates[i]) || coordinates[i] < INT_MIN ||
            coordinates[i] > INT_MAX) {
            return 0;
        }
    }

    WallPlanSegment candidate = {
        .start = { .x = (int)tool->start.x, .y = (int)tool->start.y },
        .end = { .x = (int)tool->endpoint.x, .y = (int)tool->endpoint.y }
    };
    if (wall_plan_segment_length_mm(candidate) == 0) {
        return 0;
    }
    *segment = candidate;
    return 1;
}
