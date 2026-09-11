#include <stdlib.h>
#include <stdint.h>

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

    if (wall->framing.stud_count < 2) {
        return 0;
    }

    int gaps =
        (int)(((int64_t)settings->stud_height +
         settings->nog_spacing - 1)
        /
        settings->nog_spacing);

    for (int row = 1;
         row < gaps;
         row++) {

        int vertical_position =
            (int)(((int64_t)settings->stud_height * row)
            / gaps);

        for (size_t bay = 0;
            bay + 1 < wall->framing.stud_count;
            bay++) {

            Timber *left =
                &wall->framing.studs[bay];

            Timber *right =
                &wall->framing.studs[bay + 1];

            int clear_width =
                right->position.u
                - left->position.u
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

    if (bay + 1 >= wall->framing.stud_count) {
        return 0;
    }

    const Timber *left_stud =
        &wall->framing.studs[bay];

    const Timber *right_stud =
        &wall->framing.studs[bay + 1];

    int noggin_start =
        left_stud->position.u
        + settings->stud_width;

    int noggin_end =
        right_stud->position.u;

    for (size_t i = 0;
         i < wall->definition.opening_count;
         i++) {

        const Opening *opening =
            &wall->definition.openings[i];

        WallOpeningFrameGeometry frame;
        if (!wall_opening_frame_geometry(opening, settings, &frame)) { continue; }
        /* The sill occupies the band immediately below clear bottom; avoid
         * placing noggins through it as well as through the clear rectangle. */
        int64_t framing_bottom = frame.bottom_z -
            (opening->type == OPENING_WINDOW ? settings->stud_width : 0);
        int horizontal_overlap = noggin_start < frame.right_u && noggin_end > frame.left_u;
        int vertical_overlap = vertical_position > framing_bottom && vertical_position < frame.top_z;

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

    wall->framing.nog_count = 0;
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

    if (bay + 1 >= wall->framing.stud_count) {
        return 0;
    }

    Timber *left =
        &wall->framing.studs[bay];

    Timber *right =
        &wall->framing.studs[bay + 1];

    int length =
        right->position.u
        - left->position.u
        - settings->stud_width;

    if (length <= 0) {
        return 0;
    }

    if (wall->framing.nog_count == wall->framing.nog_capacity) {

        size_t new_capacity =
            wall->framing.nog_capacity == 0
                ? 1
                : wall->framing.nog_capacity * 2;

        Timber *new_nogs = realloc(
            wall->framing.nogs,
            new_capacity * sizeof *new_nogs
        );

        if (new_nogs == NULL) {
            return 0;
        }

        wall->framing.nogs = new_nogs;
        wall->framing.nog_capacity = new_capacity;
    }

    Timber noggin = {
        .length = length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .u = left->position.u
                + settings->stud_width,
            .z = vertical_position
        },

        .type = TIMBER_NOGGIN,

        .details.noggin = {
            .bay = bay
        }
    };

    wall->framing.nogs[wall->framing.nog_count] = noggin;
    wall->framing.nog_count++;

    return 1;
}
