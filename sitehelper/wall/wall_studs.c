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
    int u,
    int z,
    int length,
    StudType type
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (u < 0 || z < 0 || length <= 0) {
        return 0;
    }

    if (wall->framing.stud_count ==
        wall->framing.stud_capacity) {

        size_t new_capacity =
            wall->framing.stud_capacity == 0
                ? 1
                : wall->framing.stud_capacity * 2;

        Timber *new_studs = realloc(
            wall->framing.studs,
            new_capacity * sizeof *new_studs
        );

        if (new_studs == NULL) {
            return 0;
        }

        wall->framing.studs = new_studs;
        wall->framing.stud_capacity =
            new_capacity;
    }

    Timber stud = {
        .length = length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .u = u,
            .z = z
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = type
        }
    };

    wall->framing.studs[
        wall->framing.stud_count
    ] = stud;

    wall->framing.stud_count++;

    return 1;
}

void wall_clear_studs(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->framing.stud_count = 0;
}

void wall_remove_stud(
    Wall *wall,
    size_t index
)
{
    if (wall == NULL ||
        index >= wall->framing.stud_count) {
        return;
    }

    for (size_t i = index;
         i + 1 < wall->framing.stud_count;
         i++) {

        wall->framing.studs[i] =
            wall->framing.studs[i + 1];
    }

    wall->framing.stud_count--;
}

int wall_compare_stud_position(
    const void *a,
    const void *b
)
{
    const Timber *stud_a = a;
    const Timber *stud_b = b;

    return
        stud_a->position.u -
        stud_b->position.u;
}


int wall_add_stud_at_position(
    int position,
    void *context
)
{
    StudGenerationContext *generation =
        context;

    return wall_add_custom_stud(
        generation->wall,
        generation->settings,
        position,
        generation->z,
        generation->length,
        generation->type
    );
}
