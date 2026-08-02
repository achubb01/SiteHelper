#include <assert.h>
#include <stdio.h>

#include "grid_snap.h"

static void assert_vec2_equal(
    Vec2 actual,
    Vec2 expected
)
{
    assert(actual.x == expected.x);
    assert(actual.y == expected.y);
}

static void test_snaps_to_nearest_grid_point(void)
{
    Vec2 result =
        grid_snap_position(
            (Vec2){
                .x = 143.0,
                .y = 267.0
            },
            100.0
        );

    assert_vec2_equal(
        result,
        (Vec2){
            .x = 100.0,
            .y = 300.0
        }
    );
}

static void test_preserves_aligned_position(void)
{
    Vec2 result =
        grid_snap_position(
            (Vec2){
                .x = 300.0,
                .y = 700.0
            },
            100.0
        );

    assert_vec2_equal(
        result,
        (Vec2){
            .x = 300.0,
            .y = 700.0
        }
    );
}

static void test_snaps_negative_coordinates(void)
{
    Vec2 result =
        grid_snap_position(
            (Vec2){
                .x = -143.0,
                .y = -267.0
            },
            100.0
        );

    assert_vec2_equal(
        result,
        (Vec2){
            .x = -100.0,
            .y = -300.0
        }
    );
}

static void test_halfway_values_snap_away_from_zero(void)
{
    Vec2 positive =
        grid_snap_position(
            (Vec2){
                .x = 150.0,
                .y = 250.0
            },
            100.0
        );

    assert_vec2_equal(
        positive,
        (Vec2){
            .x = 200.0,
            .y = 300.0
        }
    );

    Vec2 negative =
        grid_snap_position(
            (Vec2){
                .x = -150.0,
                .y = -250.0
            },
            100.0
        );

    assert_vec2_equal(
        negative,
        (Vec2){
            .x = -200.0,
            .y = -300.0
        }
    );
}

static void test_invalid_spacing_returns_original_position(void)
{
    Vec2 position = {
        .x = 143.0,
        .y = 267.0
    };

    assert_vec2_equal(
        grid_snap_position(
            position,
            0.0
        ),
        position
    );

    assert_vec2_equal(
        grid_snap_position(
            position,
            -100.0
        ),
        position
    );
}

int main(void)
{
    test_snaps_to_nearest_grid_point();
    test_preserves_aligned_position();
    test_snaps_negative_coordinates();
    test_halfway_values_snap_away_from_zero();
    test_invalid_spacing_returns_original_position();

    printf("All grid snap tests passed.\n");

    return 0;
}