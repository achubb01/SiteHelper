#include <limits.h>
#include <stdlib.h>

#include "wall.h"
#include "wall_internal.h"

static int wall_add_member(
    Wall *wall,
    Timber member
);

void wall_clear_members(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->framing.member_count = 0;
}

static int wall_add_member(
    Wall *wall,
    Timber member
)
{
    if (wall == NULL) {
        return 0;
    }

    if (wall->framing.member_count ==
        wall->framing.member_capacity) {

        size_t new_capacity =
            wall->framing.member_capacity == 0
                ? 1
                : wall->framing.member_capacity * 2;

        Timber *new_members = realloc(
            wall->framing.members,
            new_capacity *
            sizeof *new_members
        );

        if (new_members == NULL) {
            return 0;
        }

        wall->framing.members = new_members;
        wall->framing.member_capacity =
            new_capacity;
    }

    wall->framing.members[
        wall->framing.member_count
    ] = member;

    wall->framing.member_count++;

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

    WallOpeningFrameGeometry frame;
    if (!wall_opening_frame_geometry(opening, settings, &frame)) { return 0; }
    int64_t left = frame.left_u - settings->stud_width;
    int64_t length = (int64_t)frame.width + 2 * (int64_t)settings->stud_width;
    if (left < 0 || length > INT_MAX || frame.top_z + settings->stud_width > settings->stud_height) { return 0; }

    Timber header = {
        .length = (int)length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .u = (int)left,
            .z = (int)frame.top_z
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

    WallOpeningFrameGeometry frame;
    if (!wall_opening_frame_geometry(opening, settings, &frame) ||
        frame.bottom_z < settings->stud_width) { return 0; }

    Timber sill = {
        .length = frame.width,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .u = (int)frame.left_u,
            .z = (int)frame.bottom_z - settings->stud_width
        },

        .type = TIMBER_SILL
    };

    return wall_add_member(
        wall,
        sill
    );
}