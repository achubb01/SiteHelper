#include <stdlib.h>

#include "sitehelper.h"
#include "wall_editor.h"
#include "wall_render.h"

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
        .position = {-200.0, -200.0},
        .scale = 0.12
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

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,

        .stud_spacing = 600,
        .nog_spacing = 1200,

        .opening_width_allowance = 0,
        .opening_height_allowance = 0,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    Wall wall = {0};

    if (!wall_set_length(
            &wall,
            4200)) {

        renderer2d_sdl_destroy_backend(&backend);
        renderer2d_destroy(renderer);

        return 1;
    }

    if (!wall_generate(
            &wall,
            &settings)) {

        renderer2d_sdl_destroy_backend(&backend);
        renderer2d_destroy(renderer);

        return 1;
    }

    WallEditor editor;

    wall_editor_init(
        &editor
    );

    WallRenderStyle wall_style = {
        .timber_colour = {
            .r = 200,
            .g = 160,
            .b = 100,
            .a = 255
        },

        .selected_colour = {
            .r = 255,
            .g = 220,
            .b = 40,
            .a = 255
        }
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

        wall_render(
            renderer,
            &wall,
            wall_editor_get_selection(&editor),
            &wall_style
        );

        renderer2d_present(
            renderer
        );
    }

    renderer2d_sdl_destroy_backend(
        &backend
    );

    renderer2d_destroy(
        renderer
    );

    return 0;
}