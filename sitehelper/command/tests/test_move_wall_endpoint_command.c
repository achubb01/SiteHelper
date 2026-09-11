#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "command_history.h"
#include "test_support.h"
#include "wall_plan_transform.h"
#include "sitehelper_persistence.h"

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
    SiteHelperProject project, original;
    SiteHelperCommandHistory history;
    DomainId room_a, room_b, wall_id, control_id;
} Fixture;

static void clone_project(const SiteHelperProject *source, SiteHelperProject *copy)
{
    test_clone_project_authoritative(source, copy);
    for (size_t i = 0; i < copy->storeys[0].structure.wall_count; i++) {
        assert(wall_generate(&copy->storeys[0].structure.walls[i], &copy->settings));
    }
}

static void assert_project_equal(const SiteHelperProject *expected,
    const SiteHelperProject *actual)
{
    test_assert_project_authoritative_equal(expected, actual);
    assert(sitehelper_project_validate(actual).code == SITEHELPER_PROJECT_VALID);
    for (size_t i = 0; i < expected->storeys[0].structure.wall_count; i++) {
        const Wall *wall = &expected->storeys[0].structure.walls[i];
        test_assert_framing_semantically_equal(&wall->framing,
            &build_find_wall_by_id_const(&actual->storeys[0].structure, wall->id)->framing);
    }
}

static void fixture_init(Fixture *f)
{
    *f = (Fixture){0};
    sitehelper_project_init(&f->project);
    assert(sitehelper_project_add_storey(&f->project, 0));
    sitehelper_command_history_init(&f->history);
    f->room_a = sitehelper_project_add_room(&f->project, f->project.storeys[0].id);
    f->room_b = sitehelper_project_add_room(&f->project, f->project.storeys[0].id);
    f->wall_id = sitehelper_project_add_wall(&f->project, f->project.storeys[0].id,
        (WallPlanSegment){{1000, 2000}, {5000, 2000}});
    f->control_id = sitehelper_project_add_wall(&f->project, f->project.storeys[0].id,
        (WallPlanSegment){{-6000, -1000}, {0, -1000}});
    assert(f->wall_id && f->control_id);

    Wall *wall = build_find_wall_by_id(&f->project.storeys[0].structure, f->wall_id);
    Opening openings[] = {
        {.id = domain_id_generate(&f->project.domain_ids), .type = OPENING_DOOR,
         .frame_position = 500, .width = 800, .height = 2000},
        {.id = domain_id_generate(&f->project.domain_ids), .type = OPENING_WINDOW,
         .frame_position = 2800, .frame_bottom = 900, .width = 800, .height = 1000,
         .width_allowance = 12, .height_allowance = 15, .custom_allowance = true}
    };
    for (size_t i = 0; i < 2; i++) {
        assert(wall_add_opening_definition(wall, &f->project.settings, &openings[i]));
    }
    assert(wall_generate(wall, &f->project.settings));
    Wall *control = build_find_wall_by_id(&f->project.storeys[0].structure, f->control_id);
    assert(wall_add_opening(control, &f->project.settings,
        domain_id_generate(&f->project.domain_ids), OPENING_WINDOW, 1500, 900, 1000, 1000));
    assert(wall_generate(control, &f->project.settings));
    clone_project(&f->project, &f->original);
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_project_destroy(&f->project);
    sitehelper_command_history_destroy(&f->history);
    sitehelper_command_history_destroy(&f->history);
    sitehelper_project_destroy(&f->original);
}

static SiteHelperCommand move_command(Fixture *f, WallEndpoint endpoint, PlanPosition position)
{
    MoveWallEndpointCommand move;
    SiteHelperCommand command;
    assert(move_wall_endpoint_command_create(f->wall_id, endpoint, position, &move));
    assert(sitehelper_command_from_move_wall_endpoint(&move, &command));
    return command;
}

static void execute(Fixture *f, SiteHelperCommand command)
{
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&f->history, &f->project, &command, &result));
    assert(result.type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT);
    assert(result.data.move_wall_endpoint.wall_id == f->wall_id);
    assert(f->project.domain_ids.next == f->original.domain_ids.next);
}

static void assert_geometry(Fixture *f, WallPlanSegment segment, int length)
{
    SiteHelperProject expected;
    clone_project(&f->original, &expected);
    Wall *wall = build_find_wall_by_id(&expected.storeys[0].structure, f->wall_id);
    assert(wall_set_plan_segment(wall, segment));
    assert(wall_generate(wall, &expected.settings));
    assert_project_equal(&expected, &f->project);
    wall = build_find_wall_by_id(&f->project.storeys[0].structure, f->wall_id);
    assert(wall_length_mm(wall) == length);
    assert(wall->framing.topplate.length == length);
    assert(wall->framing.bottomplate.length == length);
    PlanPoint point;
    assert(wall_plan_segment_u_to_plan(wall->definition.segment, 0, &point));
    assert(point.x == segment.start.x && point.y == segment.start.y);
    assert(wall_plan_segment_u_to_plan(wall->definition.segment, length, &point));
    assert(point.x == segment.end.x && point.y == segment.end.y);
    sitehelper_project_destroy(&expected);
}

static void test_spatial_moves_and_repeated_history(void)
{
    const struct {
        WallEndpoint endpoint;
        PlanPosition position;
        WallPlanSegment segment;
        int length;
    } cases[] = {
        {WALL_ENDPOINT_END, {4000, 6000}, {{1000, 2000}, {4000, 6000}}, 5000},
        {WALL_ENDPOINT_START, {9000, 2000}, {{9000, 2000}, {5000, 2000}}, 4000},
        {WALL_ENDPOINT_END, {1000, 6000}, {{1000, 2000}, {1000, 6000}}, 4000},
        {WALL_ENDPOINT_END, {8000, 2000}, {{1000, 2000}, {8000, 2000}}, 7000},
        {WALL_ENDPOINT_END, {4800, 2000}, {{1000, 2000}, {4800, 2000}}, 3800}
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        Fixture f;
        fixture_init(&f);
        execute(&f, move_command(&f, cases[i].endpoint, cases[i].position));
        assert_geometry(&f, cases[i].segment, cases[i].length);
        Wall *wall = build_find_wall_by_id(&f.project.storeys[0].structure, f.wall_id);
        const Wall *original = build_find_wall_by_id(&f.original.storeys[0].structure, f.wall_id);
        if (cases[i].length == 4000) {
            test_assert_framing_semantically_equal(&original->framing, &wall->framing);
        }
        if (i == 0) {
            PlanPoint point;
            double u;
            assert(wall_plan_segment_u_to_plan(wall->definition.segment, 2500, &point));
            assert(point.x == 2500 && point.y == 4000);
            assert(wall_plan_segment_plan_to_u(wall->definition.segment, point, &u));
            assert(u == 2500);
        }
        if (cases[i].length == 7000) {
            assert(wall->framing.stud_count > original->framing.stud_count);
        }
        for (int cycle = 0; cycle < 5; cycle++) {
            assert(sitehelper_command_history_undo(&f.history, &f.project));
            assert(f.history.cursor == 0 && f.history.count == 1);
            assert_project_equal(&f.original, &f.project);
            assert(sitehelper_command_history_redo(&f.history, &f.project));
            assert(f.history.cursor == 1 && f.history.count == 1);
            assert_geometry(&f, cases[i].segment, cases[i].length);
        }
        fixture_destroy(&f);
    }
}

static void test_invalid_moves_preserve_model_framing_and_redo(void)
{
    Fixture f;
    fixture_init(&f);
    execute(&f, move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000}));
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    SiteHelperCommand valid = move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000});
    /* This length still fits generated opening members, but violates the
     * authoritative right-end clearance. Generation alone is insufficient. */
    const Wall *wall = build_find_wall_by_id(&f.project.storeys[0].structure, f.wall_id);
    Wall candidate = {.id = wall->id, .definition = wall->definition};
    assert(wall_set_plan_segment(&candidate, (WallPlanSegment){{1000, 2000}, {4700, 2000}}));
    const Opening *opening = &wall->definition.openings[1];
    WallOpeningProposal proposal = {
        .type = opening->type, .frame_position = opening->frame_position,
        .frame_bottom = opening->frame_bottom, .width = opening->width,
        .height = opening->height, .width_allowance = opening->width_allowance,
        .height_allowance = opening->height_allowance, .custom_allowance = opening->custom_allowance
    };
    candidate.definition.opening_count = 1;
    assert(wall_validate_opening(&candidate, &f.project.settings, &proposal).code
        == WALL_OPENING_TOO_CLOSE_TO_RIGHT_END);
    candidate.definition.opening_count = wall->definition.opening_count;
    assert(wall_generate(&candidate, &f.project.settings));
    wall_framing_destroy(&candidate.framing); /* Openings are borrowed. */

    SiteHelperCommand invalid[] = {valid, valid, valid, valid, valid, valid};
    invalid[0].data.move_wall_endpoint.new_position = (PlanPosition){4700, 2000};
    invalid[1].data.move_wall_endpoint.new_position = (PlanPosition){1000, 2000};
    invalid[2].data.move_wall_endpoint.new_position = (PlanPosition){INT_MIN, INT_MAX};
    invalid[3].data.move_wall_endpoint.wall_id = DOMAIN_ID_INVALID;
    invalid[4].data.move_wall_endpoint.wall_id = f.project.domain_ids.next + 100;
    invalid[5].data.move_wall_endpoint.endpoint = (WallEndpoint)999;
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        SiteHelperCommandResult result = {.type = SITEHELPER_COMMAND_ADD_WALL};
        assert(!sitehelper_command_history_execute(&f.history, &f.project, &invalid[i], &result));
        assert(result.type == SITEHELPER_COMMAND_NONE);
        assert(f.history.cursor == 0 && f.history.count == 1);
        assert_project_equal(&f.original, &f.project);
        assert(!sitehelper_command_execute(&f.project, &invalid[i], &result));
        assert(result.type == SITEHELPER_COMMAND_NONE);
        assert_project_equal(&f.original, &f.project);
    }
    assert(sitehelper_command_history_redo(&f.history, &f.project));
    assert_geometry(&f, (WallPlanSegment){{1000, 2000}, {4000, 6000}}, 5000);
    fixture_destroy(&f);
}

static void test_construction_and_direct_execution(void)
{
    Fixture f;
    fixture_init(&f);
    MoveWallEndpointCommand move;
    assert(!move_wall_endpoint_command_create(DOMAIN_ID_INVALID, WALL_ENDPOINT_END,
        (PlanPosition){0}, &move));
    assert(!move_wall_endpoint_command_create(f.wall_id, (WallEndpoint)-1,
        (PlanPosition){0}, &move));
    assert(!move_wall_endpoint_command_create(f.wall_id, WALL_ENDPOINT_END,
        (PlanPosition){0}, NULL));
    SiteHelperCommand command = move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000});
    assert(!sitehelper_command_from_move_wall_endpoint(NULL, &command));
    assert(!sitehelper_command_from_move_wall_endpoint(&command.data.move_wall_endpoint, NULL));
    assert(!move_wall_endpoint_command_execute(NULL, &command.data.move_wall_endpoint));
    assert(!move_wall_endpoint_command_execute(&f.project, NULL));
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&f.project, &command, &result));
    assert_geometry(&f, (WallPlanSegment){{1000, 2000}, {4000, 6000}}, 5000);
    assert(!sitehelper_command_undo(&f.project, &command, &result)); /* Needs history. */
    result.data.move_wall_endpoint.wall_id = f.control_id;
    assert(!sitehelper_command_redo(&f.project, &command, &result));
    assert_geometry(&f, (WallPlanSegment){{1000, 2000}, {4000, 6000}}, 5000);
    fixture_destroy(&f);
}

/* Same operation driver for exhaustive allocation and non-allocation failures. */
static int perform(Fixture *f, int stage, const SiteHelperCommand *command)
{
    SiteHelperCommandResult result = {.type = SITEHELPER_COMMAND_ADD_WALL};
    if (stage == 0) {
        int success = sitehelper_command_history_execute(&f->history, &f->project, command, &result);
        assert(result.type == (success ? SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT : SITEHELPER_COMMAND_NONE));
        return success;
    }
    return stage == 1 ? sitehelper_command_history_undo(&f->history, &f->project)
                      : sitehelper_command_history_redo(&f->history, &f->project);
}

static void prepare_stage(Fixture *f, int stage, SiteHelperCommand command)
{
    if (stage > 0) {
        execute(f, command);
    }
    if (stage == 2) {
        assert(sitehelper_command_history_undo(&f->history, &f->project));
    }
}

static void test_generation_failures_and_retry(void)
{
    for (int stage = 0; stage < 3; stage++) {
        Fixture f;
        fixture_init(&f);
        SiteHelperCommand command = move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000});
        prepare_stage(&f, stage, command);
        SiteHelperProject before;
        clone_project(&f.project, &before);
        size_t cursor = f.history.cursor, count = f.history.count;
        SiteHelperCommandUndoState *state = count ? f.history.entries[0].undo_state : NULL;
        f.project.settings.stud_spacing_mode = (StudSpacingMode)999;
        assert(!perform(&f, stage, &command));
        f.project.settings = before.settings;
        assert_project_equal(&before, &f.project);
        assert(f.history.cursor == cursor && f.history.count == count);
        if (count) {
            assert(f.history.entries[0].undo_state == state);
        }
        /* The same normal opening validation also applies to undo and redo. */
        f.project.settings.stud_height = 1000;
        assert(!perform(&f, stage, &command));
        f.project.settings = before.settings;
        assert_project_equal(&before, &f.project);
        assert(f.history.cursor == cursor && f.history.count == count);
        assert(perform(&f, stage, &command));
        if (stage == 1) {
            assert_project_equal(&f.original, &f.project);
        }
        else {
            assert_geometry(&f, (WallPlanSegment){{1000, 2000}, {4000, 6000}}, 5000);
        }
        sitehelper_project_destroy(&before);
        fixture_destroy(&f);
    }
}

static void test_branch_discard_and_state_relocation(void)
{
    Fixture f;
    fixture_init(&f);
    execute(&f, move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000}));
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    SiteHelperCommand deletion = {.type = SITEHELPER_COMMAND_DELETE_WALL,
        .data.delete_wall.wall_id = f.control_id};
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&f.history, &f.project, &deletion, &result));
    assert(f.history.cursor == 1 && f.history.count == 1);
    assert(!sitehelper_command_history_redo(&f.history, &f.project));
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    assert_project_equal(&f.original, &f.project);
    for (int i = 0; i < 12; i++) {
        execute(&f, move_command(&f, WALL_ENDPOINT_END, (PlanPosition){6000 + i * 100, 2000}));
    }
    assert(f.history.count == 12 && f.history.cursor == 12);
    for (int i = 0; i < 12; i++) {
        assert(sitehelper_command_history_undo(&f.history, &f.project));
        assert(f.project.domain_ids.next == f.original.domain_ids.next);
    }
    assert_project_equal(&f.original, &f.project);
    for (int i = 0; i < 12; i++) {
        assert(sitehelper_command_history_redo(&f.history, &f.project));
        assert_geometry(&f, (WallPlanSegment){{1000, 2000}, {6000 + i * 100, 2000}}, 5000 + i * 100);
    }
    fixture_destroy(&f);
}

static void test_undo_regenerates_with_current_settings(void)
{
    Fixture f;
    fixture_init(&f);
    execute(&f, move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000}));
    f.project.settings.stud_spacing = 450;
    assert(sitehelper_command_history_undo(&f.history, &f.project));
    Wall expected;
    test_clone_wall_definition(build_find_wall_by_id(&f.original.storeys[0].structure, f.wall_id), &expected);
    assert(wall_generate(&expected, &f.project.settings));
    Wall *actual = build_find_wall_by_id(&f.project.storeys[0].structure, f.wall_id);
    test_assert_wall_definition_equal(&expected, actual);
    test_assert_framing_semantically_equal(&expected.framing, &actual->framing);
    assert(f.project.domain_ids.next == f.original.domain_ids.next);
    wall_destroy(&expected);
    fixture_destroy(&f);
}

static void test_moved_segment_persists_in_v9(void)
{
    Fixture f;
    fixture_init(&f);
    execute(&f, move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000}));
    const char *path = "sitehelper_move_wall_endpoint_round_trip.txt";
    assert(sitehelper_project_save_file(&f.project, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *file = fopen(path, "r");
    char header[128];
    assert(file && fgets(header, sizeof header, file));
    assert(strcmp(header, "sitehelper_project 9\n") == 0);
    assert(fclose(file) == 0);
    SiteHelperProject loaded;
    sitehelper_project_init(&loaded);
    assert(sitehelper_project_add_storey(&loaded, 0));
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert_project_equal(&f.project, &loaded);
    assert(f.project.domain_ids.next == f.original.domain_ids.next);
    assert(remove(path) == 0);
    sitehelper_project_destroy(&loaded);
    fixture_destroy(&f);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_all_allocation_failures(void)
{
    size_t failures[3] = {0};
    for (int stage = 0; stage < 3; stage++) {
        for (size_t fail_at = 0; ; fail_at++) {
            assert(fail_at < 1000);
            Fixture f;
            fixture_init(&f);
            SiteHelperCommand command = move_command(&f, WALL_ENDPOINT_END, (PlanPosition){4000, 6000});
            prepare_stage(&f, stage, command);
            SiteHelperProject before;
            clone_project(&f.project, &before);
            size_t cursor = f.history.cursor, count = f.history.count;
            SiteHelperCommandUndoState *state = count ? f.history.entries[0].undo_state : NULL;
            allocation_failed = 0;
            allocations_before_failure = fail_at;
            int success = perform(&f, stage, &command);
            allocations_before_failure = SIZE_MAX;
            if (!success) {
                failures[stage]++;
                assert(allocation_failed);
                assert_project_equal(&before, &f.project);
                assert(f.history.cursor == cursor && f.history.count == count);
                if (count) {
                    assert(f.history.entries[0].undo_state == state);
                }
                assert(perform(&f, stage, &command));
            }
            else {
                assert(!allocation_failed);
            }
            if (stage == 1) {
                assert_project_equal(&f.original, &f.project);
            }
            else {
                assert_geometry(&f, (WallPlanSegment){{1000, 2000}, {4000, 6000}}, 5000);
            }
            sitehelper_project_destroy(&before);
            fixture_destroy(&f);
            if (success) {
                break;
            }
        }
        assert(failures[stage] > 3);
    }
    printf("allocation failures checked: execute=%zu, undo=%zu, redo=%zu\n",
        failures[0], failures[1], failures[2]);
}
#endif

int main(void)
{
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_all_allocation_failures();
#endif
    test_construction_and_direct_execution();
    test_generation_failures_and_retry();
    test_branch_discard_and_state_relocation();
    test_undo_regenerates_with_current_settings();
    test_moved_segment_persists_in_v9();
    test_spatial_moves_and_repeated_history();
    test_invalid_moves_preserve_model_framing_and_redo();
    puts("move wall endpoint command tests passed");
    return 0;
}
