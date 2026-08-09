#include "snap.h"

#include "grid_snap.h"

static double distance_squared(
    Vec2 a,
    Vec2 b
);

static int snap_type_enabled(
    SnapType type,
    const SnapSettings *settings
);

static int snap_priority(
    SnapType type
);

SnapResult editor_snap(
    Vec2 world_position,
    const SnapCandidate *candidates,
    size_t candidate_count,
    const SnapSettings *settings
)
{
    SnapResult fallback = {
        .position = world_position,
        .type = SNAP_NONE
    };

    if (settings == NULL) {
        return fallback;
    }

    if (
        settings->grid_enabled
        && settings->grid_spacing > 0.0
    ) {
        fallback = (SnapResult){
            .position = grid_snap_position(
                world_position,
                settings->grid_spacing
            ),
            .type = SNAP_GRID
        };
    }

    if (
        candidates == NULL
        || candidate_count == 0
        || settings->object_snap_tolerance <= 0.0
    ) {
        return fallback;
    }

    double tolerance_squared =
        settings->object_snap_tolerance
        * settings->object_snap_tolerance;

    double best_distance_squared =
        tolerance_squared;

    SnapResult object_result = {
        .position = world_position,
        .type = SNAP_NONE
    };

    for (
        size_t i = 0;
        i < candidate_count;
        i++
    ) {
        const SnapCandidate *candidate =
            &candidates[i];

        if (!snap_type_enabled(
                candidate->type,
                settings)) {
            continue;
        }

        double candidate_distance_squared =
            distance_squared(
                world_position,
                candidate->position
            );

        if (
            candidate_distance_squared
                > best_distance_squared
        ) {
            continue;
        }

        if (
            candidate_distance_squared
                == best_distance_squared
            && snap_priority(candidate->type)
                <= snap_priority(object_result.type)
        ) {
            continue;
        }

        object_result = (SnapResult){
            .position = candidate->position,
            .type = candidate->type
        };

        best_distance_squared =
            candidate_distance_squared;
    }

    if (object_result.type != SNAP_NONE) {
        return object_result;
    }

    return fallback;
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

static int snap_type_enabled(
    SnapType type,
    const SnapSettings *settings
)
{
    if (settings == NULL) {
        return 0;
    }

    switch (type) {
        case SNAP_ENDPOINT:
            return settings->endpoint_enabled;

        case SNAP_INTERSECTION:
            return settings->intersection_enabled;

        default:
            return 0;
    }
}

static int snap_priority(
    SnapType type
)
{
    switch (type) {
        case SNAP_INTERSECTION:
            return 3;

        case SNAP_ENDPOINT:
            return 2;

        case SNAP_GRID:
            return 1;

        default:
            return 0;
    }
}