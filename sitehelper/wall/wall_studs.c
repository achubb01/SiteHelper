#include <stdlib.h>

#include "wall.h"
#include "wall_internal.h"

int wall_add_stud(
    Wall *wall,
    const BuildSettings *settings,
    int position,
    StudType type
)
{
    return wall_add_custom_stud(
        wall,
        settings,
        position,
        0,
        settings->stud_height,
        type
    );
}

int wall_add_custom_stud(
    Wall *wall,
    const BuildSettings *settings,
    int x, 
    int y,
    int length,
    StudType type
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (x < 0 || y < 0 || length <= 0) {
        return 0;
    }

    if (wall->stud_count ==
        wall->stud_capacity) {

        size_t new_capacity =
            wall->stud_capacity == 0
                ? 1
                : wall->stud_capacity * 2;

        Timber *new_studs = realloc(
            wall->studs,
            new_capacity * sizeof *new_studs
        );

        if (new_studs == NULL) {
            return 0;
        }

        wall->studs = new_studs;
        wall->stud_capacity =
            new_capacity;
    }

    Timber stud = {
        .length = length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = x,
            .y = y
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = type
        }
    };

    wall->studs[
        wall->stud_count
    ] = stud;

    wall->stud_count++;

    return 1;
}

void wall_clear_studs(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->stud_count = 0;
}

void wall_remove_stud(
    Wall *wall,
    size_t index
)
{
    if (wall == NULL ||
        index >= wall->stud_count) {
        return;
    }

    for (size_t i = index;
         i + 1 < wall->stud_count;
         i++) {

        wall->studs[i] =
            wall->studs[i + 1];
    }

    wall->stud_count--;
}

int wall_compare_stud_position(
    const void *a,
    const void *b
)
{
    const Timber *stud_a = a;
    const Timber *stud_b = b;

    return
        stud_a->position.x -
        stud_b->position.x;
}
