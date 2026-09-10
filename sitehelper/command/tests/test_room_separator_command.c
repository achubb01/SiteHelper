#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
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
void *__wrap_malloc(size_t size) { return fail_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return fail_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *pointer, size_t size) { return fail_allocation() ? NULL : __real_realloc(pointer, size); }
#endif

static SiteHelperCommand add(PlanSegment segment)
{
    AddRoomSeparatorCommand value;
    SiteHelperCommand command;
    assert(add_room_separator_command_create(1, segment, &value));
    assert(sitehelper_command_from_add_room_separator(&value, &command));
    return command;
}
static SiteHelperCommand deletion(DomainId id)
{
    DeleteRoomSeparatorCommand value;
    SiteHelperCommand command;
    assert(delete_room_separator_command_create(id, &value));
    assert(sitehelper_command_from_delete_room_separator(&value, &command));
    return command;
}
static SiteHelperCommand move(DomainId id, RoomSeparatorEndpoint endpoint, PlanPosition position)
{
    MoveRoomSeparatorEndpointCommand value;
    SiteHelperCommand command;
    assert(move_room_separator_endpoint_command_create(id, endpoint, position, &value));
    assert(sitehelper_command_from_move_room_separator_endpoint(&value, &command));
    return command;
}
static void check(const SiteHelperProject *project, DomainId id, PlanSegment segment, DomainId next)
{
    const RoomSeparator *separator = build_find_room_separator_by_id_const(&project->storeys[0].structure, id);
    assert(separator && separator->id == id);
    assert(separator->segment.start.x == segment.start.x && separator->segment.start.y == segment.start.y);
    assert(separator->segment.end.x == segment.end.x && separator->segment.end.y == segment.end.y);
    assert(project->domain_ids.next == next);
    assert(sitehelper_project_validate(project).code == SITEHELPER_PROJECT_VALID);
}

static void test_history_and_independence(void)
{
    SiteHelperProject project, before, deleted;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_command_history_init(&history);
    PlanSegment original = {{5000, 6000}, {1000, 2000}};
    SiteHelperCommand command = add(original);
    SiteHelperCommandResult result;
    DomainId id = project.domain_ids.next;
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    assert(result.type == SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR && result.data.room_separator.separator_id == id);
    assert(project.storeys[0].structure.room_count == 0 && project.storeys[0].structure.wall_count == 0);
    DomainId next = project.domain_ids.next;
    for (int i = 0; i < 4; i++) {
        assert(sitehelper_command_history_undo(&history, &project));
        assert(project.storeys[0].structure.room_separator_count == 0 && project.domain_ids.next == next);
        assert(sitehelper_command_history_redo(&history, &project));
        check(&project, id, original, next);
    }
    OpeningCommand opening;
    assert(opening_command_create(id, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
    DomainId opening_id = 123;
    assert(!opening_command_execute(&project, &opening, &opening_id));
    assert(opening_id == DOMAIN_ID_INVALID && project.domain_ids.next == next);
    assert(!build_find_wall_by_id(&project.storeys[0].structure, id));

    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){-12, 34}));
    DomainId other = sitehelper_project_add_room_separator(&project, project.storeys[0].id, original);
    next = project.domain_ids.next;
    test_clone_project_authoritative(&project, &before);
    PlanSegment end_moved = {original.start, {INT_MIN, INT_MAX}};
    PlanSegment both_moved = {{-100, -200}, end_moved.end};
    command = move(id, ROOM_SEPARATOR_ENDPOINT_END, end_moved.end);
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    check(&project, id, end_moved, next);
    command = move(id, ROOM_SEPARATOR_ENDPOINT_START, both_moved.start);
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    for (int i = 0; i < 4; i++) {
        check(&project, id, both_moved, next);
        assert(sitehelper_command_history_undo(&history, &project));
        check(&project, id, end_moved, next);
        assert(sitehelper_command_history_undo(&history, &project));
        test_assert_project_authoritative_equal(&before, &project);
        assert(sitehelper_command_history_redo(&history, &project));
        assert(sitehelper_command_history_redo(&history, &project));
    }
    sitehelper_project_destroy(&before);
    test_clone_project_authoritative(&project, &before);
    command = deletion(id);
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    test_clone_project_authoritative(&project, &deleted);
    for (int i = 0; i < 4; i++) {
        assert(sitehelper_command_history_undo(&history, &project));
        test_assert_project_authoritative_equal(&before, &project);
        assert(project.storeys[0].structure.room_separators[0].id == id && project.storeys[0].structure.room_separators[1].id == other);
        assert(sitehelper_command_history_redo(&history, &project));
        test_assert_project_authoritative_equal(&deleted, &project);
    }
    /* A live claim of the saved ID prevents restoration, leaving history retryable. */
    assert(build_add_room(&project.storeys[0].structure, id));
    size_t cursor = history.cursor;
    assert(!sitehelper_command_history_undo(&history, &project));
    assert(history.cursor == cursor && project.domain_ids.next == next);
    project.storeys[0].structure.room_count--; /* Remove the injected independent ID claim. */
    assert(sitehelper_command_history_undo(&history, &project));
    test_assert_project_authoritative_equal(&before, &project);
    RoomSeparator restored = project.storeys[0].structure.room_separators[0];
    assert(sitehelper_project_remove_room_separator_by_id(&project, id));
    cursor = history.cursor;
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(history.cursor == cursor && project.domain_ids.next == next);
    assert(build_insert_room_separator(&project.storeys[0].structure, &restored, 0));
    test_assert_project_authoritative_equal(&before, &project);
    /* Discard deletion's redo snapshot, preserving independent room placement. */
    command = move(other, ROOM_SEPARATOR_ENDPOINT_END, (PlanPosition){0, 0});
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(build_find_room_by_id(&project.storeys[0].structure, room)->has_location);
    assert(build_find_room_by_id(&project.storeys[0].structure, room)->location.x == -12);
    assert(build_find_room_by_id(&project.storeys[0].structure, room)->location.y == 34);
    /* History and separator array relocation cannot invalidate captured geometry. */
    for (int i = 0; i < 20; i++) {
        command = add((PlanSegment){{i, i}, {i + 1, i}});
        assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    }
    next = project.domain_ids.next;
    for (int i = 0; i < 20; i++) { assert(sitehelper_command_history_undo(&history, &project)); }
    check(&project, id, both_moved, next);
    sitehelper_project_destroy(&before);
    sitehelper_project_destroy(&deleted);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_physical_command_restoration_rejects_separator_id_collisions(void)
{
    SiteHelperProject project;
    SiteHelperCommandHistory history;
    SiteHelperCommandResult result;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_command_history_init(&history);
    WallCommand wall_command;
    SiteHelperCommand wall_add;
    assert(wall_command_create(1, (WallPlanSegment){{0, 0}, {6000, 0}}, &wall_command));
    assert(sitehelper_command_from_wall(&wall_command, &wall_add));
    assert(sitehelper_command_history_execute(&history, &project, &wall_add, &result));
    DomainId wall_id = result.data.add_wall.wall_id;
    assert(sitehelper_command_history_undo(&history, &project));
    RoomSeparator claim = {.id = wall_id, .segment = {{0, 1}, {2, 3}}};
    assert(build_insert_room_separator(&project.storeys[0].structure, &claim, 0));
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(history.cursor == 0 && project.storeys[0].structure.wall_count == 0);
    assert(sitehelper_project_remove_room_separator_by_id(&project, wall_id));
    assert(sitehelper_command_history_redo(&history, &project));
    OpeningCommand opening;
    SiteHelperCommand add_opening;
    assert(opening_command_create(wall_id, OPENING_WINDOW, 1200, 900, 800, 1000, &opening));
    assert(sitehelper_command_from_opening(&opening, &add_opening));
    assert(sitehelper_command_history_execute(&history, &project, &add_opening, &result));
    DomainId opening_id = result.data.add_opening.opening_id;
    assert(sitehelper_command_history_undo(&history, &project));
    claim.id = opening_id;
    assert(build_insert_room_separator(&project.storeys[0].structure, &claim, 0));
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(history.cursor == 1 && project.storeys[0].structure.walls[0].definition.opening_count == 0);
    assert(sitehelper_project_remove_room_separator_by_id(&project, opening_id));
    assert(sitehelper_command_history_redo(&history, &project));
    DeleteWallCommand deletion;
    SiteHelperCommand delete_wall;
    assert(delete_wall_command_create(wall_id, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &delete_wall));
    assert(sitehelper_command_history_execute(&history, &project, &delete_wall, &result));
    DomainId next = project.domain_ids.next;
    for (int i = 0; i < 2; i++) {
        claim.id = i == 0 ? wall_id : opening_id;
        assert(build_insert_room_separator(&project.storeys[0].structure, &claim, 0));
        assert(!sitehelper_command_history_undo(&history, &project));
        assert(history.cursor == 3 && project.storeys[0].structure.wall_count == 0);
        assert(project.domain_ids.next == next);
        assert(sitehelper_project_remove_room_separator_by_id(&project, claim.id));
    }
    assert(sitehelper_command_history_undo(&history, &project));
    assert(wall_find_opening_by_id_const(&project.storeys[0].structure.walls[0], opening_id));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_invalid_mutations_and_failed_move_history(void)
{
    SiteHelperProject project, before;
    SiteHelperCommandHistory history;
    SiteHelperCommandResult result;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_command_history_init(&history);
    PlanSegment segment = {{1000, 2000}, {4000, 6000}};
    DomainId id = sitehelper_project_add_room_separator(&project, project.storeys[0].id, segment);
    DomainId next = project.domain_ids.next;
    SiteHelperCommand command = move(id, ROOM_SEPARATOR_ENDPOINT_START, (PlanPosition){0});
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    RoomSeparator saved = *build_find_room_separator_by_id(&project.storeys[0].structure, id);
    assert(sitehelper_project_remove_room_separator_by_id(&project, id));
    assert(!sitehelper_command_history_undo(&history, &project));
    assert(history.cursor == 1 && project.domain_ids.next == next);
    assert(build_insert_room_separator(&project.storeys[0].structure, &saved, 0));
    assert(sitehelper_command_history_undo(&history, &project));
    saved = *build_find_room_separator_by_id(&project.storeys[0].structure, id);
    assert(sitehelper_project_remove_room_separator_by_id(&project, id));
    assert(!sitehelper_command_history_redo(&history, &project));
    assert(history.cursor == 0 && project.domain_ids.next == next);
    assert(build_insert_room_separator(&project.storeys[0].structure, &saved, 0));
    test_clone_project_authoritative(&project, &before);
    SiteHelperCommand invalid[] = {
        {.type = SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR, .data.add_room_separator.segment = {{0, 0}, {0, 0}}},
        move(id, ROOM_SEPARATOR_ENDPOINT_END, segment.start),
        {.type = SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT, .data.move_room_separator_endpoint = {id, (RoomSeparatorEndpoint)99, {0, 0}}},
        {.type = SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR, .data.delete_room_separator.separator_id = 0},
        deletion(999), move(999, ROOM_SEPARATOR_ENDPOINT_START, (PlanPosition){0})
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(!sitehelper_command_history_execute(&history, &project, &invalid[i], &result));
        assert(result.type == SITEHELPER_COMMAND_NONE && history.cursor == 0 && history.count == 1);
        test_assert_project_authoritative_equal(&before, &project);
    }
    assert(sitehelper_command_history_redo(&history, &project));
    AddRoomSeparatorCommand a;
    DeleteRoomSeparatorCommand d;
    MoveRoomSeparatorEndpointCommand m;
    assert(!add_room_separator_command_create(1, (PlanSegment){0}, &a));
    assert(!add_room_separator_command_create(1, segment, NULL));
    assert(!delete_room_separator_command_create(0, &d));
    assert(!delete_room_separator_command_create(id, NULL));
    assert(!move_room_separator_endpoint_command_create(id, (RoomSeparatorEndpoint)99, (PlanPosition){0}, &m));
    assert(!move_room_separator_endpoint_command_create(0, ROOM_SEPARATOR_ENDPOINT_END, (PlanPosition){0}, &m));
    assert(!move_room_separator_endpoint_command_create(id, ROOM_SEPARATOR_ENDPOINT_END, (PlanPosition){0}, NULL));
    assert(!sitehelper_command_from_add_room_separator(NULL, &command));
    assert(!sitehelper_command_from_delete_room_separator(NULL, &command));
    assert(!sitehelper_command_from_move_room_separator_endpoint(NULL, &command));
    sitehelper_project_destroy(&before);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    PlanSegment segment = {{5000, 6000}, {1000, 2000}};
    for (size_t fail_at = 0; fail_at < 2; fail_at++) {
        SiteHelperProject project;
        SiteHelperCommandHistory history;
        sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
        sitehelper_command_history_init(&history);
        SiteHelperCommand command = add(segment);
        SiteHelperCommandResult result;
        DomainId next = project.domain_ids.next;
        allocations_before_failure = fail_at; /* History reserve then collection growth. */
        assert(!sitehelper_command_history_execute(&history, &project, &command, &result));
        allocations_before_failure = SIZE_MAX;
        assert(project.storeys[0].structure.room_separator_count == 0 && project.domain_ids.next == next);
        assert(history.cursor == 0 && history.count == 0);
        assert(sitehelper_command_history_execute(&history, &project, &command, &result));
        DomainId id = result.data.room_separator.separator_id;
        next = project.domain_ids.next;
        SiteHelperCommand removal = deletion(id);
        allocations_before_failure = 0;
        assert(!sitehelper_command_history_execute(&history, &project, &removal, &result));
        allocations_before_failure = SIZE_MAX;
        check(&project, id, segment, next);
        assert(sitehelper_command_history_execute(&history, &project, &removal, &result));
        assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, segment)); /* Fill retained capacity. */
        next = project.domain_ids.next;
        allocations_before_failure = 0;
        assert(!sitehelper_command_history_undo(&history, &project));
        allocations_before_failure = SIZE_MAX;
        assert(history.cursor == 2 && project.storeys[0].structure.room_separator_count == 1);
        assert(project.domain_ids.next == next);
        assert(sitehelper_command_history_undo(&history, &project));
        check(&project, id, segment, next);
        allocations_before_failure = 0;
        assert(sitehelper_command_history_redo(&history, &project)); /* Deletion allocates nothing. */
        assert(allocations_before_failure == 0);
        allocations_before_failure = SIZE_MAX;
        sitehelper_command_history_destroy(&history);
        sitehelper_project_destroy(&project);
    }
    SiteHelperProject project;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    sitehelper_command_history_init(&history);
    SiteHelperCommand command = add(segment);
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    DomainId id = result.data.room_separator.separator_id, next = project.domain_ids.next;
    assert(sitehelper_command_history_undo(&history, &project));
    free(project.storeys[0].structure.room_separators); /* Valid empty collection forces redo to allocate. */
    project.storeys[0].structure.room_separators = NULL;
    project.storeys[0].structure.room_separator_capacity = 0;
    allocations_before_failure = 0;
    assert(!sitehelper_command_history_redo(&history, &project));
    allocations_before_failure = SIZE_MAX;
    assert(history.cursor == 0 && project.storeys[0].structure.room_separator_count == 0 && project.domain_ids.next == next);
    assert(sitehelper_command_history_redo(&history, &project));
    command = move(id, ROOM_SEPARATOR_ENDPOINT_END, (PlanPosition){0});
    allocations_before_failure = 0;
    assert(!sitehelper_command_history_execute(&history, &project, &command, &result));
    allocations_before_failure = SIZE_MAX;
    check(&project, id, segment, next);
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    allocations_before_failure = 0;
    assert(sitehelper_command_history_undo(&history, &project));
    assert(sitehelper_command_history_redo(&history, &project));
    assert(allocations_before_failure == 0);
    allocations_before_failure = SIZE_MAX;
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}
#endif

int main(void)
{
    test_physical_command_restoration_rejects_separator_id_collisions();
    test_history_and_independence();
    test_invalid_mutations_and_failed_move_history();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("room separator command tests passed");
    return 0;
}
