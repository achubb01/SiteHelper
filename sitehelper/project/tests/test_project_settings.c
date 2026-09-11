#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_support.h"
#include "command_history.h"
#include "sitehelper_persistence.h"
#include "sitehelper_editor.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX, allocation_count;
static int failed, reject_all;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int reject(void)
{
    allocation_count++;
    if (reject_all || (fail_after != SIZE_MAX && fail_after-- == 0)) { failed = 1; return 1; }
    return 0;
}
void *__wrap_malloc(size_t size) { return reject() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return reject() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *p, size_t size) { return reject() ? NULL : __real_realloc(p, size); }
#endif

static const WallPlanSegment segment = {{0, 0}, {6000, 0}};

static DomainId add_wall(SiteHelperProject *p, DomainId storey)
{
    WallCommand command;
    DomainId id;
    assert(wall_command_create(storey, segment, &command));
    assert(wall_command_execute(p, &command, &id));
    return id;
}

static int height(const SiteHelperProject *p, DomainId storey)
{
    BuildSettings resolved;
    assert(sitehelper_project_resolve_storey_build_settings(p, storey, &resolved));
    return resolved.stud_height;
}

static void framing_height(const SiteHelperProject *p, DomainId id, int expected)
{
    const Wall *wall = sitehelper_project_find_wall_by_id_const(p, id);
    assert(wall && wall->framing.stud_count > 0);
    assert(wall->framing.topplate.position.z == expected);
    assert(wall->framing.studs[0].length == expected);
}

static void clone_generated(const SiteHelperProject *source, SiteHelperProject *copy)
{
    test_clone_project_authoritative(source, copy);
    for (size_t s = 0; s < copy->storey_count; s++) {
        BuildSettings resolved;
        assert(sitehelper_project_resolve_storey_build_settings(copy, copy->storeys[s].id, &resolved));
        for (size_t w = 0; w < copy->storeys[s].structure.wall_count; w++) {
            assert(wall_generate(&copy->storeys[s].structure.walls[w], &resolved));
        }
    }
}

static void projects_equal(const SiteHelperProject *a, const SiteHelperProject *b)
{
    test_assert_project_authoritative_equal(a, b);
    for (size_t s = 0; s < a->storey_count; s++) {
        for (size_t w = 0; w < a->storeys[s].structure.wall_count; w++) {
            test_assert_framing_semantically_equal(&a->storeys[s].structure.walls[w].framing,
                &b->storeys[s].structure.walls[w].framing);
        }
    }
}

static void validation(const SiteHelperProject *p, SiteHelperProjectValidationCode code, DomainId id)
{
    /* Byte snapshots are borrowed, not independent owning Projects. */
    SiteHelperProject bytes = *p;
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 1; allocation_count = 0;
#endif
    SiteHelperProjectValidation result = sitehelper_project_validate(p);
    assert(result.code == code && result.subject_id == id);
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 0; assert(allocation_count == 0);
#endif
    assert(memcmp(&bytes, p, sizeof bytes) == 0);
}

static void test_resolution_and_validation(void)
{
    SiteHelperProject p;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, -500), b = sitehelper_project_add_storey(&p, 9000);
    BuildSettings defaults = p.settings;
    defaults.stud_width = 45; defaults.stud_depth = 70; defaults.stud_spacing = 450;
    defaults.nog_spacing = 900; defaults.stud_spacing_mode = STUD_SPACING_EVEN;
    defaults.opening_width_allowance = 10; defaults.opening_height_allowance = 20;
    assert(sitehelper_project_set_build_settings(&p, &defaults));
    BuildSettings resolved = {0};
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 1; allocation_count = 0;
#endif
    assert(sitehelper_project_resolve_storey_build_settings(&p, a, &resolved));
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 0; assert(allocation_count == 0);
#endif
    test_assert_build_settings_equal(&defaults, &resolved);
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2700));
    assert(height(&p, a) == 2400 && height(&p, b) == 2700);
    assert(sitehelper_project_resolve_storey_build_settings(&p, b, &resolved));
    defaults.stud_height = 2700; test_assert_build_settings_equal(&defaults, &resolved);
    BuildSettings before = resolved;
    assert(!sitehelper_project_resolve_storey_build_settings(NULL, a, &resolved));
    assert(!sitehelper_project_resolve_storey_build_settings(&p, 0, &resolved));
    assert(!sitehelper_project_resolve_storey_build_settings(&p, UINT64_MAX, &resolved));
    assert(!sitehelper_project_resolve_storey_build_settings(&p, a, NULL));
    test_assert_build_settings_equal(&before, &resolved);
    assert(!sitehelper_project_set_stud_height(NULL, 2700));
    assert(!sitehelper_project_set_build_settings(&p, NULL));
    assert(!sitehelper_project_set_storey_stud_height(&p, 0, 2700));
    assert(!sitehelper_project_set_storey_stud_height(&p, UINT64_MAX, 2700));
    assert(!sitehelper_project_clear_storey_stud_height(&p, UINT64_MAX));
    assert(!sitehelper_project_clear_storey_stud_height(NULL, a));
    assert(!sitehelper_project_set_stud_height(&p, 0));
    assert(!sitehelper_project_set_storey_stud_height(&p, a, -1));
    assert(!sitehelper_project_set_storey_stud_height(&p, a, 0));
    assert(sitehelper_project_clear_storey_stud_height(&p, b));
    assert(height(&p, b) == 2400 && !p.storeys[1].settings.has_stud_height_override);
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2400));
    assert(p.storeys[1].settings.has_stud_height_override);
    assert(sitehelper_project_set_stud_height(&p, 2550));
    assert(height(&p, a) == 2550 && height(&p, b) == 2400);

    StoreyBuildSettings saved = p.storeys[1].settings;
    const StoreyBuildSettings invalid[] = {{true, 0}, {true, -1}, {false, 2400}};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        p.storeys[1].settings = invalid[i];
        validation(&p, SITEHELPER_PROJECT_INVALID_STOREY_SETTINGS, b);
        assert(!sitehelper_project_resolve_storey_build_settings(&p, b, &resolved));
        assert(memcmp(&p.storeys[1].settings, &invalid[i], sizeof invalid[i]) == 0);
        test_assert_build_settings_equal(&before, &resolved);
    }
    p.storeys[1].settings = saved;
    assert(sitehelper_project_set_storey_stud_height(&p, a, 2700));
    p.settings.stud_height = 0; /* Even with all Storeys overridden, default must be valid. */
    validation(&p, SITEHELPER_PROJECT_INVALID_SETTINGS, 0);
    assert(!sitehelper_project_resolve_storey_build_settings(&p, a, &resolved));
    p.settings.stud_height = 2550;
    assert(sitehelper_project_set_storey_stud_height(&p, a, INT_MAX));
    validation(&p, SITEHELPER_PROJECT_VALID, 0);
    /* Positive int semantics are preserved; noggin arithmetic must not overflow. */
    defaults = p.settings; defaults.nog_spacing = INT_MAX / 2;
    assert(sitehelper_project_set_build_settings(&p, &defaults));
    DomainId wall = add_wall(&p, a);
    framing_height(&p, wall, INT_MAX);
    assert(sitehelper_project_find_wall_by_id(&p, wall)->framing.nog_count > 0);
    sitehelper_project_destroy(&p);
}

static void test_regeneration_scope(void)
{
    SiteHelperProject p, definitions;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 3000);
    DomainId c = sitehelper_project_add_storey(&p, -3000);
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2700));
    DomainId wa = add_wall(&p, a), wb = add_wall(&p, b), wc = add_wall(&p, c);
    test_clone_project_authoritative(&p, &definitions);
    Wall *wall_b = sitehelper_project_find_wall_by_id(&p, wb);
    Timber *b_studs = wall_b->framing.studs;
    framing_height(&p, wa, 2400); framing_height(&p, wb, 2700);
    assert(sitehelper_project_set_stud_height(&p, 2550));
    framing_height(&p, wa, 2550); framing_height(&p, wc, 2550); framing_height(&p, wb, 2700);
    assert(wall_b->framing.studs == b_studs);
    Timber *a_studs = sitehelper_project_find_wall_by_id(&p, wa)->framing.studs;
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2900));
    framing_height(&p, wb, 2900);
    assert(sitehelper_project_find_wall_by_id(&p, wa)->framing.studs == a_studs);
    assert(sitehelper_project_clear_storey_stud_height(&p, b));
    framing_height(&p, wb, 2550);
    assert(sitehelper_project_find_wall_by_id(&p, wa)->framing.studs == a_studs);
    /* Changing only presence at equal effective height requires no regeneration. */
    b_studs = wall_b->framing.studs;
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 1; allocation_count = 0;
#endif
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2550));
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2550));
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    reject_all = 0; assert(allocation_count == 0);
#endif
    assert(wall_b->framing.studs == b_studs && p.storeys[1].settings.has_stud_height_override);
    definitions.settings = p.settings;
    for (size_t s = 0; s < p.storey_count; s++) { definitions.storeys[s].settings = p.storeys[s].settings; }
    test_assert_project_authoritative_equal(&definitions, &p);
    validation(&p, SITEHELPER_PROJECT_VALID, 0);
    sitehelper_project_destroy(&definitions); sitehelper_project_destroy(&p);
}

static void configure_opening_editor(SiteHelperEditor *editor, SiteHelperProject *p, DomainId storey, DomainId wall)
{
    sitehelper_editor_init(editor);
    assert(sitehelper_editor_set_current_storey(editor, p, storey));
    assert(sitehelper_editor_set_active_view(editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(sitehelper_editor_set_active_tool(editor, EDITOR_TOOL_OPENING));
    editor->current_wall_id = wall;
    editor->snap.settings.grid_enabled = true;
    editor->snap.settings.grid_spacing = 100;
    editor->snap.settings.object_snap_tolerance = 0;
    editor->opening_tool.width = 800;
    editor->opening_tool.height = 1600;
    editor->opening_tool.bottom = 900;
}

static void test_openings_and_preview(void)
{
    SiteHelperProject p, before;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 0);
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2700));
    DomainId walls[] = {add_wall(&p, a), add_wall(&p, b)};
    for (size_t i = 0; i < 2; i++) {
        SiteHelperEditor editor;
        DomainId storey = i == 0 ? a : b;
        configure_opening_editor(&editor, &p, storey, walls[i]);
        sitehelper_editor_pointer_move_in_project(&editor, &p, (Vec2){1200, 900});
        assert(opening_placement_is_valid(&editor.opening_placement) == (i == 1));
        BuildSettings resolved;
        assert(sitehelper_project_resolve_storey_build_settings(&p, storey, &resolved));
        WallOpeningProposal proposal = {.type = OPENING_WINDOW, .frame_position = 1200,
            .frame_bottom = 900, .width = 800, .height = 1600};
        assert((wall_validate_opening(sitehelper_project_find_wall_by_id(&p, walls[i]), &resolved, &proposal).code ==
            WALL_OPENING_VALID) == (i == 1));
        if (i == 1) {
            resolved.opening_height_allowance = 300;
            assert(wall_validate_opening(sitehelper_project_find_wall_by_id(&p, walls[i]), &resolved, &proposal).code ==
                WALL_OPENING_INVALID_HEIGHT);
            proposal.custom_allowance = true;
            assert(wall_validate_opening(sitehelper_project_find_wall_by_id(&p, walls[i]), &resolved, &proposal).code ==
                WALL_OPENING_VALID);
        }
        OpeningCommand command; DomainId opening_id, next = p.domain_ids.next;
        assert(opening_command_create(walls[i], OPENING_WINDOW, 1200, 900, 800, 1600, &command));
        assert(opening_command_execute(&p, &command, &opening_id) == (i == 1));
        if (i == 0) { assert(p.domain_ids.next == next && opening_id == 0); }
        else { framing_height(&p, walls[i], 2700); }
    }
    clone_generated(&p, &before);
    Timber *studs = sitehelper_project_find_wall_by_id(&p, walls[1])->framing.studs;
    assert(!sitehelper_project_set_storey_stud_height(&p, b, 2400));
    assert(!sitehelper_project_clear_storey_stud_height(&p, b));
    projects_equal(&before, &p);
    assert(sitehelper_project_find_wall_by_id(&p, walls[1])->framing.studs == studs);
    /* Raw invalid override proves validation uses effective height, independently of framing. */
    p.storeys[1].settings.stud_height = 2400;
    validation(&p, SITEHELPER_PROJECT_INVALID_OPENING,
        sitehelper_project_find_wall_by_id(&p, walls[1])->definition.openings[0].id);
    p.storeys[1].settings.stud_height = 2700;
    SiteHelperEditor editor;
    configure_opening_editor(&editor, &p, a, walls[0]);
    assert(sitehelper_project_set_storey_stud_height(&p, a, 2700));
    sitehelper_editor_pointer_move_in_project(&editor, &p, (Vec2){1200, 900});
    assert(opening_placement_is_valid(&editor.opening_placement));
    assert(sitehelper_project_clear_storey_stud_height(&p, a));
    EditorAction action;
    /* Click rechecks an old valid preview after an external settings mutation. */
    assert(!sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){1200, 900}, &action));
    assert(!opening_placement_is_valid(&editor.opening_placement));
    sitehelper_editor_pointer_move_in_project(&editor, NULL, (Vec2){0});
    assert(!editor.opening_placement.has_candidate && !sitehelper_editor_has_snap(&editor));
    sitehelper_project_destroy(&before); sitehelper_project_destroy(&p);
}

static void test_history_uses_current_settings(void)
{
    SiteHelperProject p;
    sitehelper_project_init(&p);
    DomainId s = sitehelper_project_add_storey(&p, 0);
    SiteHelperCommandHistory history;
    sitehelper_command_history_init(&history);
    SiteHelperCommand command; SiteHelperCommandResult result;
    WallCommand wall;
    assert(wall_command_create(s, segment, &wall));
    assert(sitehelper_command_from_wall(&wall, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    DomainId id = result.data.add_wall.wall_id;
    assert(sitehelper_command_history_undo(&history, &p));
    assert(sitehelper_project_set_storey_stud_height(&p, s, 2700));
    assert(sitehelper_command_history_redo(&history, &p)); framing_height(&p, id, 2700);
    OpeningCommand opening;
    assert(opening_command_create(id, OPENING_WINDOW, 1200, 900, 800, 1600, &opening));
    assert(sitehelper_command_from_opening(&opening, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_set_storey_stud_height(&p, s, 2900));
    assert(sitehelper_command_history_undo(&history, &p)); framing_height(&p, id, 2900);
    assert(sitehelper_project_clear_storey_stud_height(&p, s));
    size_t cursor = history.cursor;
    assert(!sitehelper_command_history_redo(&history, &p) && history.cursor == cursor);
    framing_height(&p, id, 2400);
    assert(sitehelper_project_set_storey_stud_height(&p, s, 2800));
    assert(sitehelper_command_history_redo(&history, &p)); framing_height(&p, id, 2800);
    MoveWallEndpointCommand move;
    assert(move_wall_endpoint_command_create(id, WALL_ENDPOINT_END, (PlanPosition){7000, 0}, &move));
    assert(sitehelper_command_from_move_wall_endpoint(&move, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_set_storey_stud_height(&p, s, 3000));
    assert(sitehelper_command_history_undo(&history, &p)); framing_height(&p, id, 3000);
    assert(sitehelper_command_history_redo(&history, &p)); framing_height(&p, id, 3000);
    DeleteWallCommand deletion;
    assert(delete_wall_command_create(id, &deletion));
    assert(sitehelper_command_from_delete_wall(&deletion, &command));
    assert(sitehelper_command_history_execute(&history, &p, &command, &result));
    assert(sitehelper_project_add_storey(&p, -1000));
    assert(sitehelper_project_clear_storey_stud_height(&p, s));
    cursor = history.cursor;
    assert(!sitehelper_command_history_undo(&history, &p) && history.cursor == cursor);
    assert(sitehelper_project_set_storey_stud_height(&p, s, 3100));
    assert(sitehelper_command_history_undo(&history, &p)); framing_height(&p, id, 3100);
    assert(sitehelper_project_find_owning_storey(&p, id)->id == s);
    validation(&p, SITEHELPER_PROJECT_VALID, 0);
    sitehelper_command_history_destroy(&history); sitehelper_project_destroy(&p);
}

static void test_wall_tool_and_late_regeneration_failure(void)
{
    SiteHelperProject p, before;
    sitehelper_project_init(&p);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, -1000);
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2700));
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor, &p, b));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){0, 0}, &action));
    sitehelper_editor_pointer_move_in_project(&editor, &p, (Vec2){6000, 0});
    WallPlanSegment preview;
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.start.x == 0 && preview.end.x == 6000);
    assert(sitehelper_editor_primary_action_in_project(&editor, &p, (Vec2){6000, 0}, &action));
    assert(action.command.data.wall.storey_id == b);
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&p, &action.command, &result));
    framing_height(&p, result.data.add_wall.wall_id, 2700);
    assert(sitehelper_project_set_stud_height(&p, 2700));
    DomainId first = add_wall(&p, a), second = add_wall(&p, a);
    OpeningCommand opening; DomainId opening_id;
    assert(opening_command_create(second, OPENING_WINDOW, 1200, 900, 800, 1600, &opening));
    assert(opening_command_execute(&p, &opening, &opening_id));
    clone_generated(&p, &before);
    const int invalid_heights[] = {2400, 2530}; /* Opening validation; then upper-cripple generation. */
    Timber *first_studs = sitehelper_project_find_wall_by_id(&p, first)->framing.studs;
    for (size_t i = 0; i < sizeof invalid_heights / sizeof *invalid_heights; i++) {
        assert(!sitehelper_project_set_stud_height(&p, invalid_heights[i]));
        projects_equal(&before, &p);
        assert(sitehelper_project_find_wall_by_id(&p, first)->framing.studs == first_studs);
    }
    sitehelper_project_destroy(&before); sitehelper_project_destroy(&p);
}

static void write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "w"); assert(f); assert(fputs(text, f) >= 0); assert(fclose(f) == 0);
}

static void test_persistence(void)
{
    const char *path = "project_settings_roundtrip.tmp";
    SiteHelperProject p, loaded, before;
    sitehelper_project_init(&p); sitehelper_project_init(&loaded);
    DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, -1000);
    DomainId c = sitehelper_project_add_storey(&p, 4000);
    assert(sitehelper_project_set_storey_stud_height(&p, b, 2700));
    assert(sitehelper_project_set_storey_stud_height(&p, c, 2400));
    DomainId wa = add_wall(&p, a), wb = add_wall(&p, b), wc = add_wall(&p, c);
    /* Parsing this Opening against Project defaults instead of the Storey fails. */
    OpeningCommand opening; DomainId opening_id;
    assert(opening_command_create(wb, OPENING_WINDOW, 1200, 900, 800, 1600, &opening));
    assert(opening_command_execute(&p, &opening, &opening_id));
    assert(sitehelper_project_save_file(&p, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    FILE *f = fopen(path, "r"); char text[4096]; assert(f);
    size_t length = fread(text, 1, sizeof text - 1, f); text[length] = '\0'; assert(fclose(f) == 0);
    assert(strstr(text, "sitehelper_project 9\n") == text);
    assert(strstr(text, "stud_height inherit\n") && strstr(text, "stud_height override 2400\n"));
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    projects_equal(&p, &loaded);
    assert(sitehelper_project_set_stud_height(&loaded, 2550));
    framing_height(&loaded, wa, 2550); framing_height(&loaded, wb, 2700); framing_height(&loaded, wc, 2400);
    clone_generated(&loaded, &before);
    const char *bad[] = {"", "stud_height unknown\n", "stud_height override 0\n",
        "stud_height override -1\n", "stud_height override 2147483648\n", "stud_height override\n",
        "stud_height inherit 2400\n", "stud_height override 2400\nstud_height inherit\n"};
    Storey *storage = loaded.storeys;
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        snprintf(text, sizeof text, "sitehelper_project 9\ndomain_id_next 3\n"
            "settings 2400 90 35 600 1200 0 0 maximise\nstoreys 1\nstorey 2 elevation 0\n%s"
            "walls 0\nrooms 0\nroom_separators 0\nend_storey\nend_project\n", bad[i]);
        write_file(path, text);
        assert(sitehelper_project_load_file(&loaded, path) != SITEHELPER_PERSISTENCE_SUCCESS);
        assert(loaded.storeys == storage); projects_equal(&before, &loaded);
    }
    write_file(path, "sitehelper_project 8\ndomain_id_next 5\nsettings 2550 90 35 600 1200 0 0 maximise\n"
        "storeys 2\nstorey 1 elevation 0\nwalls 1\nwall 3 segment 0 0 6000 0 openings 0\n"
        "rooms 0\nroom_separators 0\nend_storey\nstorey 2 elevation 3000\nwalls 0\n"
        "rooms 0\nroom_separators 0\nend_storey\nend_project\n");
    assert(sitehelper_project_load_file(&loaded, path) == SITEHELPER_PERSISTENCE_SUCCESS);
    assert(loaded.storey_count == 2 && loaded.domain_ids.next == 5);
    assert(!loaded.storeys[0].settings.has_stud_height_override && !loaded.storeys[1].settings.has_stud_height_override);
    framing_height(&loaded, 3, 2550);
    assert(sitehelper_project_set_stud_height(&loaded, 2700)); framing_height(&loaded, 3, 2700);
    assert(remove(path) == 0);
    sitehelper_project_destroy(&before); sitehelper_project_destroy(&p); sitehelper_project_destroy(&loaded);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_transaction_failures(void)
{
    size_t totals[4] = {0};
    for (int operation = 0; operation < 4; operation++) {
        for (size_t fail_at = 0; fail_at < 256; fail_at++) {
            SiteHelperProject p, before;
            sitehelper_project_init(&p);
            DomainId a = sitehelper_project_add_storey(&p, 0), b = sitehelper_project_add_storey(&p, 3000);
            DomainId c = sitehelper_project_add_storey(&p, -3000);
            if (operation == 2) { assert(sitehelper_project_set_storey_stud_height(&p, a, 2700)); }
            assert(sitehelper_project_set_storey_stud_height(&p, b, 2700));
            DomainId ids[] = {add_wall(&p, a), add_wall(&p, a), add_wall(&p, b), add_wall(&p, c)};
            clone_generated(&p, &before);
            WallFraming frames[4];
            for (size_t i = 0; i < 4; i++) { frames[i] = sitehelper_project_find_wall_by_id(&p, ids[i])->framing; }
            Storey *storage = p.storeys;
            BuildSettings defaults = p.settings; defaults.stud_width = 45;
            failed = 0; fail_after = fail_at;
            int success = operation == 0 ? sitehelper_project_set_stud_height(&p, 2550) :
                operation == 1 ? sitehelper_project_set_storey_stud_height(&p, a, 2800) :
                operation == 2 ? sitehelper_project_clear_storey_stud_height(&p, a) :
                sitehelper_project_set_build_settings(&p, &defaults);
            fail_after = SIZE_MAX;
            if (failed) {
                totals[operation]++;
                assert(!success && p.storeys == storage); projects_equal(&before, &p);
                for (size_t i = 0; i < 4; i++) {
                    assert(memcmp(&frames[i], &sitehelper_project_find_wall_by_id(&p, ids[i])->framing, sizeof frames[i]) == 0);
                }
                assert(operation == 0 ? sitehelper_project_set_stud_height(&p, 2550) :
                    operation == 1 ? sitehelper_project_set_storey_stud_height(&p, a, 2800) :
                    operation == 2 ? sitehelper_project_clear_storey_stud_height(&p, a) :
                    sitehelper_project_set_build_settings(&p, &defaults));
            } else { assert(success); }
            validation(&p, SITEHELPER_PROJECT_VALID, 0);
            sitehelper_project_destroy(&before); sitehelper_project_destroy(&p);
            if (!failed) { break; }
            assert(fail_at < 255);
        }
        assert(totals[operation] > 5);
    }
    printf("Settings allocation failures: default=%zu set=%zu clear=%zu defaults=%zu\n",
        totals[0], totals[1], totals[2], totals[3]);
}
#endif

int main(void)
{
    test_resolution_and_validation(); test_regeneration_scope(); test_openings_and_preview();
    test_history_uses_current_settings(); test_wall_tool_and_late_regeneration_failure(); test_persistence();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_transaction_failures();
#endif
    puts("Project settings tests passed");
    return 0;
}
