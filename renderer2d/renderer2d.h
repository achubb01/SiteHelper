#ifndef RENDERER2D_H
#define RENDERER2D_H

#include "../geometry/geometry.h"
#include "renderer2d_backend.h"

/* Drawing primitives and camera positions/deltas use caller world units.
 * Viewports, screen_point and explicit *_screen_* primitives use pixels.
 * Camera scale is pixels per world unit; zoom factors are dimensionless.
 * SiteHelper supplies millimetres as world units; renderer2d stays generic. */
typedef struct Renderer2D Renderer2D;

void renderer2d_draw_line(
    Renderer2D *renderer,
    Vec2 start,
    Vec2 end,
    Colour colour
);

void renderer2d_draw_rect(
    Renderer2D *renderer,
    Rect2 rect,
    Colour colour
);

void renderer2d_fill_rect(
    Renderer2D *renderer,
    Rect2 rect,
    Colour colour
);

void renderer2d_draw_line(
    Renderer2D *renderer,
    Vec2 start,
    Vec2 end,
    Colour colour
);

void renderer2d_set_camera(
    Renderer2D *renderer,
    Camera2D camera
);

void renderer2d_set_viewport(
    Renderer2D *renderer,
    Vec2 position,
    double width,
    double height
);

Camera2D renderer2d_get_camera(
    const Renderer2D *renderer
);

Viewport2D renderer2d_get_viewport(
    const Renderer2D *renderer
);

void renderer2d_move_camera(
    Renderer2D *renderer,
    Vec2 delta
);

void renderer2d_set_camera_position(
    Renderer2D *renderer,
    Vec2 position
);

void renderer2d_set_camera_scale(
    Renderer2D *renderer,
    double scale
);

void renderer2d_zoom_camera(
    Renderer2D *renderer,
    double factor
);

void renderer2d_zoom_at_screen_point(
    Renderer2D *renderer,
    double factor,
    Vec2 screen_point
);

Renderer2D *renderer2d_create(void);

void renderer2d_destroy(
    Renderer2D *renderer
);

void renderer2d_set_backend(
    Renderer2D *renderer,
    RendererBackend backend
);


void renderer2d_draw_rect(
    Renderer2D *renderer,
    Rect2 rect,
    Colour colour
);

void renderer2d_clear(
    Renderer2D *renderer,
    Colour colour
);

void renderer2d_present(
    Renderer2D *renderer
);


//GUI

void renderer2d_fill_screen_rect(
    Renderer2D *renderer,
    Rect2 rect,
    Colour colour
);

void renderer2d_begin_viewport_clip(
    Renderer2D *renderer
);

void renderer2d_end_viewport_clip(
    Renderer2D *renderer
);

/* Temporary single-line HUD text. Backend typography is replaceable. */
void renderer2d_draw_screen_text(Renderer2D *renderer, Vec2 position,
    const char *text, Colour colour);

#endif
