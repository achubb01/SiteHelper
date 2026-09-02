#include "opening_placement.h"

static OpeningPlacement no_candidate_placement(void)
{
    return (OpeningPlacement){
        .has_candidate = 0
    };
}

OpeningPlacement opening_find_placement(
    Vec2 position,
    const OpeningTool *tool
)
{
    if (
        tool == NULL
        || tool->width <= 0
        || tool->height <= 0
    ) {
        return no_candidate_placement();
    }

    return (OpeningPlacement){
        .has_candidate = 1,
        .left = position.x,
        .bottom = (double)tool->bottom,
        .width = tool->width,
        .height = tool->height
    };
}

int opening_placement_is_valid(
    const OpeningPlacement *placement
)
{
    return placement != NULL
        && placement->has_candidate
        && placement->validation.code == WALL_OPENING_VALID;
}
