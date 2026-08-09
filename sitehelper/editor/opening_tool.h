#ifndef OPENING_TOOL_H
#define OPENING_TOOL_H

#include "geometry.h"
#include "sitehelper_model.h"

typedef struct
{
    int active;

    OpeningType type;

    int width;
    int height;
    int bottom;

    Vec2 preview_position;
    int preview_valid;
} OpeningTool;

void opening_tool_init(
    OpeningTool *tool
);

void opening_tool_activate(
    OpeningTool *tool
);

void opening_tool_cancel(
    OpeningTool *tool
);

void opening_tool_update_preview(
    OpeningTool *tool,
    Vec2 position
);

int opening_tool_preview_rect(
    const OpeningTool *tool,
    Rect2 *rect
);

#endif