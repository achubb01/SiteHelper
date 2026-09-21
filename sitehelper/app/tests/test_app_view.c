#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_view.h"
#include "appstate.h"
#include "sitehelper_persistence.h"
#include "command_history.h"

typedef struct {
    size_t line_count, rect_count;
    size_t selected_rect_count;
    Rect2 rects[256];
} Drawing;

static void record_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)start; (void)end; (void)colour;
    ((Drawing *)context)->line_count++;
}

static void record_rect(void *context, Rect2 rect, Colour colour)
{
    Drawing *drawing = context;
    assert(drawing->rect_count < 256);
    drawing->rects[drawing->rect_count++] = rect;
    if (colour.r == 255 && colour.g == 0 && colour.b == 0) { drawing->selected_rect_count++; }
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
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId first = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{5000, 3000}, {9200, 3000}});
    DomainId second = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{-8000, 9000}, {-5480, 12360}});
    assert(first != DOMAIN_ID_INVALID && second != DOMAIN_ID_INVALID);
    assert(wall_generate(build_find_wall_by_id(&project.storeys[0].structure, first), &project.settings));
    assert(wall_generate(build_find_wall_by_id(&project.storeys[0].structure, second), &project.settings));
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
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 2 && drawing.rect_count == 0);

    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 0 && drawing.rect_count > 2);
    Drawing first_elevation = drawing;
    drawing = (Drawing){0};
    wall_elevation_render(renderer, app_current_wall_const(&project, &editor), NULL, &style);
    assert_same_rects(&first_elevation, &drawing); /* Exactly the current wall. */
    SiteHelperCommand rotation = {.type = SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
        .data.move_wall_endpoint = {first, WALL_ENDPOINT_END, {5000, 7200}}};
    SiteHelperCommandResult rotation_result;
    assert(sitehelper_command_execute(&project, &rotation, &rotation_result));
    sitehelper_editor_reconcile(&editor, &project);
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert_same_rects(&first_elevation, &drawing);
    Camera2D after_rotation = renderer2d_get_camera(renderer);
    assert(after_rotation.position.x == camera.position.x);
    assert(after_rotation.position.y == camera.position.y);
    assert(after_rotation.scale == camera.scale);
    assert(editor.current_wall_id == first);

    editor.current_wall_id = second;
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert_same_rects(&first_elevation, &drawing);

    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 local = {10.75, 10.25};
    Vec2 screen = camera_world_to_screen(&camera, viewport, local);
    Vec2 pointer = camera_screen_to_world(&camera, viewport, screen);
    EditorAction action;
    assert(app_current_room_const(&project, &editor)->id == room_id);
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, pointer, &action));
    assert(editor.selection.wall_id == second);
    assert(editor.selection.wall_member.kind == WALL_MEMBER_BOTTOM_PLATE);
    assert(editor.selection.scope == EDITOR_SELECTION_SCOPE_WALL_ELEVATION);
    /* A click on the other wall's physical plan origin is not elevation input. */
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){5010, 3010}, &action));
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
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){1600, 800}, &action));
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
    assert(build_find_wall_by_id_const(&project.storeys[0].structure, first)->definition.opening_count == 0);

    editor.current_wall_id = DOMAIN_ID_INVALID;
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.rect_count == 0 && drawing.line_count == 0);
    renderer2d_destroy(renderer);
    sitehelper_project_destroy(&project);
}

static void test_switching_views_clears_tools_and_retains_cameras(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    assert(editor.active_view == EDITOR_VIEW_PLAN);
    assert(!editor_view_supports_tool(EDITOR_VIEW_PLAN, EDITOR_TOOL_OPENING));
    assert(!editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_WALL));
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
    assert(sitehelper_project_add_storey(&project, 0));
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
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId diagonal = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{5000, 5000}, {1000, 2000}});
    DomainId horizontal = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{0, 3500}, {6000, 3500}});
    DomainId other_room = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(horizontal != DOMAIN_ID_INVALID && other_room != DOMAIN_ID_INVALID);
    DomainId later_wall = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{0, 3500}, {6000, 3500}});
    assert(later_wall != DOMAIN_ID_INVALID);
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){3080, 3560}, &action));
    assert(editor.current_wall_id == diagonal); /* Both nearby, diagonal is closer. */
    editor.current_room_id = other_room;
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_room_id == other_room && editor.current_wall_id == diagonal);
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){3000, 3500}, &action));
    assert(editor.current_wall_id == later_wall); /* Global insertion order breaks ties. */
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){1000, 2000}, &action));
    assert(editor.current_wall_id == diagonal);
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){900, 1925}, &action));
    assert(editor.current_wall_id == DOMAIN_ID_INVALID); /* Beyond the segment end. */
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){5010, 6000}, &action));
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
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    sitehelper_command_history_init(&history);
    editor.current_room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
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
    const Wall *wall = build_find_wall_by_id_const(&project.storeys[0].structure, result.data.add_wall.wall_id);
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

static void test_plan_commands_and_navigation_without_rooms(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    sitehelper_command_history_init(&history);
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(&editor, &project,
        (Vec2){1000, 2000}, &action));
    assert(action.kind == EDITOR_ACTION_NONE);
    assert(sitehelper_editor_primary_action_in_project(&editor, &project,
        (Vec2){5000, 2000}, &action));
    assert(action.kind == EDITOR_ACTION_COMMAND);
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history, &project, &action.command, &result));
    DomainId first = result.data.add_wall.wall_id;
    sitehelper_editor_complete_action(&editor, &action, &result);
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == first);
    assert(editor.current_room_id == DOMAIN_ID_INVALID);
    assert(project.storeys[0].structure.room_count == 0);
    assert(app_current_wall(&project, &editor)->id == first);
    assert(app_current_wall_const(&project, &editor)->id == first);

    WallCommand wall_command;
    SiteHelperCommand command;
    assert(wall_command_create(1, (WallPlanSegment){{-5000, -1000}, {-1000, -1000}}, &wall_command));
    assert(sitehelper_command_from_wall(&wall_command, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    DomainId second = result.data.add_wall.wall_id;
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_SELECT));
    assert(sitehelper_editor_primary_action_in_project(&editor, &project,
        (Vec2){-3000, -1000}, &action));
    assert(editor.current_wall_id == second);
    assert(sitehelper_editor_primary_action_in_project(&editor, &project,
        (Vec2){3000, 2000}, &action));
    assert(editor.current_wall_id == first);

    Renderer2D *renderer = renderer2d_create();
    assert(renderer);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing, .draw_line = record_line, .fill_rect = record_rect
    });
    WallRenderStyle style = {.timber_colour = {100, 100, 100, 255}};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 2);
    editor.current_room_id = 999; /* Stale room navigation cannot gate walls. */
    assert(app_current_wall(&project, &editor)->id == first);
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 2);
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_room_id == DOMAIN_ID_INVALID);
    assert(editor.current_wall_id == first);

    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.rect_count > 2 && drawing.line_count == 0);
    const Wall *selected_wall = app_current_wall_const(&project, &editor);
    editor_selection_set_wall_member(&editor.selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, first,
        WALL_MEMBER_BOTTOM_PLATE, &selected_wall->framing.bottomplate);
    editor.current_room_id = 999;
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == first && editor.current_room_id == DOMAIN_ID_INVALID);
    assert(wall_selection_resolve(&editor.selection.wall_member, selected_wall));
    OpeningCommand opening;
    assert(opening_command_create(first, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
    assert(sitehelper_command_from_opening(&opening, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    DomainId opening_id = result.data.add_opening.opening_id;
    DomainId next = project.domain_ids.next;
    assert(project.storeys[0].structure.room_count == 0);
    assert(sitehelper_command_history_undo(&history, &project));
    assert(!wall_find_opening_by_id_const(app_current_wall_const(&project, &editor), opening_id));
    assert(sitehelper_command_history_redo(&history, &project));
    assert(wall_find_opening_by_id_const(app_current_wall_const(&project, &editor), opening_id));
    assert(project.domain_ids.next == next);

    DeleteWallCommand deletion;
    assert(delete_wall_command_create(first, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    assert(!build_find_wall_by_id(&project.storeys[0].structure, first));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == DOMAIN_ID_INVALID);
    /* Room state may change after capture; restoring the physical wall has
     * no dependency on that room, and must preserve its independent identity. */
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(room_id != DOMAIN_ID_INVALID);
    editor.current_room_id = room_id;
    next = project.domain_ids.next;
    for (int cycle = 0; cycle < 3; cycle++) {
        assert(sitehelper_command_history_undo(&history, &project));
        const Wall *wall = build_find_wall_by_id_const(&project.storeys[0].structure, first);
        assert(wall && wall->definition.segment.start.x == 1000 && wall->definition.segment.start.y == 2000);
        assert(wall->definition.segment.end.x == 5000 && wall->definition.segment.end.y == 2000);
        assert(wall_find_opening_by_id_const(wall, opening_id));
        assert(build_find_room_by_id(&project.storeys[0].structure, room_id));
        assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
        assert(sitehelper_command_history_redo(&history, &project));
        assert(project.domain_ids.next == next);
        assert(build_find_room_by_id(&project.storeys[0].structure, room_id));
    }
    assert(sitehelper_command_history_undo(&history, &project));
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_PLAN));
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 2); /* A selected identity-only room filters nothing. */
    assert(sitehelper_editor_primary_action_in_project(&editor, &project,
        (Vec2){-3000, -1000}, &action));
    assert(editor.current_wall_id == second);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    renderer2d_destroy(renderer);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_room_placement_preserves_wall_navigation_and_rendering(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    editor.current_wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{1000, 2000}, {5000, 2000}});
    DomainId wall_id = editor.current_wall_id;
    const Room *room = app_current_room_const(&project, &editor);
    assert(room && !room->has_location);
    assert(sitehelper_project_set_room_location(&project, room->id,
        (PlanPosition){-50000, 60000}));
    sitehelper_editor_reconcile(&editor, &project);
    room = app_current_room_const(&project, &editor);
    assert(room && room->has_location && room->location.x == -50000 && room->location.y == 60000);
    assert(editor.current_wall_id == wall_id && app_current_wall_const(&project, &editor)->id == wall_id);
    Renderer2D *renderer = renderer2d_create();
    assert(renderer);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing, .draw_line = record_line, .fill_rect = record_rect
    });
    WallRenderStyle style = {.timber_colour = {100, 100, 100, 255}};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 1 && drawing.rect_count == 0);
    assert(sitehelper_project_clear_room_location(&project, room->id));
    sitehelper_editor_reconcile(&editor, &project);
    assert(!app_current_room_const(&project, &editor)->has_location);
    assert(editor.current_wall_id == wall_id);
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(&editor, &project,
        (Vec2){3000, 2000}, &action));
    assert(editor.current_wall_id == wall_id);
    renderer2d_destroy(renderer);
    sitehelper_project_destroy(&project);
}

static void test_separator_inputs_stay_out_of_wall_views_and_navigation(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, room_id, (PlanPosition){100, 200}));
    DomainId wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){{0, 0}, {4200, 0}});
    assert(wall_generate(build_find_wall_by_id(&project.storeys[0].structure, wall_id), &project.settings));
    editor.current_room_id = room_id;
    editor.current_wall_id = wall_id;
    DomainId separator = sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 5000}, {4200, 5000}});
    assert(separator);
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_room_id == room_id && editor.current_wall_id == wall_id);
    Renderer2D *renderer = renderer2d_create();
    assert(renderer);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing, .draw_line = record_line, .fill_rect = record_rect
    });
    WallRenderStyle style = {.timber_colour = {100, 100, 100, 255}};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.line_count == 1 && drawing.rect_count == 0);
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    size_t member_rectangles = drawing.rect_count;
    assert(member_rectangles > 2 && drawing.line_count == 0);
    assert(sitehelper_project_set_room_separator_segment(&project, separator, (PlanSegment){{-1, -2}, {3, 4}}));
    assert(sitehelper_project_remove_room_separator_by_id(&project, separator));
    sitehelper_editor_reconcile(&editor, &project);
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &style, NULL);
    assert(drawing.rect_count == member_rectangles && drawing.line_count == 0);
    assert(editor.current_room_id == room_id && editor.current_wall_id == wall_id);
    const Room *room = app_current_room_const(&project, &editor);
    assert(room && room->has_location && room->location.x == 100 && room->location.y == 200);
    renderer2d_destroy(renderer);
    sitehelper_project_destroy(&project);
}

static void test_elevation_highlight_requires_scope_and_owner(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    DomainId first = sitehelper_project_add_wall(&project, storey, (WallPlanSegment){{0,0},{4200,0}});
    DomainId second = sitehelper_project_add_wall(&project, storey, (WallPlanSegment){{0,1000},{4200,1000}});
    Wall *wall = sitehelper_project_find_wall_by_id(&project, first);
    assert(wall_generate(wall, &project.settings));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey));
    editor.current_wall_id = first;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    Renderer2D *renderer = renderer2d_create(); assert(renderer);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){.context = &drawing, .fill_rect = record_rect});
    renderer2d_set_camera(renderer, (Camera2D){.scale = .1});
    renderer2d_set_viewport(renderer, (Vec2){0,0}, 800,600);
    WallRenderStyle style = {.timber_colour = {100,100,100,255}};
    AppInteractionStyle interaction = app_interaction_style_default();
    interaction.selected_colour = (Colour){255,0,0,255};
    editor_selection_set_wall_member(&editor.selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        first, WALL_MEMBER_BOTTOM_PLATE, &wall->framing.bottomplate);
    app_render_walls(renderer, &project, &editor, &style, &interaction);
    assert(drawing.selected_rect_count == 1);
    editor.selection.scope = EDITOR_SELECTION_SCOPE_PLAN;
    drawing = (Drawing){0}; app_render_walls(renderer, &project, &editor, &style, &interaction);
    assert(drawing.rect_count > 0 && drawing.selected_rect_count == 0);
    editor.selection.scope = EDITOR_SELECTION_SCOPE_WALL_ELEVATION;
    editor.selection.wall_id = second;
    drawing = (Drawing){0}; app_render_walls(renderer, &project, &editor, &style, &interaction);
    assert(drawing.rect_count > 0 && drawing.selected_rect_count == 0);
    sitehelper_editor_clear_selection(&editor);
    assert(editor.current_wall_id == first);
    drawing = (Drawing){0}; app_render_walls(renderer, &project, &editor, &style, &interaction);
    assert(drawing.rect_count > 0 && drawing.selected_rect_count == 0);
    renderer2d_destroy(renderer); sitehelper_project_destroy(&project);
}

int main(void)
{
    test_elevation_highlight_requires_scope_and_owner();
    test_separator_inputs_stay_out_of_wall_views_and_navigation();
    test_room_placement_preserves_wall_navigation_and_rendering();
    test_plan_commands_and_navigation_without_rooms();
    test_view_rendering_and_local_pointer();
    test_switching_views_clears_tools_and_retains_cameras();
    test_plan_focus_uses_nearest_physical_segment();
    test_views_and_history_preserve_persistent_project();
    puts("All app view tests passed.");
    return 0;
}
