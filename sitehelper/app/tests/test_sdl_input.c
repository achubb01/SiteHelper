#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL3/SDL.h>

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
static RendererBackend original_backend;
static void record_text(void *context, Vec2 position, const char *text, Colour colour)
{
    if (strcmp(text, "4200") == 0) { saw_text = 1; }
    if (strchr(text, '^')) { saw_cursor = 1; }
    if (colour.g > colour.r) { saw_valid = 1; }
    if (colour.r > colour.g) { saw_invalid = 1; }
    original_backend.draw_screen_text(context, position, text, colour);
}
int main(void)
{
    assert(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    SiteHelperApp app;
    assert(sitehelper_app_init(&app));
    assert(app.backend.context != NULL);
    sitehelper_app_process_events(&app);
    original_backend = app.backend;
    RendererBackend recording = app.backend; recording.draw_screen_text = record_text;
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
    sitehelper_app_destroy(&app);
    puts("SDL application input tests passed");
}
