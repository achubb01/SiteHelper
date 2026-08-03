#include <assert.h>
#include <stdio.h>

#include "wall_snap.h"

static void assert_vec2_equal(
    Vec2 actual,
    Vec2 expected
)
{
    assert(actual.x == expected.x);
    assert(actual.y == expected.y);
}

static void test_collects_vertical_stud_endpoints(void)
{
    Timber stud = {
        .length = 2400,
        .position = {
            .x = 600,
            .y = 0
        },
        .type = TIMBER_STUD
    };

    Wall wall = {
        .studs = &stud,
        .stud_count = 1
    };

    SnapCandidate candidates[2];

    size_t count =
        wall_collect_snap_candidates(
            &wall,
            candidates,
            2
        );

    assert(count == 2);

    assert(candidates[0].type == SNAP_ENDPOINT);
    assert_vec2_equal(
        candidates[0].position,
        (Vec2){600.0, 0.0}
    );

    assert(candidates[1].type == SNAP_ENDPOINT);
    assert_vec2_equal(
        candidates[1].position,
        (Vec2){600.0, 2400.0}
    );
}

static void test_collects_horizontal_noggin_endpoints(void)
{
    Timber noggin = {
        .length = 565,
        .position = {
            .x = 635,
            .y = 1200
        },
        .type = TIMBER_NOGGIN
    };

    Wall wall = {
        .nogs = &noggin,
        .nog_count = 1
    };

    SnapCandidate candidates[2];

    size_t count =
        wall_collect_snap_candidates(
            &wall,
            candidates,
            2
        );

    assert(count == 2);

    assert_vec2_equal(
        candidates[0].position,
        (Vec2){635.0, 1200.0}
    );

    assert_vec2_equal(
        candidates[1].position,
        (Vec2){1200.0, 1200.0}
    );
}

static void test_respects_candidate_capacity(void)
{
    Timber studs[2] = {
        {
            .length = 2400,
            .position = {.x = 0, .y = 0},
            .type = TIMBER_STUD
        },
        {
            .length = 2400,
            .position = {.x = 600, .y = 0},
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 2
    };

    SnapCandidate candidates[3];

    size_t count =
        wall_collect_snap_candidates(
            &wall,
            candidates,
            3
        );

    assert(count == 3);
}

static void test_null_inputs_return_zero(void)
{
    SnapCandidate candidates[2];

    assert(
        wall_collect_snap_candidates(
            NULL,
            candidates,
            2
        ) == 0
    );

    Wall wall = {0};

    assert(
        wall_collect_snap_candidates(
            &wall,
            NULL,
            2
        ) == 0
    );

    assert(
        wall_collect_snap_candidates(
            &wall,
            candidates,
            0
        ) == 0
    );
}

static int contains_candidate(
    const SnapCandidate *candidates,
    size_t count,
    SnapType type,
    Vec2 position
)
{
    for (size_t i = 0; i < count; i++) {
        if (
            candidates[i].type == type
            && candidates[i].position.x == position.x
            && candidates[i].position.y == position.y
        ) {
            return 1;
        }
    }

    return 0;
}

static void test_collects_stud_to_top_plate_intersection(void)
{
    Timber stud = {
        .length = 2400,
        .width = 35,

        .position = {
            .x = 600,
            .y = 0
        },

        .type = TIMBER_STUD
    };

    Timber topplate = {
        .length = 4200,
        .width = 35,

        .position = {
            .x = 0,
            .y = 2400
        },

        .type = TIMBER_PLATE
    };

    Wall wall = {
        .studs = &stud,
        .stud_count = 1,
        .topplate = topplate
    };

    SnapCandidate candidates[16];

    size_t count =
        wall_collect_snap_candidates(
            &wall,
            candidates,
            16
        );

    /*
     * The stud occupies x = 600..635.
     * Its top end-face centre is therefore x = 617.5.
     */
    assert(
        contains_candidate(
            candidates,
            count,
            SNAP_INTERSECTION,
            (Vec2){
                .x = 617.5,
                .y = 2400.0
            }
        )
    );
}

static size_t count_matching_candidates(
    const SnapCandidate *candidates,
    size_t count,
    SnapType type,
    Vec2 position
)
{
    size_t matches = 0;

    for (
        size_t i = 0;
        i < count;
        i++
    ) {
        if (
            candidates[i].type == type
            && candidates[i].position.x == position.x
            && candidates[i].position.y == position.y
        ) {
            matches++;
        }
    }

    return matches;
}

static void test_does_not_duplicate_intersection_candidates(void)
{
    Timber stud = {
        .length = 2400,
        .width = 35,
        .position = {
            .x = 600,
            .y = 0
        },
        .type = TIMBER_STUD
    };

    Timber topplate = {
        .length = 4200,
        .width = 35,
        .position = {
            .x = 0,
            .y = 2400
        },
        .type = TIMBER_PLATE
    };

    Wall wall = {
        .studs = &stud,
        .stud_count = 1,
        .topplate = topplate
    };

    SnapCandidate candidates[32];

    size_t count =
        wall_collect_snap_candidates(
            &wall,
            candidates,
            32
        );

    assert(
        count_matching_candidates(
            candidates,
            count,
            SNAP_INTERSECTION,
            (Vec2){
                .x = 617.5,
                .y = 2400.0
            }
        ) == 1
    );
}

static void test_collects_noggin_to_stud_intersection(void)
{
    Timber stud = {
        .length = 2400,
        .width = 35,
        .position = {
            .x = 600,
            .y = 0
        },
        .type = TIMBER_STUD
    };

    Timber noggin = {
        .length = 565,
        .width = 35,
        .position = {
            .x = 35,
            .y = 1200
        },
        .type = TIMBER_NOGGIN
    };

    Wall wall = {
        .studs = &stud,
        .stud_count = 1,
        .nogs = &noggin,
        .nog_count = 1
    };

    SnapCandidate candidates[16];

    size_t count =
        wall_collect_snap_candidates(
            &wall,
            candidates,
            16
        );

    assert(
        contains_candidate(
            candidates,
            count,
            SNAP_INTERSECTION,
            (Vec2){
                .x = 600.0,
                .y = 1217.5
            }
        )
    );
}

static void test_intersection_beats_endpoint_at_same_position(void)
{
    SnapCandidate candidates[] = {
        {
            .position = {600.0, 2400.0},
            .type = SNAP_ENDPOINT
        },
        {
            .position = {600.0, 2400.0},
            .type = SNAP_INTERSECTION
        }
    };

    SnapSettings settings = {
        .endpoint_enabled = 1,
        .intersection_enabled = 1,
        .object_snap_tolerance = 30.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){610.0, 2390.0},
            candidates,
            2,
            &settings
        );

    assert(result.type == SNAP_INTERSECTION);
}

int main(void)
{
    test_collects_vertical_stud_endpoints();
    test_collects_horizontal_noggin_endpoints();
    test_respects_candidate_capacity();
    test_null_inputs_return_zero();
    test_collects_stud_to_top_plate_intersection();
    test_does_not_duplicate_intersection_candidates();
    test_collects_noggin_to_stud_intersection();
    test_intersection_beats_endpoint_at_same_position();

    printf("All wall snap tests passed.\n");

    return 0;
}