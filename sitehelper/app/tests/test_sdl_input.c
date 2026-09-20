#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "sitehelper_persistence.h"

/* Exercise the real private application wiring without exposing it as a public
 * library API or adding test hooks to production state. */
#define main sitehelper_sdl_entry
#include "../sitehelper_sdl.c"
#undef main

static void send_key(SiteHelperApp *app, SDL_Keycode key, SDL_Keymod modifiers)
{
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.key = key; event.key.mod = modifiers;
    assert(SDL_PushEvent(&event)); sitehelper_app_process_events(app);
}
static void send_text(SiteHelperApp *app, const char *text)
{
    SDL_Event event = {.type = SDL_EVENT_TEXT_INPUT}; event.text.text = text;
    assert(SDL_PushEvent(&event)); sitehelper_app_process_events(app);
}
static void click(SiteHelperApp *app, float x, float y)
{
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT; event.button.x = x; event.button.y = y;
    assert(SDL_PushEvent(&event)); event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    assert(SDL_PushEvent(&event)); sitehelper_app_process_events(app);
}
static void motion(SiteHelperApp *app, float x, float y)
{
    SDL_Event event = {.type = SDL_EVENT_MOUSE_MOTION}; event.motion.x = x; event.motion.y = y;
    assert(SDL_PushEvent(&event)); sitehelper_app_process_events(app);
}
static int saw_text, saw_cursor, saw_valid, saw_invalid;
static int measurement_live_text, measurement_complete_text, measurement_live_line, measurement_complete_line;
static RendererBackend original_backend;
static int record_snap, snap_lines;
static Colour snap_colour;
static Vec2 snap_a, snap_b;
static void record_line(void *context, Vec2 a, Vec2 b, Colour colour)
{
    if (fabs(a.x - 164) < 1e-9 && fabs(a.y - 500) < 1e-9) {
        if (fabs(b.x - 344) < 1e-9 && fabs(b.y - 260) < 1e-9) { measurement_live_line = 1; }
        if (fabs(b.x - 464) < 1e-9 && fabs(b.y - 100) < 1e-9) { measurement_complete_line = 1; }
    }
    if (record_snap) { snap_lines++; snap_colour = colour; snap_a = a; snap_b = b; }
    original_backend.draw_line(context, a, b, colour);
}
static void record_text(void *context, Vec2 position, const char *text, Colour colour)
{
    if (strcmp(text, "3000 mm") == 0) { measurement_live_text = 1; }
    if (strcmp(text, "5000 mm") == 0) { measurement_complete_text = 1; }
    if (strcmp(text, "4200") == 0) { saw_text = 1; }
    if (strchr(text, '^')) { saw_cursor = 1; }
    if (colour.g > colour.r) { saw_valid = 1; }
    if (colour.r > colour.g) { saw_invalid = 1; }
    original_backend.draw_screen_text(context, position, text, colour);
}
static void save_project_text(const SiteHelperApp *app, char *buffer, size_t capacity)
{
    const char *path = "sdl_measurement_project.tmp";
    assert(sitehelper_project_save_file(&app->project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(path, "rb"); assert(file);
    size_t count = fread(buffer, 1, capacity - 1, file);
    assert(count < capacity - 1 && !ferror(file)); buffer[count] = 0;
    assert(fclose(file) == 0 && remove(path) == 0);
}
static void test_measurement(SiteHelperApp *app)
{
    renderer2d_set_camera(app->renderer, (Camera2D){.position = {0,0}, .scale = .1});
    char before[8192], after[8192]; save_project_text(app, before, sizeof before);
    DomainId next_id = app->project.domain_ids.next;
    size_t count = app->history.count, cursor = app->history.cursor;
    PlanMeasurementQuery query;
    click(app, 32, 200); /* Appended button; existing tool positions unchanged. */
    assert(app->editor.active_tool == EDITOR_TOOL_MEASURE);
    GuiButton *button = NULL;
    for (size_t i = 0; i < app->toolbar.button_count; i++) {
        if (app->toolbar.buttons[i].id == SITEHELPER_TOOLBAR_ACTION_MEASURE) { button = &app->toolbar.buttons[i]; }
    }
    assert(button && button->enabled && button->active);
    assert(!sitehelper_editor_get_measurement(&app->editor, &query));
    assert(!SDL_TextInputActive(SDL_GetKeyboardFocus()));
    motion(app, 300,300); click(app, 164,500);
    assert(sitehelper_editor_get_measurement(&app->editor, &query));
    assert(!query.completed && query.start.x == 1000 && query.start.y == 1000);
    motion(app, 344,260);
    sitehelper_app_render(app);
    assert(measurement_live_text && measurement_live_line);
    click(app, 464,100); /* Fresh click point, not previous motion. */
    assert(sitehelper_editor_get_measurement(&app->editor, &query));
    assert(query.completed && query.distance_mm == 5000);
    sitehelper_app_render(app);
    assert(measurement_complete_text && measurement_complete_line);
    motion(app, 300,300);
    assert(sitehelper_editor_get_measurement(&app->editor, &query) && query.distance_mm == 5000);
    assert(app->history.count == count && app->history.cursor == cursor && app->project.domain_ids.next == next_id);
    save_project_text(app, after, sizeof after); assert(strcmp(before, after) == 0);
    send_key(app, SDLK_ESCAPE, 0);
    assert(!sitehelper_editor_get_measurement(&app->editor, &query));
    assert(app->editor.active_tool == EDITOR_TOOL_MEASURE);
    click(app, 164,500); send_key(app, SDLK_ESCAPE, 0);
    assert(!sitehelper_editor_get_measurement(&app->editor, &query));
    click(app, 164,500); click(app, 464,100);
    send_key(app, SDLK_Z, SDL_KMOD_CTRL);
    assert(app->history.cursor == cursor - 1 && !sitehelper_editor_get_measurement(&app->editor, &query));
    send_key(app, SDLK_Y, SDL_KMOD_CTRL);
    assert(app->history.cursor == cursor && !sitehelper_editor_get_measurement(&app->editor, &query));
    save_project_text(app, after, sizeof after); assert(strcmp(before, after) == 0);
    click(app, 164,500); click(app, 464,100);
    send_key(app, SDLK_TAB, 0);
    assert(app->editor.active_view == EDITOR_VIEW_WALL_ELEVATION && app->editor.active_tool == EDITOR_TOOL_SELECT);
    assert(!button->enabled && !button->active && !sitehelper_editor_get_measurement(&app->editor, &query));
    click(app, 32,200); assert(app->editor.active_tool == EDITOR_TOOL_SELECT);
    send_key(app, SDLK_TAB, 0);
    assert(button->enabled);
    click(app, 32,200); click(app, 164,500); click(app, 464,100);
    click(app, 32,144);
    assert(app->editor.active_tool == EDITOR_TOOL_WALL && !sitehelper_editor_get_measurement(&app->editor, &query));
    click(app, 32,200); click(app, 164,500); click(app, 464,100);
    motion(app, 550,300); /* Follow existing pointer-leave invalidation. */
    assert(!sitehelper_editor_get_measurement(&app->editor, &query));
    click(app, 164,500);
    assert(sitehelper_editor_get_measurement(&app->editor, &query) && !query.completed && query.distance_mm == 0);
    assert(app->history.count == count && app->history.cursor == cursor && app->project.domain_ids.next == next_id);
}

static void test_plan_snapping(SiteHelperApp *app)
{
    DomainId storey = sitehelper_project_add_storey(&app->project, 2700); assert(storey);
    assert(sitehelper_project_add_wall(&app->project, storey, (WallPlanSegment){{1000,1000},{3000,1000}}));
    assert(sitehelper_project_add_wall(&app->project, storey, (WallPlanSegment){{2000,0},{2000,3000}}));
    assert(sitehelper_editor_set_current_storey(&app->editor, &app->project, storey));
    renderer2d_set_camera(app->renderer, (Camera2D){.position = {0,0}, .scale = .1});
    click(app, 32,200);
    DomainId next_id = app->project.domain_ids.next;
    size_t count = app->history.count, cursor = app->history.cursor;
    char before[16384], after[16384]; save_project_text(app, before, sizeof before);
    motion(app, 364,499); /* Stale result at the other endpoint. */
    click(app, 163,499);
    PlanMeasurementQuery query;
    assert(sitehelper_editor_get_measurement(&app->editor, &query));
    assert(query.start.x == 1000 && query.start.y == 1000);
    assert(sitehelper_editor_get_snap_result(&app->editor)->type == SNAP_ENDPOINT);
    motion(app, 244,499);
    assert(sitehelper_editor_get_measurement(&app->editor, &query));
    assert(query.end.x == 1800 && query.end.y == 1000 && query.distance_mm == 800);
    assert(sitehelper_editor_get_snap_result(&app->editor)->type == SNAP_WALL_CENTRELINE);
    record_snap = 1; snap_lines = 0;
    sitehelper_app_render_snap_cursor(app);
    record_snap = 0;
    assert(snap_lines == 2 && snap_colour.r == 220 && snap_colour.g == 120 && snap_colour.b == 255);
    assert(fabs(snap_a.x - 244) < 1e-9 && fabs(snap_b.x - 244) < 1e-9);
    assert(fabs(fabs(snap_a.y - snap_b.y) - 8) < 1e-9);
    click(app, 264,500); /* Actual B is the shared wall junction. */
    assert(sitehelper_editor_get_measurement(&app->editor, &query));
    assert(query.completed && query.end.x == 2000 && query.end.y == 1000 && query.distance_mm == 1000);
    save_project_text(app, after, sizeof after);
    assert(strcmp(before, after) == 0 && app->project.domain_ids.next == next_id);
    assert(app->history.count == count && app->history.cursor == cursor);

    click(app, 32,144);
    click(app, 244,499); /* Centreline start. */
    click(app, 263,400); /* Centreline end, no motion before either click. */
    assert(app->history.count == count + 1 && app->history.cursor == cursor + 1);
    const Storey *active = sitehelper_project_find_storey_by_id_const(&app->project, storey);
    assert(active && active->structure.wall_count == 3);
    WallPlanSegment segment = active->structure.walls[2].definition.segment;
    assert(segment.start.x == 1800 && segment.start.y == 1000 && segment.end.x == 2000 && segment.end.y == 2000);
    assert(app->history.entries[count].command.type == SITEHELPER_COMMAND_ADD_WALL);
    send_key(app, SDLK_Z, SDL_KMOD_CTRL);
    assert(sitehelper_project_find_storey_by_id_const(&app->project, storey)->structure.wall_count == 2);
    send_key(app, SDLK_Y, SDL_KMOD_CTRL);
    assert(sitehelper_project_find_storey_by_id_const(&app->project, storey)->structure.wall_count == 3);
}

int main(void)
{
    assert(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    SiteHelperApp app;
    assert(sitehelper_app_init(&app));
    assert(app.backend.context != NULL);
    EditorTool slab_feature_tool;
    assert(app.toolbar.button_count==15&&
        sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_SLAB_PENETRATION,
            &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_SLAB_PENETRATION);
    const GuiButton *last_toolbar_button = &app.toolbar.buttons[app.toolbar.button_count - 1];
    assert(last_toolbar_button->bounds.position.y + last_toolbar_button->bounds.height <=
        app.toolbar.bounds.position.y + app.toolbar.bounds.height + 0.001);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_SLAB_REGION,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_SLAB_REGION);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_SLAB_EDGE_REBATE,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_SLAB_EDGE_REBATE);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_SLAB_GEOMETRY,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_SLAB_GEOMETRY);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_NOTE,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_NOTE);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_DIMENSION,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_DIMENSION);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_SYMBOL,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_SYMBOL);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_CALLOUT,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_CALLOUT);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_VIEW_DIRECTION,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_VIEW_DIRECTION);
    assert(sitehelper_app_toolbar_action_tool(SITEHELPER_TOOLBAR_ACTION_REVISION_CLOUD,
        &slab_feature_tool)&&slab_feature_tool==EDITOR_TOOL_REVISION_CLOUD);
    sitehelper_app_process_events(&app);
    original_backend = app.backend;
    RendererBackend recording = app.backend; recording.draw_screen_text = record_text; recording.draw_line = record_line;
    renderer2d_set_backend(app.renderer, recording);

    click(&app, 32, 144); /* Wall toolbar. */
    assert(app.editor.active_tool == EDITOR_TOOL_WALL);
    click(&app, 200, 250);
    assert(sitehelper_editor_has_wall_preview(&app.editor) && !app.text_input_failed);
    assert(SDL_TextInputActive(SDL_GetKeyboardFocus()));
    motion(&app, 400, 200);
    send_text(&app, "4"); send_text(&app, "2"); send_text(&app, "0"); send_text(&app, "0");
    assert(app.input.focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH && app_input_valid(&app.input));
    assert(app.project.storeys[0].structure.wall_count == 0);
    sitehelper_app_render(&app);
    assert(saw_text && saw_cursor && saw_valid);
    Camera2D camera = renderer2d_get_camera(app.renderer);
    send_key(&app, SDLK_LEFT, 0);
    send_key(&app, SDLK_RIGHT, 0);
    send_key(&app, SDLK_TAB, 0);
    assert(app.editor.active_view == EDITOR_VIEW_PLAN);
    assert(renderer2d_get_camera(app.renderer).position.x == camera.position.x);
    click(&app, 450, 220); /* Mouse cannot commit while text owns focus. */
    assert(app.project.storeys[0].structure.wall_count == 0);
    /* Command failure retains the session and does not consume IDs/history. */
    int stud_height = app.project.settings.stud_height;
    DomainId next_id = app.project.domain_ids.next;
    app.project.settings.stud_height = 0;
    send_key(&app, SDLK_RETURN, 0);
    assert(app.input.command_failed && app.input.focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH);
    assert(strcmp(app.input.text.text, "4200") == 0 && app.history.count == 0);
    assert(app.project.domain_ids.next == next_id && app.project.storeys[0].structure.wall_count == 0);
    app.project.settings.stud_height = stud_height;
    send_key(&app, SDLK_RETURN, 0);
    assert(app.project.storeys[0].structure.wall_count == 1 && app.history.count == 1);
    assert(app.history.entries[0].command.type == SITEHELPER_COMMAND_ADD_WALL);
    WallPlanSegment segment = app.project.storeys[0].structure.walls[0].definition.segment;
    assert(wall_plan_segment_length_mm(segment) == 4200);
    assert(app.input.focus == APP_KEYBOARD_FOCUS_NONE);
    assert(!SDL_TextInputActive(SDL_GetKeyboardFocus()));
    send_key(&app, SDLK_Z, SDL_KMOD_CTRL);
    assert(app.project.storeys[0].structure.wall_count == 0);
    send_key(&app, SDLK_Y, SDL_KMOD_CTRL);
    assert(app.project.storeys[0].structure.wall_count == 1);
    send_key(&app, SDLK_Z, SDL_KMOD_CTRL);
    send_key(&app, SDLK_Z, SDL_KMOD_CTRL | SDL_KMOD_SHIFT);
    assert(app.project.storeys[0].structure.wall_count == 1);
    assert(wall_plan_segment_length_mm(app.project.storeys[0].structure.walls[0].definition.segment) == 4200);

    click(&app, 200, 350); motion(&app, 320, 300); send_text(&app, "4.2mx");
    sitehelper_app_render(&app); assert(saw_invalid);
    send_key(&app, SDLK_RETURN, 0);
    assert(app.project.storeys[0].structure.wall_count == 1);
    send_key(&app, SDLK_Z, SDL_KMOD_CTRL);
    send_key(&app, SDLK_Y, SDL_KMOD_CTRL);
    send_key(&app, SDLK_Z, SDL_KMOD_CTRL | SDL_KMOD_SHIFT);
    assert(app.history.cursor == 1 && app.project.storeys[0].structure.wall_count == 1);
    send_key(&app, SDLK_ESCAPE, 0);
    assert(app.input.focus == APP_KEYBOARD_FOCUS_NONE && sitehelper_editor_has_wall_preview(&app.editor));
    send_key(&app, SDLK_ESCAPE, 0);
    assert(!sitehelper_editor_has_wall_preview(&app.editor) && app.editor.active_tool == EDITOR_TOOL_WALL);
    send_key(&app, SDLK_LEFT, 0);
    assert(renderer2d_get_camera(app.renderer).position.x == camera.position.x - 100);
    send_key(&app, SDLK_RIGHT, 0);
    assert(renderer2d_get_camera(app.renderer).position.x == camera.position.x);

    click(&app, 200, 350); motion(&app, 320, 300);
    WallPlanSegment preview;
    assert(sitehelper_editor_get_wall_preview_segment(&app.editor, &preview));
    click(&app, 320, 300);
    assert(app.project.storeys[0].structure.wall_count == 2 && app.history.count == 2);
    WallPlanSegment mouse = app.project.storeys[0].structure.walls[1].definition.segment;
    assert(mouse.start.x == preview.start.x && mouse.start.y == preview.start.y &&
        mouse.end.x == preview.end.x && mouse.end.y == preview.end.y);
    click(&app, 200, 350); motion(&app, 320, 300); send_text(&app, "4200");
    click(&app, 32, 32); /* Toolbar selection cancels session via editor lifecycle. */
    assert(app.editor.active_tool == EDITOR_TOOL_SELECT && app.input.focus == APP_KEYBOARD_FOCUS_NONE);
    assert(!SDL_TextInputActive(SDL_GetKeyboardFocus()));
    send_key(&app, SDLK_TAB, 0);
    assert(app.editor.active_view == EDITOR_VIEW_WALL_ELEVATION);
    click(&app, 32, 88);
    assert(app.editor.active_tool == EDITOR_TOOL_OPENING);
    send_key(&app, SDLK_TAB, 0);
    assert(app.editor.active_view == EDITOR_VIEW_PLAN && app.editor.active_tool == EDITOR_TOOL_SELECT);

    double scale = renderer2d_get_camera(app.renderer).scale;
    SDL_Event wheel = {.type = SDL_EVENT_MOUSE_WHEEL};
    wheel.wheel.y = 1; wheel.wheel.mouse_x = 300; wheel.wheel.mouse_y = 300;
    assert(SDL_PushEvent(&wheel)); sitehelper_app_process_events(&app);
    assert(fabs(renderer2d_get_camera(app.renderer).scale - scale * 1.1) < 1e-12);
    SDL_Event middle = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    middle.button.button = SDL_BUTTON_MIDDLE; middle.button.x = 300; middle.button.y = 300;
    assert(SDL_PushEvent(&middle)); sitehelper_app_process_events(&app);
    camera = renderer2d_get_camera(app.renderer);
    SDL_Event drag = {.type = SDL_EVENT_MOUSE_MOTION};
    drag.motion.x = 310; drag.motion.y = 305; drag.motion.xrel = 10; drag.motion.yrel = 5;
    drag.motion.state = SDL_BUTTON_MMASK;
    assert(SDL_PushEvent(&drag)); sitehelper_app_process_events(&app);
    assert(fabs(renderer2d_get_camera(app.renderer).position.x - (camera.position.x - 10/camera.scale)) < 1e-9);
    middle.type = SDL_EVENT_MOUSE_BUTTON_UP;
    assert(SDL_PushEvent(&middle)); sitehelper_app_process_events(&app);
    assert(!viewport_input_allows_pan(&app.viewport_input));
    test_measurement(&app);
    test_plan_snapping(&app);
    sitehelper_app_destroy(&app);
    puts("SDL application input tests passed");
}
