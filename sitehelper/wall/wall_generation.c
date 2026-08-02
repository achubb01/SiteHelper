#include <stdlib.h>

#include "./wall.h"
#include "wall_internal.h"

typedef int (*PositionCallback)(
    int position,
    void *context
);

static int wall_generate_studs(Wall *wall, const BuildSettings *settings);


static int wall_generate_plates(Wall *wall, const BuildSettings *settings);



int wall_set_length(Wall *wall, int length)
{
    if (wall == NULL) {
        return 0;
    }

    if (length <= 0) {
        return 0;
    }

    wall->bottomplate.length = length;

    return 1;
}

static int wall_generate_plates(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL ||
        settings == NULL) {
        return 0;
    }

    int wall_length =
        wall->bottomplate.length;

    if (wall_length <= 0 ||
        settings->stud_width <= 0 ||
        settings->stud_depth <= 0 ||
        settings->stud_height <= 0) {
        return 0;
    }

    wall->bottomplate = (Timber){
        .length = wall_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = 0,
            .y = 0
        },

        .type = TIMBER_PLATE,

        .details.plate = {
            .placeholder = 0
        }
    };

    wall->topplate = (Timber){
        .length = wall_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = 0,
            .y = settings->stud_height
        },

        .type = TIMBER_PLATE,

        .details.plate = {
            .placeholder = 0
        }
    };

    return 1;
}

int wall_generate(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (wall->bottomplate.length <= 0) {
        return 0;
    }

    if (settings->stud_height <= 0 ||
        settings->stud_width <= 0 ||
        settings->stud_depth <= 0 ||
        settings->stud_spacing <= 0 ||
        settings->nog_spacing <= 0) {
        return 0;
    }

    wall_clear_studs(wall);
    wall_clear_noggins(wall);
    wall_clear_members(wall);

    if (!wall_generate_plates(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_generate_studs(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_apply_openings(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_repair_stud_spacing(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_generate_noggins(
            wall,
            settings)) {
        return 0;
    }

    return 1;
}

static int wall_generate_studs(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL ||
        settings == NULL) {
        return 0;
    }

    int end =
        wall->bottomplate.length -
        settings->stud_width;

    if (end < 0) {
        return 0;
    }

    StudGenerationContext context = {
        .wall = wall,
        .settings = settings,
        .length = settings->stud_height,
        .type = STUD_COMMON
    };

    return wall_generate_positions(
        0,
        end,
        settings,
        wall_add_stud_at_position,
        &context
    );
}
















