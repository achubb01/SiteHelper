#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "sitehelper_editor.h"
#include "app_input.h"
#include "app_view.h"
#include "command_history.h"
#include "sitehelper_persistence.h"

static PlanMeasurementQuery get(const SiteHelperEditor *editor)
{
    PlanMeasurementQuery query;
    assert(sitehelper_editor_get_measurement(editor, &query));
    return query;
}
static void click(SiteHelperEditor *editor, const SiteHelperProject *project, Vec2 point)
{
    EditorAction action = {.kind = EDITOR_ACTION_COMMAND, .command.type = SITEHELPER_COMMAND_ADD_WALL};
    assert(sitehelper_editor_primary_action_in_project(editor, project, point, &action));
    assert(action.kind == EDITOR_ACTION_NONE && action.command.type == SITEHELPER_COMMAND_NONE);
}
static void save(const SiteHelperProject *project, char *text, size_t capacity)
{
    const char *path = "measurement_project.tmp";
    assert(sitehelper_project_save_file(project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(path, "rb"); assert(file);
    size_t length = fread(text, 1, capacity - 1, file);
    assert(length < capacity - 1 && !ferror(file)); text[length] = 0;
    assert(fclose(file) == 0 && remove(path) == 0);
}
static void test_editor_lifecycle_and_ownership(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project); sitehelper_editor_init(&editor); sitehelper_command_history_init(&history);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId other = sitehelper_project_add_storey(&project, 2700);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey));
    WallCommand wall;
    SiteHelperCommand command;
    SiteHelperCommandResult result;
    for (int i = 0; i < 2; i++) {
        assert(wall_command_create(storey, (WallPlanSegment){{0,i*1000},{4200,i*1000}}, &wall));
        assert(sitehelper_command_from_wall(&wall, &command));
        assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    }
    assert(sitehelper_command_history_undo(&history, &project));
    const DomainId next_id = project.domain_ids.next;
    const Wall *walls = project.storeys[0].structure.walls;
    const Timber *studs = walls[0].framing.studs;
    char before[8192], after[8192]; save(&project, before, sizeof before);
    PlanMeasurementQuery query;
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    assert(sitehelper_editor_tool_available(EDITOR_VIEW_PLAN, EDITOR_TOOL_MEASURE));
    assert(!sitehelper_editor_tool_available(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_MEASURE));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){999,999});
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    click(&editor, &project, (Vec2){-12,22}); /* Replaces stale snap at (1000,1000). */
    query = get(&editor);
    assert(query.start.x == 0 && query.start.y == 0 && !query.completed);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){312,422});
    query = get(&editor);
    assert(query.end.x == 300 && query.end.y == 400 && query.distance_mm == 500 && !query.completed);
    click(&editor, &project, (Vec2){612,822}); /* Click B differs from preceding motion. */
    query = get(&editor);
    assert(query.completed && query.end.x == 600 && query.end.y == 800 && query.distance_mm == 1000);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){900,900});
    assert(get(&editor).distance_mm == 1000);
    save(&project, after, sizeof after);
    assert(strcmp(before, after) == 0 && project.domain_ids.next == next_id);
    assert(project.storeys[0].structure.walls == walls && walls[0].framing.studs == studs);
    assert(project.storeys[0].structure.wall_count == 1 && history.count == 2 && history.cursor == 1);

    EditorAction action;
    assert(!sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){NAN,1}, &action));
    assert(action.kind == EDITOR_ACTION_NONE && get(&editor).distance_mm == 1000);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){1,INFINITY});
    assert(get(&editor).distance_mm == 1000);
    AppInput input = {0};
    PlatformEvent escape = {.type = PLATFORM_EVENT_KEY_DOWN, .data.key_down.key = PLATFORM_KEY_ESCAPE};
    assert(app_input_route(&input, &editor, &escape, &action) == APP_INPUT_CONSUMED);
    assert(!sitehelper_editor_get_measurement(&editor, &query) && editor.active_tool == EDITOR_TOOL_MEASURE);
    assert(!app_input_wants_text(&input, &editor));
    /* Exercise raw fractional coordinates independently of object snapping. */
    editor.snap.settings.grid_enabled = 0;
    editor.snap.settings.endpoint_enabled = 0;
    editor.snap.settings.wall_centreline_enabled = 0;
    editor.snap.settings.intersection_enabled = 0;
    click(&editor, &project, (Vec2){-.25,-.5});
    click(&editor, &project, (Vec2){2.75,3.5});
    query = get(&editor);
    assert(query.start.x == -.25 && query.end.x == 2.75 && query.distance_mm == 5);
    sitehelper_editor_pointer_leave(&editor);
    assert(!sitehelper_editor_get_measurement(&editor, &query) && !sitehelper_editor_has_snap(&editor));
    click(&editor, &project, (Vec2){1,2});
    assert(!get(&editor).completed); /* Pointer leave cleared A as well as B. */
    sitehelper_editor_invalidate_transient_state(&editor);
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    click(&editor, &project, (Vec2){1,2});
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    click(&editor, &project, (Vec2){1,2}); click(&editor, &project, (Vec2){4,6});
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(editor.active_tool == EDITOR_TOOL_SELECT && !sitehelper_editor_get_measurement(&editor, &query));
    assert(!sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_PLAN));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    click(&editor, &project, (Vec2){1,2});
    assert(sitehelper_editor_set_current_storey(&editor, &project, other));
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    click(&editor, &project, (Vec2){1,2});
    assert(sitehelper_command_history_redo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    assert(project.storeys[0].structure.wall_count == 2);
    click(&editor, &project, (Vec2){1,2}); click(&editor, &project, (Vec2){4,6});
    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(!sitehelper_editor_get_measurement(&editor, &query));
    save(&project, after, sizeof after); assert(strcmp(before, after) == 0);
    assert(!sitehelper_editor_get_measurement(NULL, &query));
    assert(!sitehelper_editor_get_measurement(&editor, NULL));
    sitehelper_command_history_destroy(&history); sitehelper_project_destroy(&project);
}

typedef struct { int lines, texts; Vec2 a,b,label; char text[400]; } Drawing;
static void line(void *context, Vec2 a, Vec2 b, Colour colour)
{
    (void)colour; Drawing *d = context; d->lines++; d->a = a; d->b = b;
}
static void text(void *context, Vec2 position, const char *value, Colour colour)
{
    (void)colour; Drawing *d = context; d->texts++; d->label = position;
    snprintf(d->text, sizeof d->text, "%s", value);
}
static void test_presentation(void)
{
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    editor.snap.settings.grid_enabled = 0;
    Renderer2D *renderer = renderer2d_create(); assert(renderer);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){.context = &drawing, .draw_line = line, .draw_screen_text = text});
    Camera2D camera = {.position = {-10,-20}, .scale = 2};
    renderer2d_set_camera(renderer, camera); renderer2d_set_viewport(renderer, (Vec2){64,10}, 600, 500);
    app_render_measurement(renderer, &editor); assert(!drawing.lines && !drawing.texts);
    EditorAction action;
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){0,0}, &action));
    sitehelper_editor_pointer_move(&editor, NULL, NULL, (Vec2){3,4});
    app_render_measurement(renderer, &editor);
    assert(drawing.lines == 1 && drawing.texts == 1 && strcmp(drawing.text, "5 mm") == 0);
    Vec2 expected = camera_world_to_screen(&camera, renderer2d_get_viewport(renderer), (Vec2){1.5,2});
    assert(drawing.label.x == expected.x && drawing.label.y == expected.y - 12);
    expected = camera_world_to_screen(&camera, renderer2d_get_viewport(renderer), (Vec2){3,4});
    assert(drawing.b.x == expected.x && drawing.b.y == expected.y);
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){0,2.5}, &action));
    app_render_measurement(renderer, &editor); assert(strcmp(drawing.text, "3 mm") == 0);
    assert(get(&editor).distance_mm == 2.5); /* Display rounding never changes query. */
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){0,0}, &action));
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){0,0}, &action));
    app_render_measurement(renderer, &editor); assert(strcmp(drawing.text, "0 mm") == 0);
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){INT_MIN,0}, &action));
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){INT_MAX,0}, &action));
    app_render_measurement(renderer, &editor); assert(strcmp(drawing.text, "4294967295 mm") == 0);
    renderer2d_destroy(renderer);
}
int main(void)
{
    test_editor_lifecycle_and_ownership(); test_presentation();
    puts("editor measurement tests passed");
}
