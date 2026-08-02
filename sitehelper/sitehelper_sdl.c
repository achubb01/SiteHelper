#include <stdlib.h>

#include "sitehelper.h"
#include "wall_editor.h"
#include "wall_render.h"

#include "renderer2d.h"
#include "renderer2d_sdl.h"

typedef struct
{
    Renderer2D *renderer;
    RendererBackend backend;

    Wall wall;
    WallEditor editor;
    WallRenderStyle wall_style;

    Colour background;

    int running;
} SiteHelperApp;

static int sitehelper_app_init(
    SiteHelperApp *app
);

static void sitehelper_app_process_events(
    SiteHelperApp *app
);

static void sitehelper_app_render(
    const SiteHelperApp *app
);

static void sitehelper_app_destroy(
    SiteHelperApp *app
);

static int sitehelper_app_init(
    SiteHelperApp *app
)
{
    if (app == NULL) {
        return 0;
    }

    *app = (SiteHelperApp){0};

    app->renderer = renderer2d_create();

    if (app->renderer == NULL) {
        return 0;
    }

    app->backend =
        renderer2d_sdl_create_backend(
            "SiteHelper",
            800,
            600
        );

    renderer2d_set_backend(
        app->renderer,
        app->backend
    );

    renderer2d_set_viewport(
        app->renderer,
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {-200.0, -200.0},
        .scale = 0.12
    };

    renderer2d_set_camera(
        app->renderer,
        camera
    );

    app->background = (Colour){
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

    if (!wall_set_length(
            &app->wall,
            4200)) {

        sitehelper_app_destroy(app);
        return 0;
    }

    if (!wall_generate(
            &app->wall,
            &settings)) {

        sitehelper_app_destroy(app);
        return 0;
    }

    wall_editor_init(
        &app->editor
    );

    app->wall_style = (WallRenderStyle){
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

    app->running = 1;

    return 1;
}

static void sitehelper_app_render(
    const SiteHelperApp *app
)
{
    if (
        app == NULL
        || app->renderer == NULL
    ) {
        return;
    }

    renderer2d_clear(
        app->renderer,
        app->background
    );

    wall_render(
        app->renderer,
        &app->wall,
        wall_editor_get_selection(&app->editor),
        &app->wall_style
    );

    renderer2d_present(
        app->renderer
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

static void sitehelper_app_process_events(
    SiteHelperApp *app
)
{
    if (
        app == NULL
        || app->renderer == NULL
    ) {
        return;
    }

    Renderer2DEvent event;

    const double pan_amount = 100.0;

    while (renderer2d_sdl_poll_event(&event)) {
        if (event.quit_requested) {
            app->running = 0;
        }

        if (event.viewport_resized) {
            renderer2d_set_viewport(
                app->renderer,
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
                app->renderer,
                &app->editor,
                &app->wall,
                screen_position
            );
        }

        if (event.move_left) {
            renderer2d_move_camera(
                app->renderer,
                (Vec2){
                    .x = -pan_amount,
                    .y = 0.0
                }
            );
        }

        if (event.move_right) {
            renderer2d_move_camera(
                app->renderer,
                (Vec2){
                    .x = pan_amount,
                    .y = 0.0
                }
            );
        }

        if (event.move_up) {
            renderer2d_move_camera(
                app->renderer,
                (Vec2){
                    .x = 0.0,
                    .y = pan_amount
                }
            );
        }

        if (event.move_down) {
            renderer2d_move_camera(
                app->renderer,
                (Vec2){
                    .x = 0.0,
                    .y = -pan_amount
                }
            );
        }

        if (event.pan_dragged) {
            Camera2D camera =
                renderer2d_get_camera(
                    app->renderer
                );

            Vec2 camera_delta = {
                .x =
                    -event.mouse_delta_x
                    / camera.scale,

                .y =
                    event.mouse_delta_y
                    / camera.scale
            };

            renderer2d_move_camera(
                app->renderer,
                camera_delta
            );
        }

        if (event.mouse_wheel) {
            Vec2 screen_position = {
                .x = event.mouse_x,
                .y = event.mouse_y
            };

            double zoom_factor;

            if (event.wheel_y > 0.0) {
                zoom_factor = 1.1;
            }
            else {
                zoom_factor = 1.0 / 1.1;
            }

            renderer2d_zoom_at_screen_point(
                app->renderer,
                zoom_factor,
                screen_position
            );
        }
    }
}

static void sitehelper_app_destroy(
    SiteHelperApp *app
)
{
    if (app == NULL) {
        return;
    }

    wall_destroy(
        &app->wall
    );

    renderer2d_sdl_destroy_backend(
        &app->backend
    );

    renderer2d_destroy(
        app->renderer
    );

    *app = (SiteHelperApp){0};
}

int main(void)
{
    SiteHelperApp app;

    if (!sitehelper_app_init(&app)) {
        return 1;
    }

    while (app.running) {
        sitehelper_app_process_events(&app);
        sitehelper_app_render(&app);
    }

    sitehelper_app_destroy(&app);

    return 0;
}
