#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "command_history.h"
#include "test_support.h"
#include "wall_query.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t allocations_before_failure = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int fail_allocation(void)
{
    if (allocations_before_failure == SIZE_MAX) { return 0; }
    if (allocations_before_failure-- == 0) { allocation_failed = 1; return 1; }
    return 0;
}
void *__wrap_malloc(size_t size) { return fail_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return fail_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *p, size_t size) { return fail_allocation() ? NULL : __real_realloc(p, size); }
#endif

typedef struct
{
    SiteHelperProject project;
    SiteHelperCommandHistory history;
    DomainId storey_id, wall_id, opening_id;
    Opening old, edited;
} Fixture;

static Wall *fixture_wall(Fixture *f)
{
    return sitehelper_project_find_wall_by_id(&f->project, f->wall_id);
}

static void clone_project(const SiteHelperProject *source, SiteHelperProject *copy)
{
    test_clone_project_authoritative(source, copy);
    for (size_t s = 0; s < copy->storey_count; s++) {
        BuildSettings settings;
        assert(sitehelper_project_resolve_storey_build_settings(copy, copy->storeys[s].id, &settings));
        for (size_t w = 0; w < copy->storeys[s].structure.wall_count; w++) {
            assert(wall_generate(&copy->storeys[s].structure.walls[w], &settings));
        }
    }
}

static void assert_equal(const SiteHelperProject *expected, const SiteHelperProject *actual)
{
    test_assert_project_authoritative_equal(expected, actual);
    assert(sitehelper_project_validate(actual).code == SITEHELPER_PROJECT_VALID);
    for (size_t s = 0; s < expected->storey_count; s++) {
        for (size_t w = 0; w < expected->storeys[s].structure.wall_count; w++) {
            const Wall *a = &expected->storeys[s].structure.walls[w];
            const Wall *b = sitehelper_project_find_wall_by_id_const(actual, a->id);
            test_assert_framing_semantically_equal(&a->framing, &b->framing);
            for (size_t i = 0; i < a->definition.opening_count; i++) {
                test_assert_opening_equal(&a->definition.openings[i], &b->definition.openings[i]);
            }
        }
    }
}

static void fixture_init(Fixture *f)
{
    *f = (Fixture){0};
    sitehelper_project_init(&f->project);
    sitehelper_command_history_init(&f->history);
    f->storey_id = sitehelper_project_add_storey(&f->project, 3000);
    assert(sitehelper_project_set_storey_stud_height(&f->project, f->storey_id, 3200));
    f->wall_id = sitehelper_project_add_wall(&f->project, f->storey_id,
        (WallPlanSegment){{5000, 5000}, {1400, 200}});
    f->opening_id = domain_id_generate(&f->project.domain_ids);
    f->old = (Opening){.id = f->opening_id, .type = OPENING_WINDOW,
        .frame_position = 500, .frame_bottom = 600, .width = 800, .height = 1000,
        .custom_allowance = true, .width_allowance = 12, .height_allowance = 15};
    f->edited = (Opening){.id = f->opening_id, .type = OPENING_DOOR,
        .frame_position = 800, .frame_bottom = 0, .width = 1100, .height = 2700,
        .custom_allowance = false, .width_allowance = 30, .height_allowance = 40};
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f->project, f->storey_id, &settings));
    Wall *wall = fixture_wall(f);
    /* Logical order deliberately differs from spatial order. Target is in the middle. */
    assert(wall_add_opening(wall, &settings, domain_id_generate(&f->project.domain_ids),
        OPENING_DOOR, 4500, 0, 800, 2000));
    assert(wall_add_opening_definition(wall, &settings, &f->old));
    assert(wall_add_opening(wall, &settings, domain_id_generate(&f->project.domain_ids),
        OPENING_DOOR, 2800, 0, 800, 2000));
    assert(wall_generate(wall, &settings));
    /* A second Storey's wall must remain entirely unaffected. */
    DomainId other = sitehelper_project_add_storey(&f->project, 0);
    DomainId control = sitehelper_project_add_wall(&f->project, other,
        (WallPlanSegment){{0, 0}, {3000, 0}});
    assert(wall_generate(sitehelper_project_find_wall_by_id(&f->project, control), &f->project.settings));
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_command_history_destroy(&f->history);
    sitehelper_project_destroy(&f->project);
}

static SiteHelperCommand make_command(Fixture *f, Opening definition)
{
    EditOpeningCommand edit;
    SiteHelperCommand command;
    assert(edit_opening_command_create(f->wall_id, f->opening_id, &definition, &edit));
    assert(sitehelper_command_from_edit_opening(&edit, &command));
    return command;
}

static int perform(Fixture *f, int stage, const SiteHelperCommand *command)
{
    if (stage == 1) { return sitehelper_command_history_undo(&f->history, &f->project); }
    if (stage == 2) { return sitehelper_command_history_redo(&f->history, &f->project); }
    SiteHelperCommandResult result = {.type = SITEHELPER_COMMAND_EDIT_OPENING};
    int success = sitehelper_command_history_execute(&f->history, &f->project, command, &result);
    assert(result.type == (success ? SITEHELPER_COMMAND_EDIT_OPENING : SITEHELPER_COMMAND_NONE));
    if (success) {
        assert(result.data.edit_opening.wall_id == f->wall_id);
        assert(result.data.edit_opening.opening_id == f->opening_id);
    }
    return success;
}

static void test_complete_definition_history(void)
{
    Fixture f;
    fixture_init(&f);
    SiteHelperProject original, edited;
    clone_project(&f.project, &original);
    SiteHelperCommand command = make_command(&f, f.edited);
    assert(perform(&f, 0, &command));
    test_assert_opening_equal(&f.edited, &fixture_wall(&f)->definition.openings[1]);
    assert(fixture_wall(&f)->framing.member_count <
        sitehelper_project_find_wall_by_id_const(&original, f.wall_id)->framing.member_count);
    clone_project(&f.project, &edited);
    /* Compare committed framing to independent regeneration. */
    assert_equal(&edited, &f.project);
    for (int i = 0; i < 3; i++) {
        assert(perform(&f, 1, &command));
        assert_equal(&original, &f.project);
        assert(perform(&f, 2, &command));
        assert_equal(&edited, &f.project);
    }
    /* The compact public undo cannot bypass history-owned state. */
    assert(!sitehelper_command_undo(&f.project, &command, &f.history.entries[0].result));
    SiteHelperCommandResult wrong = f.history.entries[0].result;
    wrong.data.edit_opening.opening_id++;
    assert(!sitehelper_command_redo(&f.project, &command, &wrong));
    assert_equal(&edited, &f.project);
    sitehelper_project_destroy(&original);
    sitehelper_project_destroy(&edited);
    fixture_destroy(&f);
}

static void test_zero_cripple_edit_history_and_clear_hit_rectangle(void)
{
    Fixture f;
    fixture_init(&f);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
    Opening boundary=f.old;
    boundary.frame_bottom=settings.stud_width;
    boundary.height=settings.stud_height-2*settings.stud_width;
    boundary.custom_allowance=true;
    boundary.height_allowance=0;
    SiteHelperProject original,edited;
    clone_project(&f.project,&original);
    SiteHelperCommand command=make_command(&f,boundary);
    assert(perform(&f,0,&command));
    test_assert_opening_equal(&boundary,&fixture_wall(&f)->definition.openings[1]);
    clone_project(&f.project,&edited);
    for(int cycle=0;cycle<3;cycle++) {
        const Wall *wall=fixture_wall(&f);
        for(size_t i=0;i<wall->framing.stud_count;i++) {
            assert(wall->framing.studs[i].details.stud.type!=STUD_CRIPPLE);
        }
        assert(wall_find_opening_at_position(wall,&settings,
            (WallLocalPosition){boundary.frame_position,boundary.frame_bottom})==f.opening_id);
        assert(wall_find_opening_at_position(wall,&settings,
            (WallLocalPosition){boundary.frame_position,boundary.frame_bottom-1})==DOMAIN_ID_INVALID);
        assert(wall_find_opening_at_position(wall,&settings,
            (WallLocalPosition){boundary.frame_position,settings.stud_height-settings.stud_width})==f.opening_id);
        assert(wall_find_opening_at_position(wall,&settings,
            (WallLocalPosition){boundary.frame_position,settings.stud_height-settings.stud_width+1})==DOMAIN_ID_INVALID);
        assert(perform(&f,1,&command));
        assert_equal(&original,&f.project);
        assert(perform(&f,2,&command));
        assert_equal(&edited,&f.project);
    }
    sitehelper_project_destroy(&original);
    sitehelper_project_destroy(&edited);
    fixture_destroy(&f);
}

static void assert_allocations_unchanged(const Wall *before, const Wall *after)
{
    assert(before->definition.openings == after->definition.openings);
    assert(before->definition.opening_capacity == after->definition.opening_capacity);
    assert(before->framing.studs == after->framing.studs);
    assert(before->framing.nogs == after->framing.nogs);
    assert(before->framing.members == after->framing.members);
}

static void test_invalid_edits_and_redo_branch(void)
{
    Fixture f;
    fixture_init(&f);
    SiteHelperProject original;
    clone_project(&f.project, &original);
    SiteHelperCommand valid = make_command(&f, f.edited);
    assert(perform(&f, 0, &valid));
    assert(perform(&f, 1, &valid));
    Opening invalid[13];
    for (size_t i = 0; i < 13; i++) { invalid[i] = f.old; }
    invalid[0].frame_position = 2700; /* Overlaps later collection entry. */
    invalid[1].frame_position = 4500; /* Overlaps earlier collection entry. */
    invalid[2].frame_position = 5800;
    invalid[3].frame_position = 0;
    invalid[4].height = 3300;
    invalid[5].width = 0;
    invalid[6].height = -1;
    invalid[7].type = (OpeningType)99;
    invalid[8].frame_bottom = -1;
    invalid[9].width_allowance = INT_MAX;
    invalid[10].height_allowance = -1000;
    invalid[11].frame_position = INT_MAX;
    invalid[12].frame_bottom = 0; /* Rejected by validation: no room for the sill below clear bottom. */
    for (size_t i = 0; i < 13; i++) {
        Wall before = *fixture_wall(&f);
        SiteHelperCommand command = make_command(&f, invalid[i]);
        assert(!perform(&f, 0, &command));
        assert(f.history.cursor == 0 && f.history.count == 1);
        assert_allocations_unchanged(&before, fixture_wall(&f));
        assert_equal(&original, &f.project);
    }
    assert(perform(&f, 2, &valid)); /* Failed executes preserved the redo branch. */
    sitehelper_project_destroy(&original);
    fixture_destroy(&f);
}

static void test_identity_and_domain_contract(void)
{
    Fixture f;
    fixture_init(&f);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
    SiteHelperProject original;
    clone_project(&f.project, &original);
    Opening wrong = f.edited;
    wrong.id++;
    EditOpeningCommand edit;
    assert(!edit_opening_command_create(f.wall_id, f.opening_id, &wrong, &edit));
    assert(!edit_opening_command_create(0, f.opening_id, &f.old, &edit));
    assert(!wall_apply_opening_definition(fixture_wall(&f), &settings, f.opening_id, &wrong));
    assert(!wall_apply_opening_definition(fixture_wall(&f), &settings, 9999, &f.old));
    assert(!wall_apply_opening_definition(NULL, &settings, f.opening_id, &f.old));
    assert(!wall_apply_opening_definition(fixture_wall(&f), NULL, f.opening_id, &f.old));
    assert(!wall_apply_opening_definition(fixture_wall(&f), &settings, f.opening_id, NULL));
    edit = (EditOpeningCommand){f.wall_id, f.opening_id, wrong};
    assert(!edit_opening_command_execute(&f.project, &edit));
    edit = (EditOpeningCommand){9999, f.opening_id, f.old};
    assert(!edit_opening_command_execute(&f.project, &edit));
    /* Alias to the existing definition is safe and does not self-overlap. */
    assert(wall_apply_opening_definition(fixture_wall(&f), &settings, f.opening_id,
        wall_find_opening_by_id_const(fixture_wall(&f), f.opening_id)));
    assert_equal(&original, &f.project);
    /* The effective Storey height is required: this edit exceeds Project default. */
    assert(!wall_apply_opening_definition(fixture_wall(&f), &f.project.settings, f.opening_id, &f.edited));
    assert_equal(&original, &f.project);
    assert(wall_apply_opening_definition(fixture_wall(&f), &settings, f.opening_id, &f.edited));
    test_assert_opening_equal(&f.edited, &fixture_wall(&f)->definition.openings[1]);
    sitehelper_project_destroy(&original);
    fixture_destroy(&f);
}

static void test_failed_undo_redo_use_current_settings(void)
{
    for (int stage = 1; stage <= 2; stage++) {
        Fixture f;
        fixture_init(&f);
        /* Old is taller, edited shorter for undo failure; reverse for redo. */
        Opening target = f.edited;
        if (stage == 1) {
            f.old.height = 2300;
            BuildSettings settings;
            assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
            assert(wall_apply_opening_definition(fixture_wall(&f), &settings, f.opening_id, &f.old));
            target.height = 2100;
        }
        SiteHelperCommand command = make_command(&f, target);
        assert(perform(&f, 0, &command));
        if (stage == 2) { assert(perform(&f, 1, &command)); }
        assert(sitehelper_project_set_storey_stud_height(&f.project, f.storey_id, 2400));
        SiteHelperProject before;
        clone_project(&f.project, &before);
        Wall allocations = *fixture_wall(&f);
        size_t cursor = f.history.cursor;
        assert(!perform(&f, stage, &command));
        assert(f.history.cursor == cursor && f.history.count == 1);
        assert_allocations_unchanged(&allocations, fixture_wall(&f));
        assert_equal(&before, &f.project);
        assert(sitehelper_project_set_storey_stud_height(&f.project, f.storey_id, 3200));
        assert(perform(&f, stage, &command));
        sitehelper_project_destroy(&before);
        fixture_destroy(&f);
    }
}

static void test_history_storage_and_branch_replacement(void)
{
    Fixture f;
    fixture_init(&f);
    SiteHelperProject original;
    clone_project(&f.project, &original);
    for (int i = 0; i < 12; i++) {
        Opening definition = f.old;
        definition.width += i + 1;
        SiteHelperCommand command = make_command(&f, definition);
        assert(perform(&f, 0, &command));
    }
    assert(f.history.count == 12);
    for (int i = 0; i < 12; i++) { assert(perform(&f, 1, NULL)); }
    assert_equal(&original, &f.project);
    for (int i = 0; i < 12; i++) { assert(perform(&f, 2, NULL)); }
    for (int i = 0; i < 12; i++) { assert(perform(&f, 1, NULL)); }
    SiteHelperCommand replacement = make_command(&f, f.edited);
    assert(perform(&f, 0, &replacement));
    assert(f.history.count == 1 && f.history.cursor == 1);
    assert(!perform(&f, 2, NULL));
    assert(perform(&f, 1, NULL));
    assert_equal(&original, &f.project);
    sitehelper_project_destroy(&original);
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
            SiteHelperCommand command = make_command(&f, f.edited);
            if (stage > 0) { assert(perform(&f, 0, &command)); }
            if (stage == 2) { assert(perform(&f, 1, &command)); }
            SiteHelperProject before;
            clone_project(&f.project, &before);
            Wall allocations = *fixture_wall(&f);
            size_t cursor = f.history.cursor, count = f.history.count;
            SiteHelperCommandUndoState *state = count ? f.history.entries[0].undo_state : NULL;
            allocation_failed = 0;
            allocations_before_failure = fail_at;
            int success = perform(&f, stage, &command);
            allocations_before_failure = SIZE_MAX;
            if (!success) {
                assert(allocation_failed);
                failures[stage]++;
                assert_equal(&before, &f.project);
                assert_allocations_unchanged(&allocations, fixture_wall(&f));
                assert(f.history.cursor == cursor && f.history.count == count);
                if (count) { assert(f.history.entries[0].undo_state == state); }
                assert(perform(&f, stage, &command));
            }
            else { assert(!allocation_failed); }
            test_assert_opening_equal(stage == 1 ? &f.old : &f.edited,
                &fixture_wall(&f)->definition.openings[1]);
            sitehelper_project_destroy(&before);
            fixture_destroy(&f);
            if (success) { break; }
        }
        assert(failures[stage] > 3);
    }
    printf("allocation failures checked: execute=%zu, undo=%zu, redo=%zu\n",
        failures[0], failures[1], failures[2]);
}
#endif

int main(void)
{
    test_complete_definition_history();
    test_zero_cripple_edit_history_and_clear_hit_rectangle();
    test_invalid_edits_and_redo_branch();
    test_identity_and_domain_contract();
    test_failed_undo_redo_use_current_settings();
    test_history_storage_and_branch_replacement();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_all_allocation_failures();
#endif
    puts("edit opening command tests passed");
    return 0;
}
