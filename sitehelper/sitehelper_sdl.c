#include <stdlib.h>

#include "sitehelper.h"
#include "wall_editor.h"
#include "wall_render.h"

#include "renderer2d.h"
#include "renderer2d_sdl.h"

static void render_scene(
    Renderer2D *renderer,
    const Wall *wall,
    const WallEditor *editor,
    const WallRenderStyle *style,
    Colour background
)
{
    if (
        renderer == NULL
        || wall == NULL
        || editor == NULL
        || style == NULL
    ) {
        return;
    }

    renderer2d_clear(
        renderer,
        background
    );

    wall_render(
        renderer,
        wall,
        wall_editor_get_selection(editor),
        style
    );

    renderer2d_present(
        renderer
    );
}

static void select_at_screen_position(
    Renderer2D *renderer,
    WallEditor *editor,
    const Wall *wall,
    Vec2 screen_position
)
{
    if (
        renderer == NULL
        || editor == NULL
        || wall == NULL
    ) {
        return;
    }

    Camera2D camera =
        renderer2d_get_camera(renderer);

    Viewport2D viewport =
        renderer2d_get_viewport(renderer);

    Vec2 world_position =
        camera_screen_to_world(
            &camera,
            viewport,
            screen_position
        );

    Position position = {
        .x = (int)world_position.x,
        .y = (int)world_position.y
    };

    wall_editor_select_at_position(
        editor,
        wall,
        position
    );
}

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

            if (event.primary_mouse_pressed) {
                Vec2 screen_position = {
                    .x = event.mouse_x,
                    .y = event.mouse_y
                };

                select_at_screen_position(
                    renderer,
                    &editor,
                    &wall,
                    screen_position
                );
            }
        }

        render_scene(
            renderer,
            &wall,
            &editor,
            &wall_style,
            background
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