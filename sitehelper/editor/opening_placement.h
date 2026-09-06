#ifndef OPENING_PLACEMENT_H
#define OPENING_PLACEMENT_H

#include "geometry.h"
#include "opening_tool.h"
#include "wall.h"

typedef struct
{
    int has_candidate;

    double left;   /* Preview U in millimetres; committed dimensions are int. */
    double bottom; /* Preview Z in millimetres. */

    int width;
    int height;

    WallOpeningValidation validation;
} OpeningPlacement;

/* Input is local elevation geometry (x = U, y = Z), without layout offset. */
OpeningPlacement opening_find_placement(
    Vec2 position,
    const OpeningTool *tool
);

int opening_placement_is_valid(
    const OpeningPlacement *placement
);

#endif
