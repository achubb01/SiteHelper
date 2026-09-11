#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"
#include "command_history.h"
#include "sitehelper_command.h"
#include "wall.h"

static Wall *add_wall(
    SiteHelperProject *project,
    DomainId wall_id,
    PlanPosition origin
)
{
    Wall candidate = { .id = wall_id };

    assert(build_append_wall(&project->storeys[0].structure, &candidate));
    Wall *wall = build_find_wall_by_id(&project->storeys[0].structure, wall_id);
    assert(wall_set_plan_segment(wall, (WallPlanSegment){
        .start = origin, .end = { .x = origin.x + 4200, .y = origin.y }
    }));
    assert(wall_generate(wall, &project->settings));
    return wall;
}

static void test_positioned_walls_select_by_stable_identity(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    add_wall(&project, 20, (PlanPosition){ 0, 0 });
    add_wall(&project, 30, (PlanPosition){ 5000, 3000 });

    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = room_id;
    EditorAction action;

    assert(sitehelper_editor_primary_action_in_project(
        &editor, &project, (Vec2){ 10, 10 }, &action));
    assert(editor.selection.kind == EDITOR_SELECTION_WALL);
    assert(editor.selection.wall_id == editor.current_wall_id);
    assert(editor.current_wall_id == 20);
    assert(editor_selection_get_wall_member(&editor.selection, 30) == NULL);

    assert(sitehelper_editor_primary_action_in_project(
        &editor, &project, (Vec2){ 5010, 3010 }, &action));
    assert(editor.selection.kind == EDITOR_SELECTION_WALL);
    assert(editor.selection.wall_id == editor.current_wall_id);
    assert(editor.current_wall_id == 30);
    assert(editor_selection_get_wall_member(&editor.selection, 20) == NULL);

    for (DomainId id = 40; id < 50; id++) {
        add_wall(&project, id, (PlanPosition){ (int)id * 1000, 0 });
    }

    assert(build_find_room_by_id(&project.storeys[0].structure, room_id));
    assert(build_find_wall_by_id(
        &project.storeys[0].structure, editor.current_wall_id)->id == 30);
    assert(editor.selection.kind == EDITOR_SELECTION_WALL);
    assert(editor.selection.wall_id == editor.current_wall_id);

    sitehelper_project_destroy(&project);
}

static void test_opening_path_uses_positioned_wall_local_coordinates(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    Wall *wall = add_wall(
        &project, 20, (PlanPosition){ 5000, 3000 }
    );

    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = room_id;
    editor.current_wall_id = wall->id;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_OPENING));

    sitehelper_editor_pointer_move(
        &editor,
        wall,
        &project.settings,
        (Vec2){ 1600, 800 }
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    assert(placement->has_candidate);
    assert(placement->left < 3000.0);

    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(
        &editor,
        &project,
        (Vec2){ 1600, 800 },
        &action
    ));
    assert(action.kind == EDITOR_ACTION_COMMAND);
    assert(action.command.type == SITEHELPER_COMMAND_ADD_OPENING);
    assert(action.command.data.opening.frame_position == (int)placement->left);

    sitehelper_project_destroy(&project);
}

static void test_reconcile_clears_removed_wall_navigation_and_selection(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    SiteHelperCommand command;
    SiteHelperCommandResult result;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    WallCommand wall_command;
    assert(wall_command_create(1,
        (WallPlanSegment){ .end = { .x = 4200 } }, &wall_command));
    assert(sitehelper_command_from_wall(&wall_command, &command));
    sitehelper_command_history_init(&history);
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));

    Wall *wall = build_find_wall_by_id(
        &project.storeys[0].structure, result.data.add_wall.wall_id);
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = room_id;
    editor.current_wall_id = wall->id;
    sitehelper_editor_select_wall_member_at_position(
        &editor, wall, (WallLocalPosition){ .u = 10, .z = 10 }
    );

    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_room_id == room_id);
    assert(editor.current_wall_id == DOMAIN_ID_INVALID);
    assert(editor.selection.kind == EDITOR_SELECTION_NONE);

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_delete_wall_reconciliation_does_not_restore_transient_state(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    sitehelper_command_history_init(&history);
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{4600, 6800}, {1000, 2000}});
    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);
    assert(wall != NULL);
    assert(wall_add_opening(wall, &project.settings,
        domain_id_generate(&project.domain_ids), OPENING_WINDOW, 1500, 900, 1000, 1000));
    assert(wall_generate(wall, &project.settings));
    editor.current_room_id = room_id;
    editor.current_wall_id = wall_id;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    sitehelper_editor_select_wall_member_at_position(
        &editor, wall, (WallLocalPosition){10, 10});
    assert(!editor_selection_is_empty(&editor.selection));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_OPENING));
    editor.opening_placement.has_candidate = 1;

    DeleteWallCommand deletion;
    SiteHelperCommand command;
    SiteHelperCommandResult result;
    assert(delete_wall_command_create(wall_id, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &command));
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_room_id == room_id);
    assert(editor.current_wall_id == DOMAIN_ID_INVALID);
    assert(editor_selection_is_empty(&editor.selection));
    assert(!editor.opening_placement.has_candidate);
    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(build_find_wall_by_id(&project.storeys[0].structure, wall_id) != NULL);
    assert(editor.current_wall_id == DOMAIN_ID_INVALID);
    assert(editor_selection_is_empty(&editor.selection));
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_wall_preview_and_command_preserve_diagonal_clicks(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = 1;
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    EditorAction action;
    Vec2 first = {5000, 5000};
    Vec2 second = {1000, 2000};
    assert(sitehelper_editor_primary_action(&editor, NULL, first, &action));
    assert(action.kind == EDITOR_ACTION_NONE);
    WallPlanSegment preview;
    assert(!sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(!sitehelper_editor_primary_action(&editor, NULL, first, &action));
    sitehelper_editor_pointer_move(&editor, NULL, NULL, second);
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.start.x == 5000 && preview.start.y == 5000);
    assert(preview.end.x == 1000 && preview.end.y == 2000);
    assert(sitehelper_editor_primary_action(&editor, NULL, second, &action));
    assert(action.kind == EDITOR_ACTION_COMMAND);
    assert(action.command.type == SITEHELPER_COMMAND_ADD_WALL);
    WallPlanSegment segment = action.command.data.wall.segment;
    assert(segment.start.x == preview.start.x && segment.start.y == preview.start.y);
    assert(segment.end.x == preview.end.x && segment.end.y == preview.end.y);
}

static void test_wall_second_click_sets_endpoint_without_pointer_move(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    editor.current_room_id = 1;
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    EditorAction action;
    assert(sitehelper_editor_primary_action(&editor, NULL,
        (Vec2){1000, 2000}, &action));
    assert(action.kind == EDITOR_ACTION_NONE);
    assert(sitehelper_editor_primary_action(&editor, NULL,
        (Vec2){5000, 5000}, &action));
    assert(action.kind == EDITOR_ACTION_COMMAND);
    WallPlanSegment segment = action.command.data.wall.segment;
    assert(segment.start.x == 1000 && segment.start.y == 2000);
    assert(segment.end.x == 5000 && segment.end.y == 5000);
}

static void test_endpoint_move_reconciles_regenerated_selection(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    sitehelper_command_history_init(&history);
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{1000, 2000}, {5000, 2000}});
    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);
    assert(wall);
    assert(wall_generate(wall, &project.settings));
    editor.current_room_id = room_id;
    editor.current_wall_id = wall_id;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    editor_selection_set_wall_member(&editor.selection, wall_id,
        WALL_MEMBER_STUD, &wall->framing.studs[0]);
    assert(!editor_selection_is_empty(&editor.selection));

    SiteHelperCommand command = {.type = SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
        .data.move_wall_endpoint = {wall_id, WALL_ENDPOINT_END, {1000, 6000}}};
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == wall_id && editor.current_room_id == room_id);
    assert(editor.active_view == EDITOR_VIEW_WALL_ELEVATION);
    assert(wall_selection_resolve(&editor.selection.wall_member, wall));
    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == wall_id);
    assert(wall_selection_resolve(&editor.selection.wall_member, wall));
    assert(sitehelper_command_history_redo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == wall_id);
    assert(wall_selection_resolve(&editor.selection.wall_member, wall));

    /* The old far-end stud disappears when the wall gets shorter. Selection
     * owns a value, so reconciliation must drop it if it cannot resolve. */
    editor_selection_set_wall_member(&editor.selection, wall_id,
        WALL_MEMBER_STUD, &wall->framing.studs[wall->framing.stud_count - 1]);
    assert(!editor_selection_is_empty(&editor.selection));
    command.data.move_wall_endpoint.new_position = (PlanPosition){1000, 5000};
    editor.opening_placement.has_candidate = 1;
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == wall_id);
    assert(editor_selection_is_empty(&editor.selection));
    assert(!editor.opening_placement.has_candidate);
    assert(editor.active_view == EDITOR_VIEW_WALL_ELEVATION);
    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == wall_id);
    assert(editor_selection_is_empty(&editor.selection));
    assert(sitehelper_command_history_redo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_wall_id == wall_id);
    assert(editor_selection_is_empty(&editor.selection));
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_endpoint_move_reconciles_regenerated_selection();
    test_delete_wall_reconciliation_does_not_restore_transient_state();
    test_wall_second_click_sets_endpoint_without_pointer_move();
    test_wall_preview_and_command_preserve_diagonal_clicks();
    test_positioned_walls_select_by_stable_identity();
    test_opening_path_uses_positioned_wall_local_coordinates();
    test_reconcile_clears_removed_wall_navigation_and_selection();
    puts("multi-wall editor tests passed");
    return 0;
}
