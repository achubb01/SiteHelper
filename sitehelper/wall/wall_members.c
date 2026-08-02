#include <stdlib.h>

#include "wall.h"
#include "wall_internal.h"

void wall_clear_members(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->member_count = 0;
}

int wall_add_member(
    Wall *wall,
    Timber member
)
{
    if (wall == NULL) {
        return 0;
    }

    if (wall->member_count ==
        wall->member_capacity) {

        size_t new_capacity =
            wall->member_capacity == 0
                ? 1
                : wall->member_capacity * 2;

        Timber *new_members = realloc(
            wall->members,
            new_capacity *
            sizeof *new_members
        );

        if (new_members == NULL) {
            return 0;
        }

        wall->members = new_members;
        wall->member_capacity =
            new_capacity;
    }

    wall->members[
        wall->member_count
    ] = member;

    wall->member_count++;

    return 1;
}

int wall_add_header(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
)
{
    if (wall == NULL ||
        settings == NULL ||
        opening == NULL) {
        return 0;
    }

    int frame_width =
        opening_frame_width(
            opening,
            settings
        );

    int frame_height =
        opening_frame_height(
            opening,
            settings
        );

    if (frame_width <= 0 ||
        frame_height <= 0) {
        return 0;
    }

    int left_trimmer_position =
        opening->frame_position -
        settings->stud_width;

    int header_length =
        frame_width +
        (2 * settings->stud_width);

    int header_y =
        opening->frame_bottom +
        frame_height;

    if (left_trimmer_position < 0) {
        return 0;
    }

    if (header_y < 0 ||
        header_y > settings->stud_height) {
        return 0;
    }

    Timber header = {
        .length = header_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = left_trimmer_position,
            .y = header_y
        },

        .type = TIMBER_HEADER
    };

    return wall_add_member(
        wall,
        header
    );
}

int wall_add_sill(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
)
{
    if (wall == NULL ||
        settings == NULL ||
        opening == NULL) {
        return 0;
    }

    if (opening->type != OPENING_WINDOW) {
        return 0;
    }

    int sill_length =
        opening_frame_width(
            opening,
            settings
        );

    if (sill_length <= 0 ||
        opening->frame_bottom <= 0) {
        return 0;
    }

    Timber sill = {
        .length = sill_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = opening->frame_position,
            .y = opening->frame_bottom
        },

        .type = TIMBER_SILL
    };

    return wall_add_member(
        wall,
        sill
    );
}