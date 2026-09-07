#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_view.h"
#include "appstate.h"
#include "sitehelper_persistence.h"
#include "command_history.h"

typedef struct {
    size_t line_count, rect_count;
    Rect2 rects[256];
} Drawing;

static void record_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)start; (void)end; (void)colour;
    ((Drawing *)context)->line_count++;
}

static void record_rect(void *context, Rect2 rect, Colour colour)
{
    (void)colour;
    Drawing *drawing = context;
    assert(drawing->rect_count < 256);
    drawing->rects[drawing->rect_count++] = rect;
}

static void assert_same_rects(const Drawing *a, const Drawing *b)
{
    assert(a->rect_count == b->rect_count);
    for (size_t i = 0; i < a->rect_count; i++) {
        assert(a->rects[i].position.x == b->rects[i].position.x);
        assert(a->rects[i].position.y == b->rects[i].position.y);
        assert(a->rects[i].width == b->rects[i].width);
        assert(a->rects[i].height == b->rects[i].height);
    }
}

static void test_view_rendering_and_local_pointer(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    DomainId room_id = sitehelper_project_add_room(&project);
    DomainId first = sitehelper_project_add_wall(&project, room_id,
        (WallPlanSegment){{5000, 3000}, {9200, 3000}});
    DomainId second = sitehelper_project_add_wall(&project, room_id,
        (WallPlanSegment){{-8000, 9000}, {-5480, 12360}});
    assert(first != DOMAIN_ID_INVALID && second != DOMAIN_ID_INVALID);
    assert(wall_generate(build_find_wall_by_id(&project.structure, first), &project.settings));
    assert(wall_generate(build_find_wall_by_id(&project.structure, second), &project.settings));
    editor.current_room_id = room_id;
    editor.current_wall_id = first;
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing, .draw_line = record_line, .fill_rect = record_rect
    });
    Camera2D camera = {.position = {-200, -300}, .scale = 0.25};
    renderer2d_set_camera(renderer, camera);
    renderer2d_set_viewport(renderer, (Vec2){40, 20}, 800, 600);
    WallRenderStyle style = {.timber_colour = {100, 100, 100, 255}};
    app_render_walls(renderer, &project, &editor, &style);
    assert(drawing.line_count == 2 && drawing.rect_count == 0);

    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style);
    assert(drawing.line_count == 0 && drawing.rect_count > 2);
    Drawing first_elevation = drawing;
    drawing = (Drawing){0};
    wall_elevation_render(renderer, app_current_wall_const(&project, &editor), NULL, &style);
    assert_same_rects(&first_elevation, &drawing); /* Exactly the current wall. */
    editor.current_wall_id = second;
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style);
    assert_same_rects(&first_elevation, &drawing);

    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 local = {10.75, 10.25};
    Vec2 screen = camera_world_to_screen(&camera, viewport, local);
    Vec2 pointer = camera_screen_to_world(&camera, viewport, screen);
    EditorAction action;
    const Room *room = app_current_room_const(&project, &editor);
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, pointer, &action));
    assert(editor.selection.wall_id == second);
    assert(editor.selection.wall_member.kind == WALL_MEMBER_BOTTOM_PLATE);
    /* A click on the other wall's physical plan origin is not elevation input. */
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){5010, 3010}, &action));
    assert(editor_selection_is_empty(&editor.selection));
    assert(editor.current_wall_id == second);

    const Wall *wall = app_current_wall_const(&project, &editor);
    sitehelper_editor_pointer_move(&editor, wall, &project.settings, (Vec2){0, 0});
    const SnapResult *snap = sitehelper_editor_get_snap_result(&editor);
    assert(snap->type == SNAP_ENDPOINT);
    assert(snap->position.x == 0 && snap->position.y == 0);
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_OPENING));
    sitehelper_editor_pointer_move(&editor, wall, &project.settings, (Vec2){1600, 800});
    Rect2 preview;
    assert(sitehelper_editor_get_opening_preview_rect(&editor, &preview));
    assert(preview.position.x == 1600 && preview.position.y == 900);
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){1600, 800}, &action));
    assert(action.kind == EDITOR_ACTION_COMMAND);
    assert(action.command.data.opening.wall_id == second);
    assert(action.command.data.opening.frame_position == 1600);
    assert(action.command.data.opening.frame_bottom == 900);
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project, &action.command, &result));
    wall = app_current_wall_const(&project, &editor);
    assert(wall->definition.opening_count == 1);
    assert(wall->definition.openings[0].frame_position == 1600);
    assert(wall->definition.openings[0].frame_bottom == 900);
    assert(build_find_wall_by_id_const(&project.structure, first)->definition.opening_count == 0);

    editor.current_wall_id = DOMAIN_ID_INVALID;
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style);
    assert(drawing.rect_count == 0 && drawing.line_count == 0);
    renderer2d_destroy(renderer);
    sitehelper_project_destroy(&project);
}

static void test_switching_views_clears_tools_and_retains_cameras(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
    assert(!sitehelper_editor_tool_available(EDITOR_VIEW_PLAN, EDITOR_TOOL_OPENING));
    assert(!sitehelper_editor_tool_available(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_WALL));
    assert(!sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_OPENING));
    assert(!sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_COUNT));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    EditorAction action;
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){1000, 2000}, &action));
    sitehelper_editor_pointer_move(&editor, NULL, NULL, (Vec2){5000, 5000});
    assert(sitehelper_editor_has_wall_preview(&editor));
    assert(sitehelper_editor_has_snap(&editor));
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Camera2D initial = {.position = {-200, -200}, .scale = 0.12};
    AppViews views;
    app_views_init(&views, initial);
    Camera2D plan = {.position = {1000, 2000}, .scale = 0.5};
    renderer2d_set_camera(renderer, plan);
    assert(app_views_set_active(&views, &editor, renderer, EDITOR_VIEW_WALL_ELEVATION));
    assert(editor.active_tool == EDITOR_TOOL_SELECT);
    assert(!editor.wall_tool.active && !editor.wall_tool.has_start);
    assert(!sitehelper_editor_has_snap(&editor));
    assert(renderer2d_get_camera(renderer).scale == initial.scale);
    assert(!sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));

    Camera2D elevation = {.position = {-500, 700}, .scale = 1.5};
    renderer2d_set_camera(renderer, elevation);
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_OPENING));
    Wall wall = {.definition.segment = {{5000, 3000}, {9200, 3000}}};
    SiteHelperProject project;
    sitehelper_project_init(&project);
    sitehelper_editor_pointer_move(&editor, &wall, &project.settings, (Vec2){1600, 800});
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(editor.opening_tool.preview_valid);
    assert(app_views_set_active(&views, &editor, renderer, EDITOR_VIEW_PLAN));
    assert(editor.active_tool == EDITOR_TOOL_SELECT);
    assert(!editor.opening_tool.active && !editor.opening_tool.preview_valid);
    assert(!sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_has_snap(&editor));
    Camera2D restored = renderer2d_get_camera(renderer);
    assert(restored.position.x == plan.position.x && restored.position.y == plan.position.y);
    assert(restored.scale == plan.scale);
    /* Plan snapping ignores generated framing, even when a current wall is passed. */
    Timber stud = {.position = {620, 0}, .length = 2400, .width = 35};
    wall.framing.studs = &stud;
    wall.framing.stud_count = 1;
    sitehelper_editor_pointer_move(&editor, &wall, &project.settings, (Vec2){619, 10});
    assert(editor.snap.result.type == SNAP_GRID);
    assert(editor.snap.result.position.x == 600 && editor.snap.result.position.y == 0);
    assert(app_views_set_active(&views, &editor, renderer, EDITOR_VIEW_WALL_ELEVATION));
    assert(!sitehelper_editor_has_snap(&editor)); /* Select-to-Select also invalidates. */
    restored = renderer2d_get_camera(renderer);
    assert(restored.position.x == elevation.position.x && restored.position.y == elevation.position.y);
    assert(restored.scale == elevation.scale);
    renderer2d_destroy(renderer);
    sitehelper_project_destroy(&project);
}

static void test_plan_focus_uses_nearest_physical_segment(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    editor.current_room_id = sitehelper_project_add_room(&project);
    DomainId diagonal = sitehelper_project_add_wall(&project, editor.current_room_id,
        (WallPlanSegment){{5000, 5000}, {1000, 2000}});
    DomainId horizontal = sitehelper_project_add_wall(&project, editor.current_room_id,
        (WallPlanSegment){{0, 3500}, {6000, 3500}});
    DomainId other_room = sitehelper_project_add_room(&project);
    assert(sitehelper_project_add_wall(&project, other_room,
        (WallPlanSegment){{0, 3500}, {6000, 3500}}) != DOMAIN_ID_INVALID);
    const Room *room = app_current_room_const(&project, &editor);
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){3080, 3560}, &action));
    assert(editor.current_wall_id == diagonal); /* Both nearby, diagonal is closer. */
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){3000, 3500}, &action));
    assert(editor.current_wall_id == horizontal); /* Exact tie: later wall wins. */
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){1000, 2000}, &action));
    assert(editor.current_wall_id == diagonal);
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){900, 1925}, &action));
    assert(editor.current_wall_id == DOMAIN_ID_INVALID); /* Beyond the segment end. */
    assert(sitehelper_editor_primary_action_in_room(&editor, &project.structure,
        room, (Vec2){5010, 6000}, &action));
    assert(editor.current_wall_id == DOMAIN_ID_INVALID); /* Old artificial stud area. */
    assert(editor_selection_is_empty(&editor.selection));
    sitehelper_project_destroy(&project);
}

static void read_saved_project(const SiteHelperProject *project, const char *path,
    char *buffer, size_t capacity)
{
    assert(sitehelper_project_save_file(project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(path, "rb");
    assert(file != NULL);
    size_t count = fread(buffer, 1, capacity - 1, file);
    assert(count < capacity - 1 && !ferror(file));
    buffer[count] = '\0';
    assert(fclose(file) == 0);
    assert(remove(path) == 0);
}

static void test_views_and_history_preserve_persistent_project(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    sitehelper_command_history_init(&history);
    editor.current_room_id = sitehelper_project_add_room(&project);
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    EditorAction action;
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){5000, 5000}, &action));
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){1000, 2000}, &action));
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history, &project, &action.command, &result));
    sitehelper_editor_complete_action(&editor, &action, &result);
    char before[4096], after[4096];
    read_saved_project(&project, "app_view_before.tmp", before, sizeof before);
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    AppViews views;
    app_views_init(&views, (Camera2D){.scale = 0.25});
    renderer2d_set_camera(renderer, (Camera2D){.position = {800, 900}, .scale = 2});
    assert(app_views_set_active(&views, &editor, renderer, EDITOR_VIEW_WALL_ELEVATION));
    renderer2d_move_camera(renderer, (Vec2){123, 456});
    renderer2d_zoom_camera(renderer, 2);
    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == DOMAIN_ID_INVALID);
    assert(sitehelper_command_history_redo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    const Wall *wall = build_find_wall_by_id_const(&project.structure, result.data.add_wall.wall_id);
    assert(wall != NULL);
    assert(wall->definition.segment.start.x == 5000 && wall->definition.segment.start.y == 5000);
    assert(wall->definition.segment.end.x == 1000 && wall->definition.segment.end.y == 2000);
    read_saved_project(&project, "app_view_after.tmp", after, sizeof after);
    assert(strcmp(before, after) == 0);
    assert(strstr(after, "view") == NULL && strstr(after, "camera") == NULL);
    assert(strstr(after, "origin") == NULL && strstr(after, "drawing") == NULL);
    renderer2d_destroy(renderer);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_view_rendering_and_local_pointer();
    test_switching_views_clears_tools_and_retains_cameras();
    test_plan_focus_uses_nearest_physical_segment();
    test_views_and_history_preserve_persistent_project();
    puts("All app view tests passed.");
    return 0;
}
