#include <assert.h>
#include <limits.h>
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
        (WallPlanSegment){ .start = {x, y}, .end = {x + length, y} },
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
    assert(wall->definition.segment.start.x == 5000);
    assert(wall->definition.segment.start.y == 3000);
    assert(wall_length_mm(wall) == 4200);
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
    assert(!wall_command_create(room_id, (WallPlanSegment){0},
        &invalid.data.wall));

    invalid.data.wall = (WallCommand){
        .room_id = room_id,
        .segment = {0}
    };
    invalid.type = SITEHELPER_COMMAND_ADD_WALL;

    assert(!sitehelper_command_execute(&project, &invalid, &result));
    assert(result.type == SITEHELPER_COMMAND_NONE);
    assert(project.domain_ids.next == next);
    Room *room = build_find_room_by_id(&project.structure, room_id);
    assert(room->wall_count == 1);
    Wall *wall = build_find_wall_by_id(&project.structure, existing_wall_id);
    assert(wall != NULL);
    assert(wall_length_mm(wall) == 4200);
    assert(wall->definition.segment.start.x == 0);
    assert(wall->definition.segment.start.y == 0);

    sitehelper_project_destroy(&project);
}

static void test_diagonal_commands_preserve_order_through_history(void)
{
    const WallPlanSegment segments[] = {
        { .start = {1000, 2000}, .end = {5000, 5000} },
        { .start = {5000, 5000}, .end = {1000, 2000} }
    };
    for (size_t i = 0; i < sizeof segments / sizeof segments[0]; i++) {
        SiteHelperProject project;
        DomainId room_id = setup_project(&project);
        SiteHelperCommandHistory history;
        sitehelper_command_history_init(&history);
        WallCommand wall_command;
        SiteHelperCommand command;
        SiteHelperCommandResult result;
        assert(wall_command_create(room_id, segments[i], &wall_command));
        assert(sitehelper_command_from_wall(&wall_command, &command));
        DomainId id = project.domain_ids.next;
        assert(sitehelper_command_history_execute(&history, &project, &command, &result));
        assert(result.data.add_wall.wall_id == id);
        DomainId next = project.domain_ids.next;
        for (int pass = 0; pass < 2; pass++) {
            const Wall *wall = build_find_wall_by_id_const(&project.structure, id);
            assert(wall != NULL && wall->id == id);
            assert(wall->definition.segment.start.x == segments[i].start.x);
            assert(wall->definition.segment.start.y == segments[i].start.y);
            assert(wall->definition.segment.end.x == segments[i].end.x);
            assert(wall->definition.segment.end.y == segments[i].end.y);
            assert(wall_length_mm(wall) == 5000);
            assert(wall->framing.bottomplate.position.u == 0);
            assert(wall->framing.bottomplate.position.z == 0);
            if (pass == 0) {
                assert(sitehelper_command_history_undo(&history, &project));
                assert(project.structure.wall_count == 0);
                assert(build_find_room_by_id(&project.structure, room_id)->wall_count == 0);
                assert(sitehelper_command_history_redo(&history, &project));
                assert(project.domain_ids.next == next);
            }
        }
        sitehelper_command_history_destroy(&history);
        sitehelper_project_destroy(&project);
    }
}

static void test_overflow_segment_command_is_transactional(void)
{
    SiteHelperProject project;
    DomainId room_id = setup_project(&project);
    DomainId next = project.domain_ids.next;
    WallPlanSegment segment = { .start = {INT_MIN, 0}, .end = {INT_MAX, 0} };
    WallCommand command = { .room_id = room_id, .segment = segment };
    assert(!wall_command_create(room_id, segment, &command));
    DomainId id = 42;
    assert(!wall_command_execute(&project, &command, &id));
    assert(id == DOMAIN_ID_INVALID);
    assert(project.domain_ids.next == next);
    assert(project.structure.wall_count == 0);
    assert(project.structure.room_count == 1);
    assert(build_find_room_by_id(&project.structure, room_id)->wall_count == 0);
    assert(!wall_command_redo(&project, &command, next));
    assert(project.structure.wall_count == 0);
    assert(project.domain_ids.next == next);
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
    test_diagonal_commands_preserve_order_through_history();
    test_overflow_segment_command_is_transactional();
    puts("wall command tests passed");
    return 0;
}
