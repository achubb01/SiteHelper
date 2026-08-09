#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "renderer2d.h"

typedef struct FakeBackendState {
    int draw_line_called;
    int draw_rect_called;
    int fill_rect_called;

    int set_clip_rect_called;
    int clear_clip_rect_called;

    Vec2 last_line_start;
    Vec2 last_line_end;

    Rect2 last_rect;
    Rect2 last_clip_rect;

    Colour last_colour;
} FakeBackendState;

static void fake_draw_rect(
    void *context,
    Rect2 rect,
    Colour colour
)
{
    FakeBackendState *state = context;

    state->draw_rect_called = 1;
    state->last_rect = rect;
    state->last_colour = colour;
}

static void fake_fill_rect(
    void *context,
    Rect2 rect,
    Colour colour
)
{
    FakeBackendState *state = context;

    state->fill_rect_called = 1;
    state->last_rect = rect;
    state->last_colour = colour;
}

static void fake_draw_line(
    void *context,
    Vec2 start,
    Vec2 end,
    Colour colour
)
{
    FakeBackendState *state = context;

    state->draw_line_called = 1;
    state->last_line_start = start;
    state->last_line_end = end;
    state->last_colour = colour;
}

static void fake_set_clip_rect(
    void *context,
    Rect2 rect
)
{
    FakeBackendState *state = context;

    state->set_clip_rect_called = 1;
    state->last_clip_rect = rect;
}

static void fake_clear_clip_rect(
    void *context
)
{
    FakeBackendState *state = context;

    state->clear_clip_rect_called = 1;
}

static int nearly_equal(double a, double b)
{
    const double epsilon = 0.000001;

    return fabs(a - b) < epsilon;
}

static void test_renderer2d_create(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    Camera2D camera = renderer2d_get_camera(renderer);

    assert(nearly_equal(camera.position.x, 0.0));
    assert(nearly_equal(camera.position.y, 0.0));
    assert(nearly_equal(camera.scale, 1.0));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_set_camera(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    Camera2D new_camera = {
        .position = {500.0, 250.0},
        .scale = 0.25
    };

    renderer2d_set_camera(renderer, new_camera);

    Camera2D result = renderer2d_get_camera(renderer);

    assert(nearly_equal(result.position.x, 500.0));
    assert(nearly_equal(result.position.y, 250.0));
    assert(nearly_equal(result.scale, 0.25));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_set_viewport(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Viewport2D viewport =
        renderer2d_get_viewport(renderer);

    assert(nearly_equal(viewport.width, 800.0));
    assert(nearly_equal(viewport.height, 600.0));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_draw_rect_transforms_to_screen_space(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .draw_rect = fake_draw_rect
    };

    renderer2d_set_backend(renderer, backend);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {100.0, -100.0},
        .scale = 0.25
    };

    renderer2d_set_camera(renderer, camera);

    Rect2 world_rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Colour colour = {
        .r = 100,
        .g = 150,
        .b = 200,
        .a = 255
    };

    renderer2d_draw_rect(
        renderer,
        world_rect,
        colour
    );

    assert(state.draw_rect_called);

    assert(nearly_equal(state.last_rect.position.x, 125.0));
    assert(nearly_equal(state.last_rect.position.y, -25.0));
    assert(nearly_equal(state.last_rect.width, 22.5));
    assert(nearly_equal(state.last_rect.height, 600.0));

    assert(state.last_colour.r == 100);
    assert(state.last_colour.g == 150);
    assert(state.last_colour.b == 200);
    assert(state.last_colour.a == 255);

    renderer2d_destroy(renderer);
}

static void test_renderer2d_fill_rect_transforms_to_screen_space(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .fill_rect = fake_fill_rect
    };

    renderer2d_set_backend(renderer, backend);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {100.0, -100.0},
        .scale = 0.25
    };

    renderer2d_set_camera(renderer, camera);

    Rect2 world_rect = {
        .position = {600.0, 0.0},
        .width = 90.0,
        .height = 2400.0
    };

    Colour colour = {
        .r = 100,
        .g = 150,
        .b = 200,
        .a = 255
    };

    renderer2d_fill_rect(
        renderer,
        world_rect,
        colour
    );

    assert(state.fill_rect_called);

    assert(nearly_equal(state.last_rect.position.x, 125.0));
    assert(nearly_equal(state.last_rect.position.y, -25.0));
    assert(nearly_equal(state.last_rect.width, 22.5));
    assert(nearly_equal(state.last_rect.height, 600.0));

    assert(state.last_colour.r == 100);
    assert(state.last_colour.g == 150);
    assert(state.last_colour.b == 200);
    assert(state.last_colour.a == 255);

    renderer2d_destroy(renderer);
}

static void test_renderer2d_draw_line_transforms_to_screen_space(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .draw_line = fake_draw_line
    };

    renderer2d_set_backend(renderer, backend);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {100.0, 100.0},
        .scale = 0.5
    };

    renderer2d_set_camera(renderer, camera);

    Vec2 start = {100.0, 100.0};
    Vec2 end = {500.0, 500.0};

    Colour colour = {
        .r = 100,
        .g = 150,
        .b = 200,
        .a = 255
    };

    renderer2d_draw_line(
        renderer,
        start,
        end,
        colour
    );

    assert(state.draw_line_called);

    assert(nearly_equal(state.last_line_start.x, 0.0));
    assert(nearly_equal(state.last_line_start.y, 600.0));

    assert(nearly_equal(state.last_line_end.x, 200.0));
    assert(nearly_equal(state.last_line_end.y, 400.0));

    assert(state.last_colour.r == 100);
    assert(state.last_colour.g == 150);
    assert(state.last_colour.b == 200);
    assert(state.last_colour.a == 255);

    renderer2d_destroy(renderer);
}

static void test_renderer2d_move_camera_changes_screen_position(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .draw_line = fake_draw_line
    };

    renderer2d_set_backend(renderer, backend);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 1.0
    };

    renderer2d_set_camera(renderer, camera);

    Vec2 start = {500.0, 100.0};
    Vec2 end   = {600.0, 100.0};

    Colour colour = {
        .r = 255,
        .g = 255,
        .b = 255,
        .a = 255
    };

    renderer2d_move_camera(
        renderer,
        (Vec2){200.0, 0.0}
    );

    renderer2d_draw_line(
        renderer,
        start,
        end,
        colour
    );

    assert(nearly_equal(
        state.last_line_start.x,
        300.0
    ));

    assert(nearly_equal(
        state.last_line_end.x,
        400.0
    ));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_set_camera_position(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    Camera2D camera = {
        .position = {500.0, 300.0},
        .scale = 1.0
    };

    renderer2d_set_camera(renderer, camera);

    renderer2d_set_camera_position(
        renderer,
        (Vec2){100.0, 50.0}
    );

    Camera2D result = renderer2d_get_camera(renderer);

    assert(nearly_equal(result.position.x, 100.0));
    assert(nearly_equal(result.position.y, 50.0));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_set_camera_scale(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    renderer2d_set_camera_scale(renderer, 0.5);

    Camera2D result = renderer2d_get_camera(renderer);

    assert(nearly_equal(result.scale, 0.5));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_zoom_camera(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    renderer2d_set_camera_scale(
        renderer,
        0.5
    );

    renderer2d_zoom_camera(
        renderer,
        2.0
    );

    Camera2D result = renderer2d_get_camera(renderer);

    assert(nearly_equal(result.scale, 1.0));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_zoom_camera_rejects_invalid_factor(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    renderer2d_set_camera_scale(renderer, 0.5);

    renderer2d_zoom_camera(renderer, 0.0);

    Camera2D result = renderer2d_get_camera(renderer);

    assert(nearly_equal(result.scale, 0.5));

    renderer2d_zoom_camera(renderer, -2.0);

    result = renderer2d_get_camera(renderer);

    assert(nearly_equal(result.scale, 0.5));

    renderer2d_destroy(renderer);
}

static void test_renderer2d_zoom_at_screen_point_keeps_world_point_fixed(void)
{
    Renderer2D *renderer = renderer2d_create();

    assert(renderer != NULL);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {100.0, 50.0},
        .scale = 1.0
    };

    renderer2d_set_camera(renderer, camera);

    Vec2 screen_point = {
        .x = 400.0,
        .y = 300.0
    };

    Camera2D before_camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);

    Vec2 world_before = camera_screen_to_world(
        &before_camera,
        viewport,
        screen_point
    );

    renderer2d_zoom_at_screen_point(
        renderer,
        2.0,
        screen_point
    );

    Camera2D after_camera = renderer2d_get_camera(renderer);

    Vec2 world_after = camera_screen_to_world(
        &after_camera,
        viewport,
        screen_point
    );

    assert(nearly_equal(world_before.x, world_after.x));
    assert(nearly_equal(world_before.y, world_after.y));

    assert(nearly_equal(after_camera.scale, 2.0));

    renderer2d_destroy(renderer);
}

//GUI

static void test_fill_screen_rect_does_not_apply_camera(void)
{
    Renderer2D *renderer =
        renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .fill_rect = fake_fill_rect
    };

    renderer2d_set_backend(
        renderer,
        backend
    );

    Camera2D camera = {
        .position = {
            .x = 500.0,
            .y = -300.0
        },
        .scale = 4.0
    };

    renderer2d_set_camera(
        renderer,
        camera
    );

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        1200.0,
        800.0
    );

    Rect2 screen_rect = {
        .position = {
            .x = 0.0,
            .y = 0.0
        },
        .width = 64.0,
        .height = 800.0
    };

    renderer2d_fill_screen_rect(
        renderer,
        screen_rect,
        (Colour){
            .r = 40,
            .g = 40,
            .b = 40,
            .a = 255
        }
    );

    assert(state.fill_rect_called);

    assert(nearly_equal(
        state.last_rect.position.x,
        0.0
    ));

    assert(nearly_equal(
        state.last_rect.position.y,
        0.0
    ));

    assert(nearly_equal(
        state.last_rect.width,
        64.0
    ));

    assert(nearly_equal(
        state.last_rect.height,
        800.0
    ));

    assert(state.last_colour.r == 40);
    assert(state.last_colour.g == 40);
    assert(state.last_colour.b == 40);
    assert(state.last_colour.a == 255);

    renderer2d_destroy(renderer);
}

static void test_viewport_clip_uses_renderer_viewport(void)
{
    Renderer2D *renderer =
        renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .set_clip_rect = fake_set_clip_rect,
        .clear_clip_rect = fake_clear_clip_rect
    };

    renderer2d_set_backend(
        renderer,
        backend
    );

    renderer2d_set_viewport(
        renderer,
        (Vec2){64.0, 20.0},
        876.0,
        600.0
    );

    renderer2d_begin_viewport_clip(
        renderer
    );

    assert(state.set_clip_rect_called);

    assert(nearly_equal(
        state.last_clip_rect.position.x,
        64.0
    ));

    assert(nearly_equal(
        state.last_clip_rect.position.y,
        20.0
    ));

    assert(nearly_equal(
        state.last_clip_rect.width,
        876.0
    ));

    assert(nearly_equal(
        state.last_clip_rect.height,
        600.0
    ));

    renderer2d_end_viewport_clip(
        renderer
    );

    assert(state.clear_clip_rect_called);

    renderer2d_destroy(renderer);
}

int main(void)
{
    test_renderer2d_create();
    test_renderer2d_set_camera();
    test_renderer2d_set_viewport();
    test_renderer2d_draw_rect_transforms_to_screen_space();
    test_renderer2d_fill_rect_transforms_to_screen_space();
    test_renderer2d_draw_line_transforms_to_screen_space();
    test_renderer2d_move_camera_changes_screen_position();
    test_renderer2d_set_camera_position();
    test_renderer2d_set_camera_scale();
    test_renderer2d_zoom_camera();
    test_renderer2d_zoom_camera_rejects_invalid_factor();
    test_renderer2d_zoom_at_screen_point_keeps_world_point_fixed();

    //GUI
    test_fill_screen_rect_does_not_apply_camera();
    test_viewport_clip_uses_renderer_viewport();

    printf("renderer2d tests passed\n");

    return 0;
}