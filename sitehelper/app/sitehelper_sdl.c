#include <stdlib.h>

#include "snap.h"
#include "editor_tool.h"
#include "opening_tool.h"
#include "opening_placement.h"

#include "wall_editor.h"
#include "wall_render.h"
#include "wall_snap.h"

#include "grid_render.h"

#include "gui_layout.h"
#include "gui_render.h"
#include "gui_button.h"
#include "gui_toolbar.h"

#include "renderer2d.h"
#include "renderer2d_sdl.h"

typedef struct
{
    Renderer2D *renderer;
    RendererBackend backend;

    Wall wall;
    WallEditor editor;

    WallRenderStyle wall_style;
    GridRenderStyle grid_style;

    SnapResult snap_result;
    int snapped_cursor_visible;

    SnapSettings snap_settings;

    Colour background;

    GuiLayout gui_layout;
    GuiRenderStyle gui_style;

    GuiButton toolbar_buttons[
        EDITOR_TOOL_COUNT
    ];

    GuiToolbar toolbar;

    EditorTool active_tool;

    OpeningTool opening_tool;

    OpeningPlacement opening_placement;

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

static void sitehelper_app_update_snap_cursor(
    SiteHelperApp *app,
    Vec2 screen_position
);

static void sitehelper_app_render_snap_cursor(
    const SiteHelperApp *app
);

static void sitehelper_app_layout_gui(
    SiteHelperApp *app
);

static void sitehelper_app_set_active_tool(
    SiteHelperApp *app,
    EditorTool tool
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

    app->grid_style = (GridRenderStyle){
        .minor_colour = {
            .r = 55,
            .g = 55,
            .b = 55,
            .a = 255
        },

        .major_colour = {
            .r = 75,
            .g = 75,
            .b = 75,
            .a = 255
        },

        .axis_colour = {
            .r = 105,
            .g = 105,
            .b = 105,
            .a = 255
        },

        .minimum_screen_spacing = 16.0
    };

    app->snap_settings = (SnapSettings){
        .grid_enabled = 1,
        .grid_spacing = 100.0,

        .endpoint_enabled = 1,
        .intersection_enabled = 1,

        .object_snap_tolerance = 80.0
    };

    app->snap_result = (SnapResult){
        .position = {0.0, 0.0},
        .type = SNAP_NONE
    };

    app->snapped_cursor_visible = 0;

    app->gui_layout =
        gui_layout_create(
            800.0,
            600.0
        );

    renderer2d_set_viewport(
        app->renderer,
        app->gui_layout.viewport.position,
        app->gui_layout.viewport.width,
        app->gui_layout.viewport.height
    );

    app->gui_style = (GuiRenderStyle){
        .toolbar_colour = {
            .r = 42,
            .g = 42,
            .b = 42,
            .a = 255
        },

        .properties_colour = {
            .r = 48,
            .g = 48,
            .b = 48,
            .a = 255
        },
        
        .button_idle_colour = {
            .r = 55,
            .g = 55,
            .b = 55,
            .a = 255
        },

        .button_hover_colour = {
            .r = 70,
            .g = 70,
            .b = 70,
            .a = 255
        },

        .button_pressed_colour = {
            .r = 45,
            .g = 45,
            .b = 45,
            .a = 255
        },

        .button_active_colour = {
            .r = 80,
            .g = 110,
            .b = 150,
            .a = 255
        },

        .button_disabled_colour = {
            .r = 35,
            .g = 35,
            .b = 35,
            .a = 255
        }
    
    };

    sitehelper_app_layout_gui(
        app
    );

    opening_tool_init(
        &app->opening_tool
    );

    app->opening_placement =
        (OpeningPlacement){0};

    sitehelper_app_set_active_tool(
        app,
        EDITOR_TOOL_SELECT
    );
    
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

    renderer2d_begin_viewport_clip(
        app->renderer
    );

    grid_render(
        app->renderer,
        &app->grid_style
    );

    wall_render(
        app->renderer,
        &app->wall,
        wall_editor_get_selection(
            &app->editor
        ),
        &app->wall_style
    );

    if (
        app->active_tool
            == EDITOR_TOOL_OPENING
    ) {
        if (
            app->active_tool
                == EDITOR_TOOL_OPENING
            && app->opening_placement.valid
        ) {
            renderer2d_draw_rect(
                app->renderer,
                app->opening_placement.preview,
                (Colour){
                    .r = 100,
                    .g = 180,
                    .b = 255,
                    .a = 255
                }
            );
        }
    }

    sitehelper_app_render_snap_cursor(
        app
    );

    renderer2d_end_viewport_clip(
        app->renderer
    );

    gui_render(
        app->renderer,
        &app->gui_layout,
        &app->gui_style
    );

    gui_render_toolbar(
        app->renderer,
        &app->toolbar,
        &app->gui_style
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

        if (event.primary_mouse_pressed) {
            Vec2 screen_position = {
                .x = event.mouse_x,
                .y = event.mouse_y
            };

            gui_toolbar_mouse_press(
                &app->toolbar,
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

        if (event.mouse_moved) {
            Vec2 screen_position = {
                .x = event.mouse_x,
                .y = event.mouse_y
            };

            gui_toolbar_mouse_move(
                &app->toolbar,
                screen_position
            );

            sitehelper_app_update_snap_cursor(
                app,
                screen_position
            );

            if (
                app->active_tool
                    == EDITOR_TOOL_OPENING
                && app->snap_result.type
                    != SNAP_NONE
            ) {
                opening_tool_update_preview(
                    &app->opening_tool,
                    app->snap_result.position
                );

                app->opening_placement =
                    opening_find_placement(
                        &app->wall,
                        app->snap_result.position,
                        &app->opening_tool
                    );
            }
            else {
                app->opening_placement =
                    (OpeningPlacement){0};
            }
        }

        if (event.primary_mouse_released) {
            Vec2 screen_position = {
                .x = event.mouse_x,
                .y = event.mouse_y
            };

            int clicked_button =
                gui_toolbar_mouse_release(
                    &app->toolbar,
                    screen_position
                );

            if (clicked_button >= 0) {
                sitehelper_app_set_active_tool(
                    app,
                    (EditorTool)clicked_button
                );
            }
            else if (
                rect2_contains_point(
                    app->gui_layout.viewport,
                    screen_position
                )
            ) {
                switch (app->active_tool) {
                    case EDITOR_TOOL_SELECT:
                        select_at_screen_position(
                            app->renderer,
                            &app->editor,
                            &app->wall,
                            screen_position
                        );
                        break;

                    case EDITOR_TOOL_OPENING:
                        /*
                        * Opening placement comes next.
                        */
                        break;

                    case EDITOR_TOOL_WALL:
                        /*
                        * Wall tool comes later.
                        */
                        break;

                    default:
                        break;
                }
            }
        }

        if (event.viewport_resized) {
            app->gui_layout =
                gui_layout_create(
                    event.viewport_width,
                    event.viewport_height
                );

            renderer2d_set_viewport(
                app->renderer,
                app->gui_layout.viewport.position,
                app->gui_layout.viewport.width,
                app->gui_layout.viewport.height
            );

            app->toolbar.bounds =
                app->gui_layout.toolbar;

            gui_toolbar_layout(
                &app->toolbar
            );

            sitehelper_app_set_active_tool(
                app,
                app->active_tool
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

static void sitehelper_app_update_snap_cursor(
    SiteHelperApp *app,
    Vec2 screen_position
)
{
    if (
        app == NULL
        || app->renderer == NULL
    ) {
        return;
    }

    Camera2D camera =
        renderer2d_get_camera(
            app->renderer
        );

    Viewport2D viewport =
        renderer2d_get_viewport(
            app->renderer
        );

    Vec2 world_position =
        camera_screen_to_world(
            &camera,
            viewport,
            screen_position
        );

    enum {
        MAX_SNAP_CANDIDATES = 256
    };

    SnapCandidate candidates[
        MAX_SNAP_CANDIDATES
    ];

    size_t candidate_count =
        wall_collect_snap_candidates(
            &app->wall,
            candidates,
            MAX_SNAP_CANDIDATES
        );

    app->snap_result =
        editor_snap(
            world_position,
            candidates,
            candidate_count,
            &app->snap_settings
        );

    app->snapped_cursor_visible =
        app->snap_result.type != SNAP_NONE;
}

static void sitehelper_app_render_snap_cursor(
    const SiteHelperApp *app
)
{
    if (
        app == NULL
        || app->renderer == NULL
        || !app->snapped_cursor_visible
    ) {
        return;
    }

    const double marker_radius =
        40.0;

    Colour marker_colour;

    switch (app->snap_result.type) {
        case SNAP_ENDPOINT:
            marker_colour = (Colour){
                .r = 255,
                .g = 180,
                .b = 60,
                .a = 255
            };
            break;

        case SNAP_GRID:
            marker_colour = (Colour){
                .r = 80,
                .g = 200,
                .b = 255,
                .a = 255
            };
            break;

        case SNAP_INTERSECTION:
            marker_colour = (Colour){
                .r = 80,
                .g = 255,
                .b = 120,
                .a = 255
            };
            break;

        default:
            return;
    }

    Vec2 position =
        app->snap_result.position;

    renderer2d_draw_line(
        app->renderer,
        (Vec2){
            .x = position.x - marker_radius,
            .y = position.y
        },
        (Vec2){
            .x = position.x + marker_radius,
            .y = position.y
        },
        marker_colour
    );

    renderer2d_draw_line(
        app->renderer,
        (Vec2){
            .x = position.x,
            .y = position.y - marker_radius
        },
        (Vec2){
            .x = position.x,
            .y = position.y + marker_radius
        },
        marker_colour
    );
}

static void sitehelper_app_layout_gui(
    SiteHelperApp *app
)
{
    if (app == NULL) {
        return;
    }

    gui_toolbar_init(
        &app->toolbar,
        app->toolbar_buttons,
        EDITOR_TOOL_COUNT,
        app->gui_layout.toolbar
    );
}

static void sitehelper_app_set_active_tool(
    SiteHelperApp *app,
    EditorTool tool
)
{
    if (
        app == NULL
        || tool < 0
        || tool >= EDITOR_TOOL_COUNT
    ) {
        return;
    }

    app->active_tool = tool;

    for (
        size_t i = 0;
        i < app->toolbar.button_count;
        i++
    ) {
        gui_button_set_active(
            &app->toolbar.buttons[i],
            i == (size_t)tool
        );
    }

    if (tool == EDITOR_TOOL_OPENING) {
        opening_tool_activate(
            &app->opening_tool
        );
    }
    else {
        opening_tool_cancel(
            &app->opening_tool
        );
    }
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
