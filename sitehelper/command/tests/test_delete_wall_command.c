#include <assert.h>
#include <stdio.h>

#include "command_history.h"
#include "test_support.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
/* Fail exactly one allocation while an operation is armed. Wrappers are local
 * to this test executable; project/library APIs remain unchanged. */
static size_t allocations_before_failure = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);

static int fail_allocation(void)
{
    if (allocations_before_failure == SIZE_MAX) {
        return 0;
    }
    if (allocations_before_failure-- == 0) {
        allocation_failed = 1;
        return 1;
    }
    return 0;
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

typedef struct {
    SiteHelperProject project;
    SiteHelperProject original;
    SiteHelperCommandHistory history;
    DomainId room_a, room_b, room_c, wall_id, control_id;
    SiteHelperCommand command;
} Fixture;

static void fixture_init(Fixture *f)
{
    *f = (Fixture){0};
    sitehelper_project_init(&f->project);
    sitehelper_command_history_init(&f->history);
    f->room_a = sitehelper_project_add_room(&f->project);
    f->room_b = sitehelper_project_add_room(&f->project);
    f->room_c = sitehelper_project_add_room(&f->project);
    f->wall_id = sitehelper_project_add_wall(&f->project,
        (WallPlanSegment){{4600, 6800}, {1000, 2000}});
    f->control_id = sitehelper_project_add_wall(&f->project,
        (WallPlanSegment){{-6000, -1000}, {0, -1000}});
    assert(f->wall_id && f->control_id);
    Wall *wall = build_find_wall_by_id(&f->project.structure, f->wall_id);
    Opening openings[] = {
        {.id = domain_id_generate(&f->project.domain_ids), .type = OPENING_DOOR,
         .frame_position = 500, .width = 800, .height = 2000},
        {.id = domain_id_generate(&f->project.domain_ids), .type = OPENING_WINDOW,
         .frame_position = 3000, .frame_bottom = 900, .width = 1000, .height = 1000,
         .width_allowance = 12, .height_allowance = 15, .custom_allowance = true}
    };
    for (size_t i = 0; i < 2; i++) {
        assert(wall_add_opening_definition(wall, &f->project.settings, &openings[i]));
    }
    assert(wall_generate(wall, &f->project.settings));
    Wall *control = build_find_wall_by_id(&f->project.structure, f->control_id);
    assert(wall_add_opening(control, &f->project.settings,
        domain_id_generate(&f->project.domain_ids), OPENING_WINDOW, 1500, 900, 1000, 1000));
    assert(wall_generate(control, &f->project.settings));
    test_clone_project_authoritative(&f->project, &f->original);
    DeleteWallCommand deletion;
    assert(delete_wall_command_create(f->wall_id, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &f->command));
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_project_destroy(&f->project); /* History owns independent copies. */
    sitehelper_command_history_destroy(&f->history);
    sitehelper_command_history_destroy(&f->history);
    sitehelper_project_destroy(&f->original);
}

static void assert_deleted(Fixture *f)
{
    assert(!build_find_wall_by_id(&f->project.structure, f->wall_id));
    assert(f->project.structure.wall_count == 1);
    assert(f->project.domain_ids.next == f->original.domain_ids.next);
    assert(f->project.structure.room_count == f->original.structure.room_count);
    for (size_t i = 0; i < f->project.structure.room_count; i++) {
        assert(f->project.structure.rooms[i].id == f->original.structure.rooms[i].id);
    }
    test_assert_wall_definition_equal(
        build_find_wall_by_id(&f->original.structure, f->control_id),
        build_find_wall_by_id(&f->project.structure, f->control_id));
}

static void execute_delete(Fixture *f)
{
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&f->history, &f->project, &f->command, &result));
    assert(result.type == SITEHELPER_COMMAND_DELETE_WALL);
    assert(result.data.delete_wall.wall_id == f->wall_id);
    assert_deleted(f);
}

static void test_delete_undo_redo_identity_and_independent_rooms(void)
{
    Fixture f;
    fixture_init(&f);
    execute_delete(&f);
    for (int cycle = 0; cycle < 5; cycle++) {
        assert(sitehelper_command_history_undo(&f.history, &f.project));
        test_assert_project_authoritative_equal(&f.original, &f.project);
        assert(f.history.cursor == 0 && f.history.count == 1);
        assert(sitehelper_command_history_redo(&f.history, &f.project));
        assert_deleted(&f);
    }
    fixture_destroy(&f); /* Destroy an applied delete snapshot. */
}

static void test_undo_regenerates_using_current_settings(void)
{
    Fixture f;
    fixture_init(&f);
    execute_delete(&f);
    f.project.settings.stud_height = 2800;
    f.project.settings.stud_spacing = 450;
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    Wall expected;
    test_clone_wall_definition(build_find_wall_by_id(&f.original.structure, f.wall_id), &expected);
    assert(wall_generate(&expected, &f.project.settings));
    Wall *restored = build_find_wall_by_id(&f.project.structure, f.wall_id);
    test_assert_wall_definition_equal(&expected, restored);
    test_assert_framing_semantically_equal(&expected.framing, &restored->framing);
    assert(restored->framing.topplate.position.z == 2800);
    assert(f.project.domain_ids.next == f.original.domain_ids.next);
    wall_destroy(&expected);
    fixture_destroy(&f); /* Destroy an undone delete snapshot. */
}

static void test_invalid_and_missing_delete_preserve_model_and_redo(void)
{
    Fixture f;
    fixture_init(&f);
    DeleteWallCommand deletion;
    assert(!delete_wall_command_create(DOMAIN_ID_INVALID, &deletion));
    assert(!delete_wall_command_create(f.wall_id, NULL));
    assert(!sitehelper_command_from_delete_wall(NULL, &f.command));
    assert(!sitehelper_command_from_delete_wall(&deletion, NULL));
    execute_delete(&f);
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    DomainId invalid_ids[] = {DOMAIN_ID_INVALID, f.project.domain_ids.next + 100};
    for (size_t i = 0; i < 2; i++) {
        SiteHelperCommand invalid = {.type = SITEHELPER_COMMAND_DELETE_WALL,
            .data.delete_wall.wall_id = invalid_ids[i]};
        SiteHelperCommandResult result = {.type = SITEHELPER_COMMAND_ADD_WALL};
        assert(!sitehelper_command_history_execute(&f.history, &f.project, &invalid, &result));
        assert(result.type == SITEHELPER_COMMAND_NONE);
        assert(f.history.count == 1 && f.history.cursor == 0);
        test_assert_project_authoritative_equal(&f.original, &f.project);
        assert(!sitehelper_command_execute(&f.project, &invalid, &result));
        test_assert_project_authoritative_equal(&f.original, &f.project);
    }
    SiteHelperCommandResult result;
    assert(!sitehelper_command_history_execute(NULL, &f.project, &f.command, &result));
    assert(!sitehelper_command_history_execute(&f.history, NULL, &f.command, &result));
    assert(!sitehelper_command_history_execute(&f.history, &f.project, NULL, &result));
    assert(!sitehelper_command_history_execute(&f.history, &f.project, &f.command, NULL));
    assert(sitehelper_command_history_redo(&f.history, &f.project));
    assert_deleted(&f);
    fixture_destroy(&f);
}

static void assert_failed_undo_unchanged(Fixture *f)
{
    SiteHelperProject before;
    test_clone_project_authoritative(&f->project, &before);
    assert(!sitehelper_command_history_undo(&f->history, &f->project));
    assert(f->history.cursor == 1 && f->history.count == 1);
    test_assert_project_authoritative_equal(&before, &f->project);
    sitehelper_project_destroy(&before);
}

static void test_undo_failures_are_transactional_and_retryable(void)
{
    Fixture f;
    fixture_init(&f);
    execute_delete(&f);
    Wall collision = {.id = f.wall_id, .definition.segment = {{0, 0}, {4200, 0}}};
    assert(build_append_wall(&f.project.structure, &collision));
    assert_failed_undo_unchanged(&f);
    assert(build_remove_wall_by_id(&f.project.structure, f.wall_id));

    Opening *control_opening = &build_find_wall_by_id(&f.project.structure,
        f.control_id)->definition.openings[0];
    DomainId saved_id = control_opening->id;
    control_opening->id = build_find_wall_by_id(&f.original.structure,
        f.wall_id)->definition.openings[0].id;
    assert_failed_undo_unchanged(&f); /* Opening identity is already in use. */
    control_opening->id = saved_id;

    f.project.settings.stud_spacing_mode = (StudSpacingMode)999;
    assert_failed_undo_unchanged(&f); /* Generation fails before insertion. */
    f.project.settings = f.original.settings;
    f.project.settings.stud_height = 1000;
    assert_failed_undo_unchanged(&f); /* Opening cannot be restored. */
    f.project.settings = f.original.settings;
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    test_assert_project_authoritative_equal(&f.original, &f.project);
    fixture_destroy(&f);
}

static void test_discard_redo_and_history_reallocation(void)
{
    Fixture f;
    fixture_init(&f);
    execute_delete(&f);
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    SiteHelperCommand add;
    WallCommand wall;
    assert(wall_command_create((WallPlanSegment){{0, 0}, {4200, 0}}, &wall));
    assert(sitehelper_command_from_wall(&wall, &add));
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&f.history, &f.project, &add, &result));
    assert(f.history.count == 1 && f.history.cursor == 1);
    assert(!sitehelper_command_history_redo(&f.history, &f.project));
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    assert(sitehelper_command_history_redo(&f.history, &f.project));
    /* Keep a snapshot while reallocating the entry array beyond capacity 8. */
    assert(sitehelper_command_history_execute(&f.history, &f.project, &f.command, &result));
    for (int i = 0; i < 12; i++) {
        assert(sitehelper_command_history_execute(&f.history, &f.project, &add, &result));
    }
    for (int i = 0; i < 13; i++) {
        assert(sitehelper_command_history_undo(&f.history, &f.project));
    }
    test_assert_wall_definition_equal(build_find_wall_by_id(&f.original.structure, f.wall_id),
        build_find_wall_by_id(&f.project.structure, f.wall_id));
    fixture_destroy(&f);
}

static void test_redo_preserves_original_snapshot_and_failed_redo(void)
{
    Fixture f;
    fixture_init(&f);
    execute_delete(&f);
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    Wall *wall = build_find_wall_by_id(&f.project.structure, f.wall_id);
    assert(wall_set_plan_segment(wall, (WallPlanSegment){{0, 0}, {6000, 0}}));
    assert(sitehelper_command_history_redo(&f.history, &f.project));
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    test_assert_project_authoritative_equal(&f.original, &f.project);
    assert(build_remove_wall_by_id(&f.project.structure, f.wall_id));
    assert(!sitehelper_command_history_redo(&f.history, &f.project));
    assert(f.history.cursor == 0 && f.history.count == 1);
    assert_deleted(&f);
    fixture_destroy(&f);
}

static void test_direct_execute(void)
{
    Fixture f;
    fixture_init(&f);
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&f.history, &f.project, &f.command, &result));
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    assert(build_find_wall_by_id(&f.project.structure, f.wall_id));
    assert(sitehelper_command_execute(&f.project, &f.command, &result));
    assert_deleted(&f);
    fixture_destroy(&f);
}

static void test_empty_wall_without_rooms(void)
{
    SiteHelperProject project;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    sitehelper_command_history_init(&history);
    DomainId wall_id = domain_id_generate(&project.domain_ids);
    Wall wall = {.id = wall_id, .definition.segment = {{1000, 2000}, {4600, 6800}}};
    assert(wall_generate(&wall, &project.settings));
    assert(build_append_wall(&project.structure, &wall));
    DomainId next = project.domain_ids.next;
    SiteHelperCommand command = {.type = SITEHELPER_COMMAND_DELETE_WALL,
        .data.delete_wall.wall_id = wall_id};
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history, &project, &command, &result));
    assert(project.structure.wall_count == 0);
    assert(sitehelper_command_history_undo(&history, &project));
    assert(project.structure.wall_count == 1 && project.structure.room_count == 0);
    assert(build_find_wall_by_id(&project.structure, wall_id)->definition.opening_count == 0);
    assert(project.domain_ids.next == next);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures_preserve_execute_and_undo(void)
{
    size_t execute_failures = 0, undo_failures = 0;
    for (size_t fail_at = 0; ; fail_at++) {
        assert(fail_at < 1000);
        Fixture f;
        fixture_init(&f);
        SiteHelperCommandResult result;
        allocation_failed = 0;
        allocations_before_failure = fail_at;
        int success = sitehelper_command_history_execute(
            &f.history, &f.project, &f.command, &result);
        allocations_before_failure = SIZE_MAX;
        if (success) {
            assert(!allocation_failed);
            assert_deleted(&f);
            fixture_destroy(&f);
            break;
        }
        assert(allocation_failed);
        execute_failures++;
        assert(f.history.count == 0 && f.history.cursor == 0);
        assert(result.type == SITEHELPER_COMMAND_NONE);
        test_assert_project_authoritative_equal(&f.original, &f.project);
        execute_delete(&f); /* Capture failure must be retryable. */
        fixture_destroy(&f);
    }
    for (size_t fail_at = 0; ; fail_at++) {
        assert(fail_at < 1000);
        Fixture f;
        fixture_init(&f);
        execute_delete(&f);
        /* Fill retained capacity so restoration must allocate global wall storage. */
        DomainId extra = sitehelper_project_add_wall(&f.project,
            (WallPlanSegment){{0, 0}, {4200, 0}});
        assert(extra != DOMAIN_ID_INVALID);

        assert(f.project.structure.wall_count == f.project.structure.wall_capacity);
        SiteHelperProject before;
        test_clone_project_authoritative(&f.project, &before);
        allocation_failed = 0;
        allocations_before_failure = fail_at;
        int success = sitehelper_command_history_undo(&f.history, &f.project);
        allocations_before_failure = SIZE_MAX;
        if (!success) {
            assert(allocation_failed);
            undo_failures++;
            assert(f.history.cursor == 1 && f.history.count == 1);
            test_assert_project_authoritative_equal(&before, &f.project);
            assert(sitehelper_command_history_undo(&f.history, &f.project));
        }
        else {
            assert(!allocation_failed);
        }
        assert(f.project.domain_ids.next == before.domain_ids.next);
        test_assert_wall_definition_equal(build_find_wall_by_id(&f.original.structure, f.wall_id),
            build_find_wall_by_id(&f.project.structure, f.wall_id));
        sitehelper_project_destroy(&before);
        fixture_destroy(&f);
        if (success) {
            break;
        }
    }
    assert(execute_failures >= 3); /* History, state, openings. */
    assert(undo_failures > 3); /* Definitions/framing and global insertion. */
    printf("allocation failures checked: execute=%zu, undo=%zu\n",
        execute_failures, undo_failures);
}
#endif

int main(void)
{
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures_preserve_execute_and_undo();
#endif
    test_empty_wall_without_rooms();
    test_delete_undo_redo_identity_and_independent_rooms();
    test_undo_regenerates_using_current_settings();
    test_invalid_and_missing_delete_preserve_model_and_redo();
    test_undo_failures_are_transactional_and_retryable();
    test_discard_redo_and_history_reallocation();
    test_redo_preserves_original_snapshot_and_failed_redo();
    test_direct_execute();
    puts("delete wall command tests passed");
    return 0;
}
