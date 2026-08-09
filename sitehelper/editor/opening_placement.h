#ifndef OPENING_PLACEMENT_H
#define OPENING_PLACEMENT_H

#include <stddef.h>

#include "geometry.h"
#include "opening_tool.h"
#include "wall.h"

typedef struct
{
    int valid;

    size_t start_bay_index;
    size_t end_bay_index;

    double left;
    double bottom;

    int width;
    int height;
} OpeningPlacement;

OpeningPlacement opening_find_placement(
    const Wall *wall,
    Vec2 position,
    const OpeningTool *tool
);

#endif