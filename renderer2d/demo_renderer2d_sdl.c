#include <stdio.h>

#include "renderer2d.h"
#include "renderer2d_sdl.h"
#include "platform_event_sdl.h"

int main(void)
{
    Renderer2D *renderer = renderer2d_create();

    if (renderer == NULL) {
        return 1;
    }

    RendererBackend backend = renderer2d_sdl_create_backend(
        "Renderer2D Demo",
        800,
        600
    );

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

    Rect2 rect = {
        .position = {100.0, 100.0},
        .width = 300.0,
        .height = 200.0
    };

    Colour background = {
        .r = 30,
        .g = 30,
        .b = 30,
        .a = 255
    };

    Colour colour = {
        .r = 255,
        .g = 255,
        .b = 255,
        .a = 255
    };

    Vec2 line_start = {100.0, 100.0};
    Vec2 line_end = {400.0, 300.0};

    Colour line_colour = {
        .r = 255,
        .g = 0,
        .b = 0,
        .a = 255
    };

    int running = 1;

    while (running) {
        PlatformEvent event;

        while (platform_event_sdl_poll_event(&event)) {
            if (event.type == PLATFORM_EVENT_QUIT) {
                running = 0;
            }

            if (event.type == PLATFORM_EVENT_WINDOW_RESIZED) {
                renderer2d_set_viewport(
                    renderer,
                    (Vec2){0.0, 0.0},
                    event.data.window_resized.width,
                    event.data.window_resized.height
                );
            }

            const double pan_amount = 50.0;

            if (
                event.type == PLATFORM_EVENT_KEY_DOWN
                && event.data.key_down.key == PLATFORM_KEY_LEFT
            ) {
                renderer2d_move_camera(
                    renderer,
                    (Vec2){-pan_amount, 0.0}
                );
            }

            if (
                event.type == PLATFORM_EVENT_KEY_DOWN
                && event.data.key_down.key == PLATFORM_KEY_RIGHT
            ) {
                renderer2d_move_camera(
                    renderer,
                    (Vec2){pan_amount, 0.0}
                );
            }

            if (
                event.type == PLATFORM_EVENT_KEY_DOWN
                && event.data.key_down.key == PLATFORM_KEY_UP
            ) {
                renderer2d_move_camera(
                    renderer,
                    (Vec2){0.0, pan_amount}
                );
            }

            if (
                event.type == PLATFORM_EVENT_KEY_DOWN
                && event.data.key_down.key == PLATFORM_KEY_DOWN_ARROW
            ) {
                renderer2d_move_camera(
                    renderer,
                    (Vec2){0.0, -pan_amount}
                );
            }

            if (event.type == PLATFORM_EVENT_MOUSE_WHEEL) {
                Vec2 mouse_position = {
                    .x = event.data.mouse_wheel.mouse_x,
                    .y = event.data.mouse_wheel.mouse_y
                };

                double zoom_factor;

                if (event.data.mouse_wheel.delta_y > 0.0) {
                    zoom_factor = 1.1;
                }
                else {
                    zoom_factor = 1.0 / 1.1;
                }

                renderer2d_zoom_at_screen_point(
                    renderer,
                    zoom_factor,
                    mouse_position
                );
            }
        }

        renderer2d_clear(renderer, background);
        renderer2d_fill_rect(renderer, rect, colour);
        renderer2d_draw_line(renderer, line_start, line_end, line_colour);
        renderer2d_present(renderer);
    }

    renderer2d_sdl_destroy_backend(&backend);
    renderer2d_destroy(renderer);

    return 0;
}
