#include <stdlib.h>

#include "renderer2d.h"
#include "renderer2d_sdl.h"

int main(void)
{
    Renderer2D *renderer = renderer2d_create();

    if (renderer == NULL) {
        return 1;
    }

    RendererBackend backend =
        renderer2d_sdl_create_backend(
            "SiteHelper",
            800,
            600
        );

    renderer2d_set_backend(
        renderer,
        backend
    );

    renderer2d_set_viewport(
        renderer,
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 1.0
    };

    renderer2d_set_camera(
        renderer,
        camera
    );

    Colour background = {
        .r = 30,
        .g = 30,
        .b = 30,
        .a = 255
    };

    int running = 1;

    while (running) {
        Renderer2DEvent event;

        while (renderer2d_sdl_poll_event(&event)) {
            if (event.quit_requested) {
                running = 0;
            }

            if (event.viewport_resized) {
                renderer2d_set_viewport(
                    renderer,
                    event.viewport_width,
                    event.viewport_height
                );
            }
        }

        renderer2d_clear(
            renderer,
            background
        );

        renderer2d_present(renderer);
    }

    renderer2d_sdl_destroy_backend(
        &backend
    );

    renderer2d_destroy(renderer);

    return 0;
}