#include <stdlib.h>

#include "sitehelper_project.h"
#include "sitehelper_editor.h"
#include "appstate.h"

#include "domain_id.h"

#include "editor_tool.h"

#include "opening_command.h"

#include "wall_render.h"

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

    SiteHelperProject project;
    SiteHelperEditor editor;

    WallRenderStyle wall_style;
    GridRenderStyle grid_style;

    Colour background;

    GuiLayout gui_layout;
    GuiRenderStyle gui_style;

    GuiButton toolbar_buttons[
        EDITOR_TOOL_COUNT
    ];

    GuiToolbar toolbar;

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

static void sitehelper_app_update_editor_pointer(
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

static Wall *sitehelper_app_current_wall(
    SiteHelperApp *app
);


static Wall *sitehelper_app_current_wall(
    SiteHelperApp *app
)
{
    if (app == NULL) {
        return NULL;
    }

    return app_current_wall(
        &app->project,
        &app->editor
    );
}

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

    sitehelper_project_init(
        &app->project
    );

    sitehelper_editor_init(
        &app->editor
    );

    sitehelper_app_set_active_tool(
        app,
        sitehelper_editor_get_active_tool(
            &app->editor
        )
    );

    DomainId room_id =
        sitehelper_project_add_room(
            &app->project
        );

    if (room_id == DOMAIN_ID_INVALID) {
        sitehelper_app_destroy(app);
        return 0;
    }

    Room *room =
        build_find_room_by_id(
            &app->project.structure,
            room_id
        );

    if (room == NULL) {
        sitehelper_app_destroy(app);
        return 0;
    }

    DomainId wall_id =
        sitehelper_project_add_wall(
            &app->project,
            room_id
        );

    if (wall_id == DOMAIN_ID_INVALID) {
        sitehelper_app_destroy(app);
        return 0;
    }

    Wall *wall =
        room_find_wall_by_id(
            room,
            wall_id
        );

    if (wall == NULL) {
        sitehelper_app_destroy(app);
        return 0;
    }

    app->editor.current_room_id =
        room_id;

    app->editor.current_wall_id =
        wall_id;

    if (!wall_set_length(
            wall,
            4200)) {

        sitehelper_app_destroy(app);
        return 0;
    }

    if (!wall_generate(
            wall,
            &app->project.settings)) {

        sitehelper_app_destroy(app);
        return 0;
    }

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

    const Wall *wall =
        app_current_wall_const(
            &app->project,
            &app->editor
        );

    if (wall != NULL) {
        const EditorSelection *selection =
            sitehelper_editor_get_selection(
                &app->editor
            );

        const WallSelection *wall_selection =
            editor_selection_get_wall_member(
                selection,
                wall->id
            );

        const Timber *selected =
            wall_selection_resolve(
                wall_selection,
                wall
            );

        wall_render(
            app->renderer,
            wall,
            selected,
            &app->wall_style
        );
    }

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(
            &app->editor
        );

    if (
        sitehelper_editor_get_active_tool(
            &app->editor
        ) == EDITOR_TOOL_OPENING
        && placement != NULL
        && placement->valid
    ) {
        Rect2 preview_rect = {
            .position = {
                .x = placement->left,
                .y = placement->bottom
            },

            .width =
                (double)placement->width,

            .height =
                (double)placement->height
        };

        renderer2d_draw_rect(
            app->renderer,
            preview_rect,
            (Colour){
                .r = 100,
                .g = 180,
                .b = 255,
                .a = 255
            }
        );
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
    SiteHelperEditor *editor,
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

    sitehelper_editor_select_wall_member_at_position(
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

            sitehelper_app_update_editor_pointer(
                app,
                screen_position
            );
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
                switch (
                    sitehelper_editor_get_active_tool(
                        &app->editor
                    )
                ) {
                    case EDITOR_TOOL_SELECT:
                    {
                        Wall *wall =
                            sitehelper_app_current_wall(
                                app
                            );

                        if (wall != NULL) {
                            select_at_screen_position(
                                app->renderer,
                                &app->editor,
                                wall,
                                screen_position
                            );
                        }

                        break;
                    }

                    case EDITOR_TOOL_OPENING:
                    {
                        DomainId opening_id =
                            domain_id_generate(
                                &app->project.domain_ids
                            );

                        if (opening_id == DOMAIN_ID_INVALID) {
                            /* handle failure */
                            continue;
                        }

                        OpeningCommand command;

                        if (!sitehelper_editor_create_opening_command(
                                &app->editor,
                                opening_id,
                                &command)) {
                            break;
                        }

                        Wall *wall =
                            sitehelper_app_current_wall(
                                app
                            );

                        if (wall == NULL) {
                            break;
                        }

                        if (!opening_command_execute(
                                wall,
                                &app->project.settings,
                                &command)) {

                            break;
                        }

                        app->editor.opening_placement =
                            (OpeningPlacement){0};

                        break;
                    }

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
                sitehelper_editor_get_active_tool(
                    &app->editor
                )
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

    sitehelper_project_destroy(
        &app->project
    );

    renderer2d_sdl_destroy_backend(
        &app->backend
    );

    renderer2d_destroy(
        app->renderer
    );

    *app = (SiteHelperApp){0};
}

static void sitehelper_app_update_editor_pointer(
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

    if (
        !rect2_contains_point(
            app->gui_layout.viewport,
            screen_position
        )
    ) {
        sitehelper_editor_pointer_leave(
            &app->editor
        );

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

    Wall *wall =
        sitehelper_app_current_wall(
            app
        );

    sitehelper_editor_pointer_move(
        &app->editor,
        wall,
        world_position
    );
}

static void sitehelper_app_render_snap_cursor(
    const SiteHelperApp *app
)
{
    if (
        app == NULL
        || app->renderer == NULL
        || !sitehelper_editor_has_snap(
            &app->editor
        )
    ) {
        return;
    }

    const SnapResult *snap_result =
        sitehelper_editor_get_snap_result(
            &app->editor
        );

    if (snap_result == NULL) {
        return;
    }

    const double marker_radius =
        40.0;

    Colour marker_colour;

    switch (snap_result->type) {
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
        snap_result->position;

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
    if (app == NULL) {
        return;
    }

    if (!sitehelper_editor_set_active_tool(
            &app->editor,
            tool)) {
        return;
    }

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
