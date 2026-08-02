#include "snap.h"

#include "grid_snap.h"

static double distance_squared(
    Vec2 a,
    Vec2 b
);

static Vec2 timber_end_position(
    const Timber *timber
);

static void consider_endpoint(
    Vec2 cursor,
    Vec2 endpoint,
    double tolerance_squared,
    SnapResult *best_result,
    double *best_distance_squared
);

static void consider_timber_endpoints(
    Vec2 cursor,
    const Timber *timber,
    double tolerance_squared,
    SnapResult *best_result,
    double *best_distance_squared
);

SnapResult editor_snap(
    Vec2 world_position,
    const Wall *wall,
    const SnapSettings *settings
)
{
    SnapResult best_result = {
        .position = world_position,
        .type = SNAP_NONE
    };

    if (settings == NULL) {
        return best_result;
    }

    double best_distance_squared = 0.0;
    int candidate_found = 0;

    /*
     * Grid snapping is always a candidate when enabled.
     */
    if (
        settings->grid_enabled
        && settings->grid_spacing > 0.0
    ) {
        Vec2 grid_position =
            grid_snap_position(
                world_position,
                settings->grid_spacing
            );

        best_result = (SnapResult){
            .position = grid_position,
            .type = SNAP_GRID
        };

        best_distance_squared =
            distance_squared(
                world_position,
                grid_position
            );

        candidate_found = 1;
    }

    /*
     * Object snapping requires a wall and a valid tolerance.
     */
    if (
        settings->endpoint_enabled
        && settings->object_snap_tolerance > 0.0
        && wall != NULL
    ) {
        double tolerance_squared =
            settings->object_snap_tolerance
            * settings->object_snap_tolerance;

        /*
         * If there was no grid candidate, initialise the comparison
         * distance beyond the allowed endpoint tolerance.
         */
        if (!candidate_found) {
            best_distance_squared =
                tolerance_squared + 1.0;
        }

        for (
            size_t i = 0;
            i < wall->stud_count;
            i++
        ) {
            consider_timber_endpoints(
                world_position,
                &wall->studs[i],
                tolerance_squared,
                &best_result,
                &best_distance_squared
            );
        }

        for (
            size_t i = 0;
            i < wall->nog_count;
            i++
        ) {
            consider_timber_endpoints(
                world_position,
                &wall->nogs[i],
                tolerance_squared,
                &best_result,
                &best_distance_squared
            );
        }

        for (
            size_t i = 0;
            i < wall->member_count;
            i++
        ) {
            consider_timber_endpoints(
                world_position,
                &wall->members[i],
                tolerance_squared,
                &best_result,
                &best_distance_squared
            );
        }

        /*
         * Plates are stored directly rather than in an array.
         * A positive length indicates that the plate is initialized.
         */
        if (wall->bottomplate.length > 0) {
            consider_timber_endpoints(
                world_position,
                &wall->bottomplate,
                tolerance_squared,
                &best_result,
                &best_distance_squared
            );
        }

        if (wall->topplate.length > 0) {
            consider_timber_endpoints(
                world_position,
                &wall->topplate,
                tolerance_squared,
                &best_result,
                &best_distance_squared
            );
        }
    }

    return best_result;
}

static double distance_squared(
    Vec2 a,
    Vec2 b
)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;

    return dx * dx + dy * dy;
}

static Vec2 timber_end_position(
    const Timber *timber
)
{
    Vec2 end = {
        .x = (double)timber->position.x,
        .y = (double)timber->position.y
    };

    if (timber->type == TIMBER_STUD) {
        end.y += timber->length;
    }
    else {
        end.x += timber->length;
    }

    return end;
}

static void consider_endpoint(
    Vec2 cursor,
    Vec2 endpoint,
    double tolerance_squared,
    SnapResult *best_result,
    double *best_distance_squared
)
{
    double candidate_distance_squared =
        distance_squared(
            cursor,
            endpoint
        );

    if (
        candidate_distance_squared > tolerance_squared
        || candidate_distance_squared
            > *best_distance_squared
    ) {
        return;
    }

    *best_result = (SnapResult){
        .position = endpoint,
        .type = SNAP_ENDPOINT
    };

    *best_distance_squared =
        candidate_distance_squared;
}

static void consider_timber_endpoints(
    Vec2 cursor,
    const Timber *timber,
    double tolerance_squared,
    SnapResult *best_result,
    double *best_distance_squared
)
{
    if (
        timber == NULL
        || timber->length <= 0
    ) {
        return;
    }

    Vec2 start = {
        .x = (double)timber->position.x,
        .y = (double)timber->position.y
    };

    Vec2 end =
        timber_end_position(timber);

    consider_endpoint(
        cursor,
        start,
        tolerance_squared,
        best_result,
        best_distance_squared
    );

    consider_endpoint(
        cursor,
        end,
        tolerance_squared,
        best_result,
        best_distance_squared
    );
}