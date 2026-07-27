#include <stdlib.h>

#include "renderer2d.h"

struct Renderer2D {
    Camera2D camera;
    Viewport2D viewport;
    RendererBackend backend;
};

void renderer2d_set_camera(
    Renderer2D *renderer,
    Camera2D camera
)
{
    renderer->camera = camera;
}

void renderer2d_set_viewport(
    Renderer2D *renderer,
    double width,
    double height
)
{
    renderer->viewport.width = width;
    renderer->viewport.height = height;
}

Camera2D renderer2d_get_camera(
    const Renderer2D *renderer
)
{
    return renderer->camera;
}

Viewport2D renderer2d_get_viewport(
    const Renderer2D *renderer
)
{
    return renderer->viewport;
}

void renderer2d_move_camera(
    Renderer2D *renderer,
    Vec2 delta
) {
    renderer->camera.position.x += delta.x;
    renderer->camera.position.y += delta.y;
}

void renderer2d_set_camera_position(
    Renderer2D *renderer,
    Vec2 position
)
{
    renderer->camera.position = position;
}

void renderer2d_set_camera_scale(
    Renderer2D *renderer,
    double scale
)
{
    if (scale <= 0.0) {
        return;
    }

    renderer->camera.scale = scale;
}

void renderer2d_zoom_camera(
    Renderer2D *renderer,
    double factor
)
{
    if (factor <= 0.0) {
        return;
    }

    renderer->camera.scale *= factor;
}

void renderer2d_zoom_at_screen_point(
    Renderer2D *renderer,
    double factor,
    Vec2 screen_point
)
{
    if (factor <= 0.0) {
        return;
    }

    Vec2 world_before = camera_screen_to_world(
        &renderer->camera,
        renderer->viewport,
        screen_point
    );

    renderer->camera.scale *= factor;

    Vec2 world_after = camera_screen_to_world(
        &renderer->camera,
        renderer->viewport,
        screen_point
    );

    Vec2 correction = {
        .x = world_before.x - world_after.x,
        .y = world_before.y - world_after.y
    };

    renderer2d_move_camera(
        renderer,
        correction
    );
}

Renderer2D *renderer2d_create(void) {
    Renderer2D *renderer = malloc(sizeof(Renderer2D));

    if (renderer == NULL) {
        return NULL;
    }

    renderer->camera = (Camera2D) {
        .position = {0.0, 0.0},
        .scale = 1.0
    };

    renderer->viewport = (Viewport2D) {
        .width = 0.0,
        .height = 0.0
    };

    renderer->backend = (RendererBackend) {
        .context = NULL,
        .draw_rect = NULL
    };

    return renderer;
}

void renderer2d_destroy(Renderer2D *renderer)
{
    free(renderer);
}

void renderer2d_set_backend(Renderer2D *renderer, RendererBackend backend) {
    renderer->backend = backend;
}

void renderer2d_draw_rect(Renderer2D *renderer, Rect2 rect, Colour colour) {
    if (renderer->backend.draw_rect == NULL) {
        return;
    }

    Rect2 screen_rect = rect2_world_to_screen(
        &renderer->camera,
        renderer->viewport,
        rect
    );

    renderer->backend.draw_rect(
        renderer->backend.context,
        screen_rect,
        colour
    );
}

void renderer2d_fill_rect(Renderer2D *renderer, Rect2 rect, Colour colour) {
    if (renderer->backend.fill_rect == NULL) {
        return;
    }

    Rect2 screen_rect = rect2_world_to_screen(
        &renderer->camera,
        renderer->viewport,
        rect
    );

    renderer->backend.fill_rect(
        renderer->backend.context,
        screen_rect,
        colour
    );
}

void renderer2d_draw_line(
    Renderer2D *renderer,
    Vec2 start,
    Vec2 end,
    Colour colour
)
{
    if (renderer->backend.draw_line == NULL) {
        return;
    }

    Vec2 screen_start = camera_world_to_screen(
        &renderer->camera,
        renderer->viewport,
        start
    );

    Vec2 screen_end = camera_world_to_screen(
        &renderer->camera,
        renderer->viewport,
        end
    );

    renderer->backend.draw_line(
        renderer->backend.context,
        screen_start,
        screen_end,
        colour
    );
}

void renderer2d_clear(
    Renderer2D *renderer,
    Colour colour
)
{
    if (renderer->backend.clear == NULL) {
        return;
    }

    renderer->backend.clear(
        renderer->backend.context,
        colour
    );
}

void renderer2d_present(
    Renderer2D *renderer
)
{
    if (renderer->backend.present == NULL) {
        return;
    }

    renderer->backend.present(
        renderer->backend.context
    );
}