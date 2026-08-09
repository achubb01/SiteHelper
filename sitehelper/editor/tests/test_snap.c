#include <assert.h>
#include <stdio.h>

#include "snap.h"

static void assert_vec2_equal(
    Vec2 actual,
    Vec2 expected
)
{
    assert(actual.x == expected.x);
    assert(actual.y == expected.y);
}

static void test_snap_to_grid_returns_grid_result(void)
{
    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = 100.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 143.0,
                .y = 267.0
            },
            NULL,
            0,
            &settings
        );

    assert(result.type == SNAP_GRID);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 100.0,
            .y = 300.0
        }
    );
}

static void test_invalid_spacing_returns_no_snap(void)
{
    Vec2 position = {
        .x = 143.0,
        .y = 267.0
    };

    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = 0.0
    };

    SnapResult result =
        editor_snap(
            position,
            NULL,
            0,
            &settings
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(
        result.position,
        position
    );
}

static void test_negative_spacing_returns_no_snap(void)
{
    Vec2 position = {
        .x = -143.0,
        .y = -267.0
    };

    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = -100.0
    };

    SnapResult result =
        editor_snap(
            position,
            NULL,
            0,
            &settings
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(
        result.position,
        position
    );
}

static void test_aligned_position_still_reports_grid_snap(void)
{
    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = 100.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 300.0,
                .y = 700.0
            },
            NULL,
            0,
            &settings
        );

    assert(result.type == SNAP_GRID);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 300.0,
            .y = 700.0
        }
    );
}

static void test_disabled_grid_returns_no_snap(void)
{
    Vec2 position = {
        .x = 143.0,
        .y = 267.0
    };

    SnapSettings settings = {
        .grid_enabled = 0,
        .grid_spacing = 100.0
    };

    SnapResult result =
        editor_snap(
            position,
            NULL,
            0,
            &settings
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(
        result.position,
        position
    );
}

static void test_null_settings_returns_no_snap(void)
{
    Vec2 position = {
        .x = 143.0,
        .y = 267.0
    };

    SnapResult result =
        editor_snap(
            position,
            NULL,
            0,
            NULL
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(
        result.position,
        position
    );
}

static void test_snap_to_nearby_endpoint(void)
{
    SnapCandidate candidates[] = {
        {
            .position = {
                .x = 600.0,
                .y = 2400.0
            },
            .type = SNAP_ENDPOINT
        }
    };

    SnapSettings settings = {
        .grid_enabled = 0,

        .endpoint_enabled = 1,
        .object_snap_tolerance = 30.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 615.0,
                .y = 2385.0
            },
            candidates,
            1,
            &settings
        );

    assert(result.type == SNAP_ENDPOINT);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 600.0,
            .y = 2400.0
        }
    );
}

static void test_endpoint_outside_tolerance_falls_back_to_grid(void)
{
    SnapCandidate candidates[] = {
        {
            .position = {
                .x = 600.0,
                .y = 2400.0
            },
            .type = SNAP_ENDPOINT
        }
    };

    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = 100.0,

        .endpoint_enabled = 1,
        .object_snap_tolerance = 20.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 640.0,
                .y = 2360.0
            },
            candidates,
            1,
            &settings
        );

    assert(result.type == SNAP_GRID);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 600.0,
            .y = 2400.0
        }
    );
}

static void test_selects_nearest_endpoint_candidate(void)
{
    SnapCandidate candidates[] = {
        {
            .position = {600.0, 2400.0},
            .type = SNAP_ENDPOINT
        },
        {
            .position = {700.0, 2400.0},
            .type = SNAP_ENDPOINT
        }
    };

    SnapSettings settings = {
        .endpoint_enabled = 1,
        .object_snap_tolerance = 100.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 680.0,
                .y = 2390.0
            },
            candidates,
            2,
            &settings
        );

    assert(result.type == SNAP_ENDPOINT);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 700.0,
            .y = 2400.0
        }
    );
}

static void test_disabled_endpoint_candidate_uses_grid(void)
{
    SnapCandidate candidates[] = {
        {
            .position = {
                .x = 650.0,
                .y = 2400.0
            },
            .type = SNAP_ENDPOINT
        }
    };

    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = 100.0,

        .endpoint_enabled = 0,
        .object_snap_tolerance = 50.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 655.0,
                .y = 2395.0
            },
            candidates,
            1,
            &settings
        );

    assert(result.type == SNAP_GRID);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 700.0,
            .y = 2400.0
        }
    );
}

static void test_endpoint_within_tolerance_beats_closer_grid_point(void)
{
    SnapCandidate candidates[] = {
        {
            .position = {
                .x = 635.0,
                .y = 2400.0
            },
            .type = SNAP_ENDPOINT
        }
    };

    SnapSettings settings = {
        .grid_enabled = 1,
        .grid_spacing = 100.0,

        .endpoint_enabled = 1,
        .object_snap_tolerance = 50.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 605.0,
                .y = 2400.0
            },
            candidates,
            1,
            &settings
        );

    assert(result.type == SNAP_ENDPOINT);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 635.0,
            .y = 2400.0
        }
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
    test_snap_to_grid_returns_grid_result();
    test_invalid_spacing_returns_no_snap();
    test_negative_spacing_returns_no_snap();
    test_aligned_position_still_reports_grid_snap();
    test_disabled_grid_returns_no_snap();
    test_null_settings_returns_no_snap();
    test_snap_to_nearby_endpoint();
    test_endpoint_outside_tolerance_falls_back_to_grid();
    test_selects_nearest_endpoint_candidate();
    test_disabled_endpoint_candidate_uses_grid();
    test_endpoint_within_tolerance_beats_closer_grid_point();
    test_intersection_beats_endpoint_at_same_position();


    printf(
        "All editor snap tests passed.\n"
    );

    return 0;
}