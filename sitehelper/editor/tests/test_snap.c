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
    SnapResult result =
        editor_snap_to_grid(
            (Vec2){
                .x = 143.0,
                .y = 267.0
            },
            100.0
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

    SnapResult result =
        editor_snap_to_grid(
            position,
            0.0
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(result.position, position);
}

static void test_negative_spacing_returns_no_snap(void)
{
    Vec2 position = {
        .x = -143.0,
        .y = -267.0
    };

    SnapResult result =
        editor_snap_to_grid(
            position,
            -100.0
        );

    assert(result.type == SNAP_NONE);
    assert_vec2_equal(result.position, position);
}

static void test_aligned_position_still_reports_grid_snap(void)
{
    SnapResult result =
        editor_snap_to_grid(
            (Vec2){
                .x = 300.0,
                .y = 700.0
            },
            100.0
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

int main(void)
{
    test_snap_to_grid_returns_grid_result();
    test_invalid_spacing_returns_no_snap();
    test_negative_spacing_returns_no_snap();
    test_aligned_position_still_reports_grid_snap();

    printf("All editor snap tests passed.\n");

    return 0;
}