#include <stdlib.h>

#include "sitehelper_project.h"
#include "sitehelper_editor.h"
#include "appstate.h"
#include "app_view.h"
#include "viewport_input.h"

#include "domain_id.h"

#include "editor_tool.h"

#include "sitehelper_command.h"
#include "opening_command.h"
#include "command_history.h"

#include "wall_render.h"

#include "grid_render.h"

#include "gui_layout.h"
#include "gui_render.h"
#include "gui_button.h"
#include "gui_toolbar.h"

#include "renderer2d.h"
#include "renderer2d_sdl.h"
#include "platform_event_sdl.h"

typedef enum
{
    SITEHELPER_TOOLBAR_ACTION_SELECT = 10,
    SITEHELPER_TOOLBAR_ACTION_OPENING = 20,
    SITEHELPER_TOOLBAR_ACTION_WALL = 30
} SiteHelperToolbarAction;

enum
{
    SITEHELPER_TOOLBAR_BUTTON_COUNT = 3
};

static const GuiButtonId sitehelper_toolbar_button_ids[
    SITEHELPER_TOOLBAR_BUTTON_COUNT
] = {
    SITEHELPER_TOOLBAR_ACTION_SELECT,
    SITEHELPER_TOOLBAR_ACTION_OPENING,
    SITEHELPER_TOOLBAR_ACTION_WALL
};

typedef struct
{
    Renderer2D *renderer;
    RendererBackend backend;

    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;

    WallRenderStyle wall_style;
    GridRenderStyle grid_style;

    Colour background;

    GuiLayout gui_layout;
    GuiRenderStyle gui_style;

    GuiButton toolbar_buttons[
        SITEHELPER_TOOLBAR_BUTTON_COUNT
    ];

    GuiToolbar toolbar;

    ViewportInput viewport_input;
    AppViews views;

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

static int sitehelper_app_toolbar_action_tool(
    GuiButtonId action,
    EditorTool *tool
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
            "SiteHelper (Tab: Plan / Wall Elevation)",
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

    app_views_init(&app->views, camera);
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

    sitehelper_app_layout_gui(app);

    sitehelper_command_history_init(
        &app->history
    );

    viewport_input_init(&app->viewport_input);

    sitehelper_app_set_active_tool(
        app,
        sitehelper_editor_get_active_tool(
            &app->editor
        )
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

    app_render_walls(app->renderer, &app->project, &app->editor, &app->wall_style);

    Rect2 preview_rect;

    if (
        sitehelper_editor_get_opening_preview_rect(
            &app->editor,
            &preview_rect
        )
    ) {
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

    WallPlanSegment preview_segment;
    if (sitehelper_editor_get_wall_preview_segment(&app->editor, &preview_segment)) {
        renderer2d_draw_line(
            app->renderer,
            (Vec2){ .x = preview_segment.start.x, .y = preview_segment.start.y },
            (Vec2){ .x = preview_segment.end.x, .y = preview_segment.end.y },
            (Colour){ .r = 100, .g = 220, .b = 150, .a = 255 }
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

    PlatformEvent event;

    const double pan_amount = 100.0;

    while (platform_event_sdl_poll_event(&event)) {
        switch (event.type) {
            case PLATFORM_EVENT_QUIT:
                app->running = 0;
                break;

            case PLATFORM_EVENT_KEY_DOWN:
            {
                PlatformKey key = event.data.key_down.key;
                int modifiers = event.data.key_down.modifiers;

                if (
                    !event.data.key_down.repeat
                    && (
                        (
                            key == PLATFORM_KEY_Y
                            && (modifiers & PLATFORM_MODIFIER_CTRL)
                        )
                        ||
                        (
                            key == PLATFORM_KEY_Z
                            && (modifiers & PLATFORM_MODIFIER_CTRL)
                            && (modifiers & PLATFORM_MODIFIER_SHIFT)
                        )
                    )
                ) {
                    if (sitehelper_command_history_redo(
                            &app->history,
                            &app->project)) {

                        sitehelper_editor_reconcile(
                            &app->editor,
                            &app->project
                        );
                    }

                    continue;
                }

                if (
                    !event.data.key_down.repeat
                    && key == PLATFORM_KEY_Z
                    && (modifiers & PLATFORM_MODIFIER_CTRL)
                ) {
                    if (sitehelper_command_history_undo(
                            &app->history,
                            &app->project)) {

                        sitehelper_editor_reconcile(
                            &app->editor,
                            &app->project
                        );
                    }

                    continue;
                }

                if (key == PLATFORM_KEY_TAB && !event.data.key_down.repeat &&
                    modifiers == PLATFORM_MODIFIER_NONE) {
                    EditorView view = app->editor.active_view == EDITOR_VIEW_PLAN
                        ? EDITOR_VIEW_WALL_ELEVATION : EDITOR_VIEW_PLAN;
                    app_views_set_active(&app->views, &app->editor, app->renderer, view);
                    viewport_input_end_middle_drag(&app->viewport_input);
                    sitehelper_app_set_active_tool(app, app->editor.active_tool);
                    continue;
                }

                switch (key) {
                    case PLATFORM_KEY_LEFT:
                        renderer2d_move_camera(
                            app->renderer,
                            (Vec2){-pan_amount, 0.0}
                        );
                        break;

                    case PLATFORM_KEY_RIGHT:
                        renderer2d_move_camera(
                            app->renderer,
                            (Vec2){pan_amount, 0.0}
                        );
                        break;

                    case PLATFORM_KEY_UP:
                        renderer2d_move_camera(
                            app->renderer,
                            (Vec2){0.0, pan_amount}
                        );
                        break;

                    case PLATFORM_KEY_DOWN_ARROW:
                        renderer2d_move_camera(
                            app->renderer,
                            (Vec2){0.0, -pan_amount}
                        );
                        break;

                    default:
                        break;
                }

                break;
            }

            case PLATFORM_EVENT_MOUSE_MOTION:
            {
                Vec2 screen_position = {
                    .x = event.data.mouse_motion.x,
                    .y = event.data.mouse_motion.y
                };

                if (
                    viewport_input_allows_pan(
                        &app->viewport_input
                    )
                    && (
                        event.data.mouse_motion.held_buttons
                        & PLATFORM_MOUSE_BUTTON_STATE_MIDDLE
                    )
                ) {
                    Camera2D camera = renderer2d_get_camera(
                        app->renderer
                    );

                    Vec2 camera_delta = {
                        .x =
                            -event.data.mouse_motion.delta_x
                            / camera.scale,
                        .y =
                            event.data.mouse_motion.delta_y
                            / camera.scale
                    };

                    renderer2d_move_camera(
                        app->renderer,
                        camera_delta
                    );
                }

                gui_toolbar_mouse_move(
                    &app->toolbar,
                    screen_position
                );

                sitehelper_app_update_editor_pointer(
                    app,
                    screen_position
                );
                break;
            }

            case PLATFORM_EVENT_MOUSE_WHEEL:
            {
                Vec2 screen_position = {
                    .x = event.data.mouse_wheel.mouse_x,
                    .y = event.data.mouse_wheel.mouse_y
                };

                if (viewport_input_allows_wheel(
                        &app->gui_layout,
                        screen_position)) {
                    double zoom_factor =
                        event.data.mouse_wheel.delta_y > 0.0
                        ? 1.1
                        : 1.0 / 1.1;

                    renderer2d_zoom_at_screen_point(
                        app->renderer,
                        zoom_factor,
                        screen_position
                    );
                }
                break;
            }

            case PLATFORM_EVENT_MOUSE_BUTTON_DOWN:
                if (
                    event.data.mouse_button.button
                    == PLATFORM_MOUSE_BUTTON_PRIMARY
                ) {
                    (void)gui_toolbar_mouse_press(
                        &app->toolbar,
                        (Vec2){
                            .x = event.data.mouse_button.x,
                            .y = event.data.mouse_button.y
                        }
                    );
                }
                else if (
                    event.data.mouse_button.button
                    == PLATFORM_MOUSE_BUTTON_MIDDLE
                ) {
                    viewport_input_begin_middle_drag(
                        &app->viewport_input,
                        &app->gui_layout,
                        (Vec2){
                            .x = event.data.mouse_button.x,
                            .y = event.data.mouse_button.y
                        }
                    );
                }
                break;

            case PLATFORM_EVENT_MOUSE_BUTTON_UP:
                if (
                    event.data.mouse_button.button
                    == PLATFORM_MOUSE_BUTTON_PRIMARY
                ) {
                    Vec2 screen_position = {
                        .x = event.data.mouse_button.x,
                        .y = event.data.mouse_button.y
                    };

                    GuiToolbarResult toolbar_result =
                        gui_toolbar_mouse_release(
                        &app->toolbar,
                        screen_position
                    );

                    if (toolbar_result.handled) {
                        EditorTool tool;

                        if (sitehelper_app_toolbar_action_tool(
                                toolbar_result.action_id,
                                &tool)) {
                            sitehelper_app_set_active_tool(app, tool);
                        }
                    }
                    else if (
                        rect2_contains_point(
                            app->gui_layout.viewport,
                            screen_position
                        )
                    ) {
                        Camera2D camera = renderer2d_get_camera(
                            app->renderer
                        );
                        Viewport2D viewport = renderer2d_get_viewport(
                            app->renderer
                        );
                        Vec2 view_position = camera_screen_to_world(
                            &camera,
                            viewport,
                            screen_position
                        );
                        EditorAction action;

                        if (!sitehelper_editor_primary_action_in_project(
                            &app->editor,
                            &app->project.structure,
                            view_position,
                            &action)) {
                            continue;
                        }

                        if (action.kind == EDITOR_ACTION_COMMAND) {
                            SiteHelperCommandResult result;

                            if (sitehelper_command_history_execute(
                                    &app->history,
                                    &app->project,
                                    &action.command,
                                    &result)) {
                                sitehelper_editor_complete_action(
                                    &app->editor,
                                    &action,
                                    &result
                                );
                            }
                        }
                    }
                }
                else if (
                    event.data.mouse_button.button
                    == PLATFORM_MOUSE_BUTTON_MIDDLE
                ) {
                    viewport_input_end_middle_drag(
                        &app->viewport_input
                    );
                }
                break;

            case PLATFORM_EVENT_WINDOW_RESIZED:
                app->gui_layout = gui_layout_create(
                    event.data.window_resized.width,
                    event.data.window_resized.height
                );

                renderer2d_set_viewport(
                    app->renderer,
                    app->gui_layout.viewport.position,
                    app->gui_layout.viewport.width,
                    app->gui_layout.viewport.height
                );

                app->toolbar.bounds = app->gui_layout.toolbar;
                gui_toolbar_layout(&app->toolbar);

                sitehelper_app_set_active_tool(
                    app,
                    sitehelper_editor_get_active_tool(&app->editor)
                );
                break;

            case PLATFORM_EVENT_NONE:
            default:
                break;
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

    sitehelper_command_history_destroy(
        &app->history
    );

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

    Vec2 view_position =
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
        &app->project.settings,
        view_position
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
        sitehelper_toolbar_button_ids,
        SITEHELPER_TOOLBAR_BUTTON_COUNT,
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
        EditorTool button_tool;
        int known = sitehelper_app_toolbar_action_tool(
            app->toolbar.buttons[i].id, &button_tool
        );
        gui_button_set_enabled(&app->toolbar.buttons[i], known &&
            sitehelper_editor_tool_available(app->editor.active_view, button_tool));

        gui_button_set_active(
            &app->toolbar.buttons[i],
            sitehelper_app_toolbar_action_tool(
                app->toolbar.buttons[i].id,
                &button_tool
            )
            && button_tool == tool
        );
    }
}

static int sitehelper_app_toolbar_action_tool(
    GuiButtonId action,
    EditorTool *tool
)
{
    if (tool == NULL) {
        return 0;
    }

    switch (action) {
        case SITEHELPER_TOOLBAR_ACTION_SELECT:
            *tool = EDITOR_TOOL_SELECT;
            return 1;

        case SITEHELPER_TOOLBAR_ACTION_OPENING:
            *tool = EDITOR_TOOL_OPENING;
            return 1;

        case SITEHELPER_TOOLBAR_ACTION_WALL:
            *tool = EDITOR_TOOL_WALL;
            return 1;

        default:
            return 0;
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
