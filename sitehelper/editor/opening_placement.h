#ifndef OPENING_PLACEMENT_H
#define OPENING_PLACEMENT_H

#include <stddef.h>

#include "geometry.h"
#include "opening_tool.h"
#include "wall.h"

typedef struct
{
    int valid;

    size_t bay_index;

    Rect2 preview;
} OpeningPlacement;

OpeningPlacement opening_find_placement(
    const Wall *wall,
    Vec2 position,
    const OpeningTool *tool
);

#endif