#include "opening_placement.h"

static OpeningPlacement invalid_placement(void)
{
    return (OpeningPlacement){
        .valid = 0,
        .start_bay_index = 0,
        .end_bay_index = 0,
        .preview = {0}
    };
}

static int find_bay_index(
    const Wall *wall,
    double x,
    size_t *bay_index
)
{
    if (
        wall == NULL
        || bay_index == NULL
        || wall->studs == NULL
        || wall->stud_count < 2
    ) {
        return 0;
    }

    for (
        size_t i = 0;
        i + 1 < wall->stud_count;
        i++
    ) {
        double left =
            (double)wall->studs[i].position.x;

        double right =
            (double)wall->studs[i + 1].position.x;

        if (
            x >= left
            && x <= right
        ) {
            *bay_index = i;
            return 1;
        }
    }

    return 0;
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

    double opening_left =
        position.x;

    double opening_right =
        opening_left
        + (double)tool->width;

    double wall_left =
        (double)wall->studs[0].position.x;

    double wall_right =
        (double)wall->studs[
            wall->stud_count - 1
        ].position.x;

    if (
        opening_left < wall_left
        || opening_right > wall_right
    ) {
        return invalid_placement();
    }

    size_t start_bay_index;
    size_t end_bay_index;

    if (
        !find_bay_index(
            wall,
            opening_left,
            &start_bay_index
        )
        ||
        !find_bay_index(
            wall,
            opening_right,
            &end_bay_index
        )
    ) {
        return invalid_placement();
    }

    return (OpeningPlacement){
        .valid = 1,

        .start_bay_index =
            start_bay_index,

        .end_bay_index =
            end_bay_index,

        .preview = {
            .position = {
                .x = opening_left,
                .y = (double)tool->bottom
            },

            .width =
                (double)tool->width,

            .height =
                (double)tool->height
        }
    };
}