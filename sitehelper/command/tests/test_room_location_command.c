#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "command_history.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t allocations_before_failure = SIZE_MAX;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int fail_allocation(void)
{
    return allocations_before_failure != SIZE_MAX && allocations_before_failure-- == 0;
}
void *__wrap_malloc(size_t size)
{
    return fail_allocation() ? NULL : __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    return fail_allocation() ? NULL : __real_calloc(count, size);
}
void *__wrap_realloc(void *pointer, size_t size)
{
    return fail_allocation() ? NULL : __real_realloc(pointer, size);
}
#endif

static SiteHelperCommand placement(DomainId id, bool placed, PlanPosition point)
{
    RoomLocationCommand location;
    SiteHelperCommand command;
    assert(placed ? room_location_command_create(id, point, &location)
        : room_location_command_create_clear(id, &location));
    assert(sitehelper_command_from_room_location(&location, &command));
    return command;
}

static void check(const SiteHelperProject *project, DomainId id,
    bool placed, PlanPosition point, DomainId next)
{
    const Room *room = build_find_room_by_id_const(&project->storeys[0].structure, id);
    assert(room && room->has_location == placed);
    if (placed) {
        assert(room->location.x == point.x && room->location.y == point.y);
    }
    assert(project->domain_ids.next == next);
    assert(sitehelper_project_validate(project).code == SITEHELPER_PROJECT_VALID);
}

static void test_history_exactness_failures_and_branch_discard(void)
{
    SiteHelperProject project, before;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_command_history_init(&history);
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId other = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId next = project.domain_ids.next;
    PlanPosition origin = {0}, distant = {INT_MIN, INT_MAX};
    SiteHelperCommand commands[] = {
        placement(room, true, origin), placement(room, true, distant),
        placement(room, false, origin)
    };
    SiteHelperCommandResult result;
    for (size_t i = 0; i < 3; i++) {
        assert(sitehelper_command_history_execute(&history, &project, &commands[i], &result));
        assert(result.type == SITEHELPER_COMMAND_SET_ROOM_LOCATION);
        assert(result.data.room_location.room_id == room);
    }
    check(&project, room, false, origin, next);
    assert(!sitehelper_command_undo(&project, &commands[2], &result));
    for (int cycle = 0; cycle < 4; cycle++) {
        for (int i = 2; i >= 0; i--) {
            assert(sitehelper_command_history_undo(&history, &project));
            check(&project, room, i != 0, i == 2 ? distant : origin, next);
        }
        for (int i = 0; i < 3; i++) {
            assert(sitehelper_command_history_redo(&history, &project));
            check(&project, room, i != 2, i == 1 ? distant : origin, next);
        }
        check(&project, other, false, origin, next);
    }

    /* Missing targets must retain the cursor and snapshot for a later retry. */
    Room *target = build_find_room_by_id(&project.storeys[0].structure, room);
    target->id = 999;
    test_clone_project_authoritative(&project, &before);
    assert(!sitehelper_command_history_undo(&history, &project));
    assert(history.cursor == 3 && history.count == 3);
    test_assert_project_authoritative_equal(&before, &project);
    sitehelper_project_destroy(&before);
    target->id = room;
    assert(sitehelper_command_history_undo(&history, &project));
    target->id = 999;
    test_clone_project_authoritative(&project, &before);
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(history.cursor == 2 && history.count == 3);
    test_assert_project_authoritative_equal(&before, &project);
    SiteHelperCommand missing = placement(room, true, origin);
    assert(!sitehelper_command_history_execute(&history, &project, &missing, &result));
    assert(result.type == SITEHELPER_COMMAND_NONE);
    assert(history.cursor == 2 && history.count == 3);
    test_assert_project_authoritative_equal(&before, &project);
    sitehelper_project_destroy(&before);
    target->id = room;
    assert(sitehelper_command_history_redo(&history, &project));
    assert(sitehelper_command_history_undo(&history, &project));

    SiteHelperCommand branch = placement(other, true, (PlanPosition){-123, 456});
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    allocations_before_failure = 0; /* Snapshot failure must preserve the redo branch. */
    assert(!sitehelper_command_history_execute(&history, &project, &branch, &result));
    allocations_before_failure = SIZE_MAX;
    assert(history.cursor == 2 && history.count == 3);
    check(&project, room, true, distant, next);
    check(&project, other, false, origin, next);
#endif
    assert(sitehelper_command_history_execute(&history, &project, &branch, &result));
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(history.count == 3 && history.cursor == 3);
    check(&project, room, true, distant, next);
    check(&project, other, true, (PlanPosition){-123, 456}, next);
    /* Relocate both room storage and history; only stable IDs may be retained. */
    for (int i = 0; i < 20; i++) {
        assert(sitehelper_project_add_room(&project, project.storeys[0].id));
        SiteHelperCommand move = placement(room, true, (PlanPosition){i, -i});
        assert(sitehelper_command_history_execute(&history, &project, &move, &result));
    }
    next = project.domain_ids.next;
    for (int i = 0; i < 20; i++) {
        assert(sitehelper_command_history_undo(&history, &project));
    }
    check(&project, room, true, distant, next);
    sitehelper_command_history_destroy(&history); /* Includes live and redo snapshots. */
    sitehelper_project_destroy(&project);
}

static void test_invalid_commands(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    RoomLocationCommand location;
    assert(!room_location_command_create(0, (PlanPosition){0}, &location));
    assert(!room_location_command_create(1, (PlanPosition){0}, NULL));
    assert(!room_location_command_create_clear(0, &location));
    assert(!room_location_command_create_clear(1, NULL));
    assert(!room_location_command_execute(&project, NULL));
    assert(!sitehelper_command_from_room_location(NULL, NULL));
    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    SiteHelperCommand command = placement(room, true, (PlanPosition){12, -34});
    assert(!room_location_command_execute(NULL, &command.data.room_location));
    assert(!sitehelper_command_from_room_location(&command.data.room_location, NULL));
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project, &command, &result));
    result.data.room_location.room_id++;
    assert(!sitehelper_command_redo(&project, &command, &result));
    command.data.room_location.room_id = DOMAIN_ID_INVALID;
    assert(!sitehelper_command_execute(&project, &command, &result));
    assert(result.type == SITEHELPER_COMMAND_NONE);
    check(&project, room, true, (PlanPosition){12, -34}, project.domain_ids.next);
    sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures_and_allocation_free_mutation(void)
{
    for (size_t fail_at = 0; fail_at < 2; fail_at++) {
        SiteHelperProject project;
        SiteHelperCommandHistory history;
        sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
        sitehelper_command_history_init(&history);
        DomainId id = sitehelper_project_add_room(&project, project.storeys[0].id);
        DomainId next = project.domain_ids.next;
        SiteHelperCommand command = placement(id, true, (PlanPosition){0});
        SiteHelperCommandResult result;
        allocations_before_failure = fail_at; /* History reserve, then snapshot. */
        assert(!sitehelper_command_history_execute(&history, &project, &command, &result));
        allocations_before_failure = SIZE_MAX;
        assert(history.count == 0 && history.cursor == 0);
        check(&project, id, false, (PlanPosition){0}, next);
        assert(sitehelper_command_history_execute(&history, &project, &command, &result));
        allocations_before_failure = 0;
        assert(sitehelper_command_history_undo(&history, &project));
        assert(sitehelper_command_history_redo(&history, &project));
        assert(sitehelper_project_clear_room_location(&project, id));
        assert(sitehelper_project_set_room_location(&project, id, (PlanPosition){0}));
        assert(allocations_before_failure == 0); /* No allocation attempted. */
        allocations_before_failure = SIZE_MAX;
        check(&project, id, true, (PlanPosition){0}, next);
        sitehelper_command_history_destroy(&history);
        sitehelper_project_destroy(&project);
    }
}
#endif

int main(void)
{
    test_history_exactness_failures_and_branch_discard();
    test_invalid_commands();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures_and_allocation_free_mutation();
#endif
    puts("room location command tests passed");
    return 0;
}
