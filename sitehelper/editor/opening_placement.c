#include "opening_placement.h"

static OpeningPlacement invalid_placement(void)
{
    return (OpeningPlacement){
        .valid = 0,
        .bay_index = 0,
        .preview = {0}
    };
}

OpeningPlacement opening_find_placement(
    const Wall *wall,
    Vec2 position,
    const OpeningTool *tool
)
{
    if (
        wall == NULL
        || tool == NULL
        || wall->studs == NULL
        || wall->stud_count < 2
        || tool->width <= 0
        || tool->height <= 0
    ) {
        return invalid_placement();
    }

    for (
        size_t i = 0;
        i + 1 < wall->stud_count;
        i++
    ) {
        const Timber *left =
            &wall->studs[i];

        const Timber *right =
            &wall->studs[i + 1];

        double bay_left =
            (double)left->position.x
            + (double)left->width;

        double bay_right =
            (double)right->position.x;

        if (
            position.x < bay_left
            || position.x > bay_right
        ) {
            continue;
        }

        double bay_width =
            bay_right - bay_left;

        if ((double)tool->width > bay_width) {
            return invalid_placement();
        }

        return (OpeningPlacement){
            .valid = 1,
            .bay_index = i,
            .preview = {
                .position = {
                    .x = bay_left,
                    .y = (double)tool->bottom
                },
                .width = (double)tool->width,
                .height = (double)tool->height
            }
        };
    }

    return invalid_placement();
}