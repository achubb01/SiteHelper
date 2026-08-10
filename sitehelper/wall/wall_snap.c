#include "wall_snap.h"

static void collect_timber_array(
    const Timber *timbers,
    size_t timber_count,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *candidate_count
);

static Vec2 timber_end_face_centre(
    const Timber *timber
);

static int point_is_on_timber(
    Vec2 point,
    const Timber *timber
);

static Vec2 timber_start(
    const Timber *timber
);

static Vec2 timber_end(
    const Timber *timber
);

static int candidate_matches(
    const SnapCandidate *candidate,
    SnapType type,
    Vec2 position
);

static int candidate_exists(
    const SnapCandidate *candidates,
    size_t count,
    SnapType type,
    Vec2 position
);

static void append_candidate(
    SnapCandidate candidate,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *count
);

static void append_timber_intersections(
    const Timber *source,
    const Timber *target,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *count
);

static void append_timber_endpoints(
    const Timber *timber,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *count
);

static Vec2 timber_start(
    const Timber *timber
)
{
    return (Vec2){
        .x = (double)timber->position.x,
        .y = (double)timber->position.y
    };
}

static Vec2 timber_end(
    const Timber *timber
)
{
    Vec2 end = timber_start(timber);

    if (timber->type == TIMBER_STUD) {
        end.y += timber->length;
    }
    else {
        end.x += timber->length;
    }

    return end;
}

static void append_candidate(
    SnapCandidate candidate,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *count
)
{
    if (
        candidates == NULL
        || count == NULL
        || *count >= capacity
    ) {
        return;
    }

    if (candidate_exists(
            candidates,
            *count,
            candidate.type,
            candidate.position)) {
        return;
    }

    candidates[*count] = candidate;
    (*count)++;
}

static void append_timber_endpoints(
    const Timber *timber,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *count
)
{
    if (
        timber == NULL
        || timber->length <= 0
    ) {
        return;
    }

    append_candidate(
        (SnapCandidate){
            .position = timber_start(timber),
            .type = SNAP_ENDPOINT
        },
        candidates,
        capacity,
        count
    );

    append_candidate(
        (SnapCandidate){
            .position = timber_end(timber),
            .type = SNAP_ENDPOINT
        },
        candidates,
        capacity,
        count
    );
}

static int timber_is_vertical(
    const Timber *timber
)
{
    return timber->type == TIMBER_STUD;
}

static Vec2 timber_start_face_centre(
    const Timber *timber
)
{
    if (timber_is_vertical(timber)) {
        return (Vec2){
            .x =
                (double)timber->position.x
                + (double)timber->width / 2.0,

            .y = (double)timber->position.y
        };
    }

    return (Vec2){
        .x = (double)timber->position.x,

        .y =
            (double)timber->position.y
            + (double)timber->width / 2.0
    };
}

static Vec2 timber_end_face_centre(
    const Timber *timber
)
{
    Vec2 end =
        timber_start_face_centre(timber);

    if (timber_is_vertical(timber)) {
        end.y += timber->length;
    }
    else {
        end.x += timber->length;
    }

    return end;
}

static void collect_timber_array(
    const Timber *timbers,
    size_t timber_count,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *candidate_count
)
{
    if (
        timbers == NULL
        || candidates == NULL
        || candidate_count == NULL
    ) {
        return;
    }

    for (
        size_t i = 0;
        i < timber_count;
        i++
    ) {
        append_timber_endpoints(
            &timbers[i],
            candidates,
            capacity,
            candidate_count
        );

        if (*candidate_count >= capacity) {
            return;
        }
    }
}

static int point_is_on_timber(
    Vec2 point,
    const Timber *timber
)
{
    if (
        timber == NULL
        || timber->length <= 0
        || timber->width <= 0
    ) {
        return 0;
    }

    double left =
        (double)timber->position.x;

    double bottom =
        (double)timber->position.y;

    double right;
    double top;

    if (timber_is_vertical(timber)) {
        right = left + timber->width;
        top = bottom + timber->length;
    }
    else {
        right = left + timber->length;
        top = bottom + timber->width;
    }

    return
        point.x >= left
        && point.x <= right
        && point.y >= bottom
        && point.y <= top;
}

static void append_timber_intersections(
    const Timber *source,
    const Timber *target,
    SnapCandidate *candidates,
    size_t capacity,
    size_t *count
)
{
    if (
        source == NULL
        || target == NULL
        || source == target
    ) {
        return;
    }

    Vec2 start =
        timber_start_face_centre(source);

    Vec2 end =
        timber_end_face_centre(source);

    if (point_is_on_timber(start, target)) {
        append_candidate(
            (SnapCandidate){
                .position = start,
                .type = SNAP_INTERSECTION
            },
            candidates,
            capacity,
            count
        );
    }

    if (point_is_on_timber(end, target)) {
        append_candidate(
            (SnapCandidate){
                .position = end,
                .type = SNAP_INTERSECTION
            },
            candidates,
            capacity,
            count
        );
    }
}

size_t wall_collect_snap_candidates(
    const Wall *wall,
    SnapCandidate *candidates,
    size_t capacity
)
{
    if (
        wall == NULL
        || candidates == NULL
        || capacity == 0
    ) {
        return 0;
    }

    size_t count = 0;

    collect_timber_array(
        wall->framing.studs,
        wall->framing.stud_count,
        candidates,
        capacity,
        &count
    );

    collect_timber_array(
        wall->framing.nogs,
        wall->framing.nog_count,
        candidates,
        capacity,
        &count
    );

    collect_timber_array(
        wall->framing.members,
        wall->framing.member_count,
        candidates,
        capacity,
        &count
    );

    append_timber_endpoints(
        &wall->framing.bottomplate,
        candidates,
        capacity,
        &count
    );

    append_timber_endpoints(
        &wall->framing.topplate,
        candidates,
        capacity,
        &count
    );

    for (
        size_t i = 0;
        i < wall->framing.stud_count;
        i++
    ) {
        append_timber_intersections(
            &wall->framing.studs[i],
            &wall->framing.bottomplate,
            candidates,
            capacity,
            &count
        );

        append_timber_intersections(
            &wall->framing.studs[i],
            &wall->framing.topplate,
            candidates,
            capacity,
            &count
        );
    }

    for (
        size_t noggin_index = 0;
        noggin_index < wall->framing.nog_count;
        noggin_index++
    ) {
        for (
            size_t stud_index = 0;
            stud_index < wall->framing.stud_count;
            stud_index++
        ) {
            append_timber_intersections(
                &wall->framing.nogs[noggin_index],
                &wall->framing.studs[stud_index],
                candidates,
                capacity,
                &count
            );
        }
    }

    return count;
}

static int candidate_matches(
    const SnapCandidate *candidate,
    SnapType type,
    Vec2 position
)
{
    return
        candidate->type == type
        && candidate->position.x == position.x
        && candidate->position.y == position.y;
}

static int candidate_exists(
    const SnapCandidate *candidates,
    size_t count,
    SnapType type,
    Vec2 position
)
{
    if (candidates == NULL) {
        return 0;
    }

    for (
        size_t i = 0;
        i < count;
        i++
    ) {
        if (candidate_matches(
                &candidates[i],
                type,
                position)) {
            return 1;
        }
    }

    return 0;
}

