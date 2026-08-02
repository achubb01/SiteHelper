#include <stdlib.h>

#include "wall.h"
#include "wall_internal.h"

static int noggin_intersects_opening(
    const Wall *wall,
    size_t bay,
    int vertical_position,
    const BuildSettings *settings
);

int wall_generate_noggins(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (settings->stud_height <= 0 ||
        settings->nog_spacing <= 0) {
        return 0;
    }

    if (wall->stud_count < 2) {
        return 0;
    }

    int gaps =
        (settings->stud_height +
         settings->nog_spacing - 1)
        /
        settings->nog_spacing;

    for (int row = 1;
         row < gaps;
         row++) {

        int vertical_position =
            (settings->stud_height * row)
            / gaps;

        for (size_t bay = 0;
            bay + 1 < wall->stud_count;
            bay++) {

            Timber *left =
                &wall->studs[bay];

            Timber *right =
                &wall->studs[bay + 1];

            int clear_width =
                right->position.x
                - left->position.x
                - settings->stud_width;

            if (clear_width <= 0) {
                continue;
            }

            /*
            * Both vertical members must physically
            * reach this noggin row.
            */
            if (left->length <= vertical_position ||
                right->length <= vertical_position) {
                continue;
            }

            if (noggin_intersects_opening(
                    wall,
                    bay,
                    vertical_position,
                    settings)) {
                continue;
            }

            if (!wall_add_noggin(
                    wall,
                    settings,
                    bay,
                    vertical_position)) {

                return 0;
            }
        }
    }

    return 1;
}

static int noggin_intersects_opening(
    const Wall *wall,
    size_t bay,
    int vertical_position,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (bay + 1 >= wall->stud_count) {
        return 0;
    }

    const Timber *left_stud =
        &wall->studs[bay];

    const Timber *right_stud =
        &wall->studs[bay + 1];

    int noggin_start =
        left_stud->position.x
        + settings->stud_width;

    int noggin_end =
        right_stud->position.x;

    for (size_t i = 0;
         i < wall->opening_count;
         i++) {

        const Opening *opening =
            &wall->openings[i];

        int opening_left =
            opening->frame_position;

        int opening_right =
            opening_left +
            opening_frame_width(
                opening,
                settings
            );

        int opening_bottom =
            opening->frame_bottom;

        int opening_top =
            opening_bottom +
            opening_frame_height(
                opening,
                settings
            );

        int horizontal_overlap =
            noggin_start < opening_right &&
            noggin_end > opening_left;

        int vertical_overlap =
            vertical_position > opening_bottom &&
            vertical_position < opening_top;

        if (horizontal_overlap &&
            vertical_overlap) {

            return 1;
        }
    }

    return 0;
}

void wall_clear_noggins(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->nog_count = 0;
}

int wall_add_noggin(
    Wall *wall,
    const BuildSettings *settings,
    size_t bay,
    int vertical_position
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (bay + 1 >= wall->stud_count) {
        return 0;
    }

    Timber *left =
        &wall->studs[bay];

    Timber *right =
        &wall->studs[bay + 1];

    int length =
        right->position.x
        - left->position.x
        - settings->stud_width;

    if (length <= 0) {
        return 0;
    }

    if (wall->nog_count == wall->nog_capacity) {

        size_t new_capacity =
            wall->nog_capacity == 0
                ? 1
                : wall->nog_capacity * 2;

        Timber *new_nogs = realloc(
            wall->nogs,
            new_capacity * sizeof *new_nogs
        );

        if (new_nogs == NULL) {
            return 0;
        }

        wall->nogs = new_nogs;
        wall->nog_capacity = new_capacity;
    }

    Timber noggin = {
        .length = length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = left->position.x
                + settings->stud_width,
            .y = vertical_position
        },

        .type = TIMBER_NOGGIN,

        .details.noggin = {
            .bay = bay
        }
    };

    wall->nogs[wall->nog_count] = noggin;
    wall->nog_count++;

    return 1;
}