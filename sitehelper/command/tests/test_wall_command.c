#include <assert.h>
#include <stdio.h>

#include "sitehelper_command.h"
#include "command_history.h"
#include "wall.h"

static DomainId setup_project(SiteHelperProject *project)
{
    sitehelper_project_init(project);
    DomainId room_id = sitehelper_project_add_room(project);
    assert(room_id != DOMAIN_ID_INVALID);
    return room_id;
}

static SiteHelperCommand add_wall_command(
    DomainId room_id,
    int x,
    int y,
    int length
)
{
    WallCommand wall;
    SiteHelperCommand command;

    assert(wall_command_create(
        room_id,
        (Position){ .x = x, .y = y },
        length,
        &wall
    ));
    assert(sitehelper_command_from_wall(&wall, &command));
    return command;
}

static void test_execute_undo_redo_preserves_identity(void)
{
    SiteHelperProject project;
    DomainId room_id = setup_project(&project);
    SiteHelperCommand command = add_wall_command(room_id, 5000, 3000, 4200);
    SiteHelperCommandResult result;
    DomainId next_before = project.domain_ids.next;

    assert(sitehelper_command_execute(&project, &command, &result));
    assert(result.type == SITEHELPER_COMMAND_ADD_WALL);
    assert(result.data.add_wall.wall_id == next_before);
    assert(project.structure.wall_count == 1);

    Room *room = build_find_room_by_id(&project.structure, room_id);
    assert(room_has_wall_id(room, next_before));
    Wall *wall = build_find_wall_by_id(&project.structure, next_before);
    assert(wall != NULL);
    assert(wall->definition.origin.x == 5000);
    assert(wall->definition.origin.y == 3000);
    assert(wall->definition.length == 4200);
    assert(wall->framing.stud_count > 0);

    DomainId next_after_execute = project.domain_ids.next;
    assert(sitehelper_command_undo(&project, &command, &result));
    assert(project.structure.wall_count == 0);
    assert(build_find_wall_by_id(&project.structure, next_before) == NULL);
    assert(!room_has_wall_id(room, next_before));
    assert(sitehelper_command_redo(&project, &command, &result));
    assert(project.structure.wall_count == 1);
    assert(project.domain_ids.next == next_after_execute);
    assert(build_find_wall_by_id(&project.structure, next_before) != NULL);

    sitehelper_project_destroy(&project);
}

static void test_failure_does_not_consume_identity(void)
{
    SiteHelperProject project;
    DomainId room_id = setup_project(&project);
    SiteHelperCommandResult result;
    DomainId next = project.domain_ids.next;
    SiteHelperCommand invalid = add_wall_command(room_id, 0, 0, 4200);

    SiteHelperCommand existing = add_wall_command(room_id, 0, 0, 4200);
    assert(sitehelper_command_execute(&project, &existing, &result));
    DomainId existing_wall_id = result.data.add_wall.wall_id;
    next = project.domain_ids.next;

    /* Constructing an invalid command is intentionally rejected. */
    assert(!wall_command_create(room_id, (Position){0}, 0,
        &invalid.data.wall));

    invalid.data.wall = (WallCommand){
        .room_id = room_id,
        .length = 0
    };
    invalid.type = SITEHELPER_COMMAND_ADD_WALL;

    assert(!sitehelper_command_execute(&project, &invalid, &result));
    assert(result.type == SITEHELPER_COMMAND_NONE);
    assert(project.domain_ids.next == next);
    Room *room = build_find_room_by_id(&project.structure, room_id);
    assert(room->wall_count == 1);
    Wall *wall = build_find_wall_by_id(&project.structure, existing_wall_id);
    assert(wall != NULL);
    assert(wall->definition.length == 4200);
    assert(wall->definition.origin.x == 0);
    assert(wall->definition.origin.y == 0);

    sitehelper_project_destroy(&project);
}

static void test_history_interleaving_keeps_wall_ids(void)
{
    SiteHelperProject project;
    SiteHelperCommandHistory history;
    DomainId room_id = setup_project(&project);
    sitehelper_command_history_init(&history);

    SiteHelperCommand first = add_wall_command(room_id, 0, 0, 4200);
    SiteHelperCommand second = add_wall_command(room_id, 5000, 3000, 4200);
    SiteHelperCommandResult first_result;
    SiteHelperCommandResult second_result;

    assert(sitehelper_command_history_execute(&history, &project, &first,
        &first_result));
    assert(sitehelper_command_history_execute(&history, &project, &second,
        &second_result));
    assert(sitehelper_command_history_undo(&history, &project));
    assert(sitehelper_command_history_undo(&history, &project));
    assert(sitehelper_command_history_redo(&history, &project));
    assert(sitehelper_command_history_redo(&history, &project));

    Room *room = build_find_room_by_id(&project.structure, room_id);
    assert(build_find_wall_by_id(
        &project.structure, first_result.data.add_wall.wall_id));
    assert(build_find_wall_by_id(
        &project.structure, second_result.data.add_wall.wall_id));

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_execute_undo_redo_preserves_identity();
    test_failure_does_not_consume_identity();
    test_history_interleaving_keeps_wall_ids();
    puts("wall command tests passed");
    return 0;
}
