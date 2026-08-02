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
            NULL
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(
        result.position,
        position
    );
}

static void test_snap_to_nearby_timber_endpoint(void)
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
            &wall,
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
            &wall,
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

static void test_snap_to_horizontal_timber_endpoint(void)
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

    SnapSettings settings = {
        .grid_enabled = 0,

        .endpoint_enabled = 1,
        .object_snap_tolerance = 30.0
    };

    SnapResult result =
        editor_snap(
            (Vec2){
                .x = 1185.0,
                .y = 1210.0
            },
            &wall,
            &settings
        );

    assert(result.type == SNAP_ENDPOINT);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 1200.0,
            .y = 1200.0
        }
    );
}

static void test_disabled_endpoint_snap_uses_grid(void)
{
    Timber stud = {
        .length = 2400,
        .position = {
            .x = 650,
            .y = 0
        },
        .type = TIMBER_STUD
    };

    Wall wall = {
        .studs = &stud,
        .stud_count = 1
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
            &wall,
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
    Timber stud = {
        .length = 2400,
        .position = {
            .x = 635,
            .y = 0
        },
        .type = TIMBER_STUD
    };

    Wall wall = {
        .studs = &stud,
        .stud_count = 1
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
            &wall,
            &settings
        );

    /*
     * Grid point (600, 2400) is only 5 units away.
     * Endpoint (635, 2400) is 30 units away.
     *
     * Endpoint still wins because it is within
     * the object-snap tolerance.
     */
    assert(result.type == SNAP_ENDPOINT);

    assert_vec2_equal(
        result.position,
        (Vec2){
            .x = 635.0,
            .y = 2400.0
        }
    );
}

int main(void)
{
    test_snap_to_grid_returns_grid_result();
    test_invalid_spacing_returns_no_snap();
    test_negative_spacing_returns_no_snap();
    test_aligned_position_still_reports_grid_snap();
    test_disabled_grid_returns_no_snap();
    test_null_settings_returns_no_snap();
    test_snap_to_nearby_timber_endpoint();
    test_endpoint_outside_tolerance_falls_back_to_grid();
    test_snap_to_horizontal_timber_endpoint();
    test_disabled_endpoint_snap_uses_grid();
    test_endpoint_within_tolerance_beats_closer_grid_point();

    printf(
        "All editor snap tests passed.\n"
    );

    return 0;
}