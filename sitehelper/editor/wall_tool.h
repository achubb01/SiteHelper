#ifndef WALL_TOOL_H
#define WALL_TOOL_H

#include "geometry.h"
#include "position.h"

typedef struct
{
    int active;
    int has_start;
    Vec2 start;
    Vec2 endpoint;
} WallTool;

void wall_tool_init(WallTool *tool);
void wall_tool_activate(WallTool *tool);
void wall_tool_cancel(WallTool *tool);
void wall_tool_update(WallTool *tool, Vec2 endpoint);
int wall_tool_begin(WallTool *tool, Vec2 start);
int wall_tool_command_data(
    const WallTool *tool,
    Position *origin,
    int *length
);
int wall_tool_preview_rect(const WallTool *tool, Rect2 *rect);

#endif
