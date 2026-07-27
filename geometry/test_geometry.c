#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "./geometry.h"

static int nearly_equal(double a, double b)
{
    const double epsilon = 0.000001;

    return fabs(a - b) < epsilon;
}

static void test_world_to_screen_with_translation_and_scale(void)
{
    Camera2D camera = {
        .position = {200.0, 100.0},
        .scale = 0.5,
    };

    Vec2 world = {1000.0, 700.0};

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 result = camera_world_to_screen(
        &camera,
        viewport,
        world
    );

    assert(nearly_equal(result.x, 400.0));
    assert(nearly_equal(result.y, 300.0));
}

static void test_world_to_screen_with_origin_camera(void)
{
    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 2.0,
    };

    Vec2 world = {150.0, 75.0};

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 result = camera_world_to_screen(
        &camera,
        viewport,
        world
    );

    assert(nearly_equal(result.x, 300.0));
    assert(nearly_equal(result.y, 450.0));
}

static void test_world_to_screen_can_produce_negative_coordinates(void)
{
    Camera2D camera = {
        .position = {500.0, 500.0},
        .scale = 1.0,
    };

    Vec2 world = {200.0, 1300.0};

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 result = camera_world_to_screen(
        &camera,
        viewport,
        world
    );

    assert(nearly_equal(result.x, -300.0));
    assert(nearly_equal(result.y, -200.0));
}

static void test_world_screen_round_trip(void)
{
    Camera2D camera = {
        .position = {200.0, 100.0},
        .scale = 0.5,
    };

    Vec2 original = {1000.0, 700.0};

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 screen = camera_world_to_screen(
        &camera,
        viewport,
        original
    );

    Vec2 result = camera_screen_to_world(
        &camera,
        viewport,
        screen
    );

    assert(nearly_equal(result.x, original.x));
    assert(nearly_equal(result.y, original.y)); 
}

static void test_world_screen_round_trip_fractional_scale(void)
{
    Camera2D camera = {
        .position = {123.4, 567.8},
        .scale = 0.173,
    };

    Vec2 original = {
        1837.25,
        2416.75
    };

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 screen = camera_world_to_screen(
        &camera,
        viewport,
        original
    );

    Vec2 result = camera_screen_to_world(
        &camera,
        viewport,
        screen
    );

    assert(nearly_equal(result.x, original.x));
    assert(nearly_equal(result.y, original.y));
}

static void test_rect2_top_right(void)
{
    Rect2 rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Vec2 result = rect2_top_right(rect);

    assert(nearly_equal(result.x, 690.0));
    assert(nearly_equal(result.y, 2400.0));
}

static void test_rect2_contains_point_inside(void)
{
    Rect2 rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Vec2 point = {650.0, 1200.0};

    assert(rect2_contains_point(rect, point));
}

static void test_rect2_contains_point_outside(void)
{
    Rect2 rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Vec2 point = {800.0, 1200.0};

    assert(!rect2_contains_point(rect, point));
}

static void test_rect2_contains_point_on_edge(void)
{
    Rect2 rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Vec2 point = {690.0, 1200.0};

    assert(rect2_contains_point(rect, point));
}

static void test_rect2_world_to_screen(void)
{
    Camera2D camera = {
        .position = {100.0, -100.0},
        .scale = 0.25,
    };

    Rect2 world_rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Rect2 result = rect2_world_to_screen(&camera, viewport, world_rect);

    assert(nearly_equal(result.position.x, 125.0));
    assert(nearly_equal(result.position.y, -25.0));
    assert(nearly_equal(result.width, 22.5));
    assert(nearly_equal(result.height, 600.0));
}

static void test_world_to_screen_flips_y_axis(void)
{
    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 1.0,
    };

    Vec2 world = {100.0, 100.0};

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 result = camera_world_to_screen(&camera, viewport, world);

    assert(nearly_equal(result.x, 100.0));
    assert(nearly_equal(result.y, 500.0));
}

static void test_world_origin_maps_to_bottom_left(void)
{
    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 1.0,
    };

    Vec2 world = {0.0, 0.0};

    Viewport2D viewport = {
        .width = 800.0,
        .height = 600.0
    };

    Vec2 result = camera_world_to_screen(&camera, viewport, world);

    assert(nearly_equal(result.x, 0.0));
    assert(nearly_equal(result.y, 600.0));
}

int main(void)
{
    test_world_to_screen_with_translation_and_scale();
    test_world_to_screen_with_origin_camera();
    test_world_to_screen_can_produce_negative_coordinates();
    test_world_screen_round_trip();
    test_world_screen_round_trip_fractional_scale();
    test_rect2_top_right();
    test_rect2_contains_point_inside();
    test_rect2_contains_point_outside();
    test_rect2_contains_point_on_edge();
    test_rect2_world_to_screen();
    test_world_to_screen_flips_y_axis();
    test_world_origin_maps_to_bottom_left();

    printf("geometry tests passed\n");

    return 0;
}