#include <stdlib.h>

#include "./wall.h"
#include "wall_internal.h"

typedef int (*PositionCallback)(
    int position,
    void *context
);

static int wall_generate_studs(Wall *wall, const BuildSettings *settings, int wall_length);


static int wall_generate_plates(Wall *wall, const BuildSettings *settings, int wall_length);



static int wall_generate_plates(
    Wall *wall,
    const BuildSettings *settings,
    int wall_length
)
{
    if (wall == NULL ||
        settings == NULL) {
        return 0;
    }

    if (wall_length <= 0 ||
        settings->stud_width <= 0 ||
        settings->stud_depth <= 0 ||
        settings->stud_height <= 0) {
        return 0;
    }

    wall->framing.bottomplate = (Timber){
        .length = wall_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .u = 0,
            .z = 0
        },

        .type = TIMBER_PLATE,

        .details.plate = {
            .placeholder = 0
        }
    };

    wall->framing.topplate = (Timber){
        .length = wall_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .u = 0,
            .z = settings->stud_height
        },

        .type = TIMBER_PLATE,

        .details.plate = {
            .placeholder = 0
        }
    };

    return 1;
}

static int wall_build_framing(
    Wall *wall,
    const BuildSettings *settings,
    int wall_length
)
{
    if (!wall_generate_plates(
            wall,
            settings,
            wall_length)) {
        return 0;
    }

    if (!wall_generate_studs(
            wall,
            settings,
            wall_length)) {
        return 0;
    }

    if (!wall_apply_openings(
            wall,
            settings,
            wall_length)) {
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

int wall_generate(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    int wall_length = wall_length_mm(wall);
    if (wall_length == 0) {
        return 0;
    }

    if (!wall_opening_definitions_valid(wall, settings)) { return 0; }

    /*
     * Build replacement framing separately
     * from the currently committed framing.
     *
     * The definition is borrowed from the
     * live wall. The candidate does not own
     * the openings array.
     * Local generation receives only the derived scalar length; it does not
     * inspect the borrowed physical segment.
     */
    Wall candidate = {
        .id = wall->id,
        .definition = wall->definition,
        .framing = {0}
    };

    if (!wall_build_framing(
            &candidate,
            settings,
            wall_length)) {

        wall_framing_destroy(
            &candidate.framing
        );

        return 0;
    }

    /*
     * Generation completed successfully.
     *
     * Only now do we discard the previously
     * committed framing.
     */
    wall_framing_destroy(
        &wall->framing
    );

    wall->framing =
        candidate.framing;

    return 1;
}

static int wall_generate_studs(
    Wall *wall,
    const BuildSettings *settings,
    int wall_length
)
{
    if (wall == NULL ||
        settings == NULL) {
        return 0;
    }

    int end =
        wall_length -
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













