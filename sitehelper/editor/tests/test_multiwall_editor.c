#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"
#include "command_history.h"
#include "sitehelper_command.h"
#include "wall.h"

static Wall *add_wall(
    SiteHelperProject *project,
    DomainId room_id,
    DomainId wall_id,
    Position origin
)
{
    Room *room = build_find_room_by_id(&project->structure, room_id);
    assert(room != NULL);
    assert(room_add_wall(room, wall_id));
    Wall *wall = room_find_wall_by_id(room, wall_id);
    assert(wall_set_origin(wall, origin));
    assert(wall_set_length(wall, 4200));
    assert(wall_generate(wall, &project->settings));
    return wall;
}

static void test_positioned_walls_select_by_stable_identity(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    DomainId room_id = sitehelper_project_add_room(&project);
    add_wall(&project, room_id, 20, (Position){ 0, 0 });
    add_wall(&project, room_id, 30, (Position){ 5000, 3000 });
    Room *room = build_find_room_by_id(&project.structure, room_id);

    sitehelper_editor_init(&editor);
    editor.current_room_id = room_id;
    EditorAction action;

    assert(sitehelper_editor_primary_action_in_room(
        &editor, room, (Vec2){ 10, 10 }, &action));
    assert(editor.selection.wall_id == 20);
    assert(editor.current_wall_id == 20);
    assert(editor_selection_get_wall_member(&editor.selection, 30) == NULL);

    assert(sitehelper_editor_primary_action_in_room(
        &editor, room, (Vec2){ 5010, 3010 }, &action));
    assert(editor.selection.wall_id == 30);
    assert(editor.current_wall_id == 30);
    assert(editor_selection_get_wall_member(&editor.selection, 20) == NULL);

    for (DomainId id = 40; id < 50; id++) {
        add_wall(&project, room_id, id, (Position){ (int)id * 1000, 0 });
    }

    room = build_find_room_by_id(&project.structure, room_id);
    assert(room_find_wall_by_id(room, editor.current_wall_id)->id == 30);
    assert(editor.selection.wall_id == 30);

    sitehelper_project_destroy(&project);
}

static void test_opening_path_uses_positioned_wall_local_coordinates(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    DomainId room_id = sitehelper_project_add_room(&project);
    Wall *wall = add_wall(
        &project, room_id, 20, (Position){ 5000, 3000 }
    );

    sitehelper_editor_init(&editor);
    editor.current_room_id = room_id;
    editor.current_wall_id = wall->id;
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_OPENING));

    sitehelper_editor_pointer_move(
        &editor,
        wall,
        &project.settings,
        (Vec2){ 6600, 3800 }
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    assert(placement->has_candidate);
    assert(placement->left < 3000.0);

    EditorAction action;
    assert(sitehelper_editor_primary_action_in_room(
        &editor,
        build_find_room_by_id(&project.structure, room_id),
        (Vec2){ 6600, 3800 },
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
    DomainId room_id = sitehelper_project_add_room(&project);
    WallCommand wall_command;
    assert(wall_command_create(room_id, (Position){0}, 4200, &wall_command));
    assert(sitehelper_command_from_wall(&wall_command, &command));
    sitehelper_command_history_init(&history);
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));

    Room *room = build_find_room_by_id(&project.structure, room_id);
    Wall *wall = room_find_wall_by_id(room, result.data.add_wall.wall_id);
    sitehelper_editor_init(&editor);
    editor.current_room_id = room_id;
    editor.current_wall_id = wall->id;
    sitehelper_editor_select_wall_member_at_position(
        &editor, wall, (Position){ 10, 10 }
    );

    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(editor.current_room_id == room_id);
    assert(editor.current_wall_id == DOMAIN_ID_INVALID);
    assert(editor.selection.kind == EDITOR_SELECTION_NONE);

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_positioned_walls_select_by_stable_identity();
    test_opening_path_uses_positioned_wall_local_coordinates();
    test_reconcile_clears_removed_wall_navigation_and_selection();
    puts("multi-wall editor tests passed");
    return 0;
}
