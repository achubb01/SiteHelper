#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "editor_properties.h"
#include "command_history.h"
#include "test_support.h"
#include "wall_query.h"

typedef struct
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    DomainId storey_id, wall_id, opening_id;
    Opening opening;
} Fixture;

static Wall *fixture_wall(Fixture *f)
{
    return sitehelper_project_find_wall_by_id(&f->project, f->wall_id);
}

static void fixture_init(Fixture *f)
{
    *f = (Fixture){0};
    sitehelper_project_init(&f->project);
    sitehelper_editor_init(&f->editor);
    sitehelper_command_history_init(&f->history);
    f->storey_id = sitehelper_project_add_storey(&f->project, 3000);
    assert(sitehelper_project_set_storey_stud_height(&f->project, f->storey_id, 3200));
    BuildSettings defaults = f->project.settings;
    defaults.stud_spacing = 450;
    defaults.opening_width_allowance = 25;
    defaults.opening_height_allowance = 30;
    assert(sitehelper_project_set_build_settings(&f->project, &defaults));
    f->wall_id = sitehelper_project_add_wall(&f->project, f->storey_id,
        (WallPlanSegment){{4600, 6800}, {1000, 2000}});
    f->opening_id = domain_id_generate(&f->project.domain_ids);
    f->opening = (Opening){.id = f->opening_id, .type = OPENING_WINDOW,
        .frame_position = 1000, .frame_bottom = 600, .width = 1000, .height = 1200,
        .custom_allowance = true, .width_allowance = 12, .height_allowance = 15};
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f->project, f->storey_id, &settings));
    assert(wall_add_opening_definition(fixture_wall(f), &settings, &f->opening));
    assert(wall_generate(fixture_wall(f), &settings));
    assert(sitehelper_editor_set_current_storey(&f->editor, &f->project, f->storey_id));
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_command_history_destroy(&f->history);
    sitehelper_project_destroy(&f->project);
}

static void assert_empty_projection(Fixture *f)
{
    EditorProperties properties;
    memset(&properties, 0xa5, sizeof properties);
    assert(!sitehelper_editor_inspect_properties(&f->editor, &f->project, &properties));
    assert(properties.kind == EDITOR_SELECTION_NONE);
    assert(properties.data.wall.wall_id == DOMAIN_ID_INVALID);
    assert(properties.data.wall.length_mm == 0);
    SiteHelperCommand command = {.type = SITEHELPER_COMMAND_ADD_OPENING};
    EditorPropertyEdit edit = {.property = EDITOR_PROPERTY_OPENING_WIDTH, .value.millimetres = 1200};
    assert(!sitehelper_editor_create_property_command(&f->editor, &f->project, &edit, &command));
    assert(command.type == SITEHELPER_COMMAND_NONE);
}

static void test_snapshots_are_fresh_and_navigation_independent(void)
{
    Fixture f;
    fixture_init(&f);
    assert_empty_projection(&f);
    editor_selection_set_wall(&f.editor.selection, f.wall_id);
    assert(f.editor.current_wall_id == DOMAIN_ID_INVALID);
    EditorProperties properties;
    assert(sitehelper_editor_inspect_properties(&f.editor, &f.project, &properties));
    assert(properties.kind == EDITOR_SELECTION_WALL);
    EditorWallProperties wall = properties.data.wall;
    assert(wall.wall_id == f.wall_id);
    assert(wall.segment.start.x == 4600 && wall.segment.start.y == 6800);
    assert(wall.segment.end.x == 1000 && wall.segment.end.y == 2000);
    assert(wall.length_mm == 6000);
    assert(wall.resolved_stud_height == 3200 && f.project.settings.stud_height == 2400);
    assert(wall.resolved_stud_spacing == 450);
    assert(sitehelper_project_set_storey_stud_height(&f.project, f.storey_id, 3400));
    assert(sitehelper_editor_inspect_properties(&f.editor, &f.project, &properties));
    assert(properties.data.wall.resolved_stud_height == 3400);
    /* The returned value is independent of the authoritative model. */
    properties.data.wall.segment.start.x = 999;
    assert(fixture_wall(&f)->definition.segment.start.x == 4600);
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    assert(sitehelper_editor_inspect_properties(&f.editor, &f.project, &properties));
    assert(properties.kind == EDITOR_SELECTION_OPENING);
    assert(properties.data.opening.wall_id == f.wall_id);
    test_assert_opening_equal(&f.opening, &properties.data.opening.definition);
    properties.data.opening.definition.width = 999;
    test_assert_opening_equal(&f.opening, &fixture_wall(&f)->definition.openings[0]);
    fixture_destroy(&f);
}

static void test_selection_relocation_regeneration_and_stale_ids(void)
{
    Fixture f;
    fixture_init(&f);
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    EditorSelection saved = f.editor.selection;
    /* Reallocate all containing collections; selection contains only values. */
    for (int i = 0; i < 12; i++) {
        assert(sitehelper_project_add_storey(&f.project, i * 1000));
        assert(sitehelper_project_add_wall(&f.project, f.storey_id,
            (WallPlanSegment){{i * 100, 0}, {i * 100 + 3000, 0}}));
    }
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
    assert(wall_add_opening(fixture_wall(&f), &settings, domain_id_generate(&f.project.domain_ids),
        OPENING_DOOR, 4000, 0, 800, 2100));
    assert(wall_generate(fixture_wall(&f), &settings));
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(f.editor.selection.kind == saved.kind);
    assert(f.editor.selection.wall_id == saved.wall_id && f.editor.selection.opening_id == saved.opening_id);
    EditorProperties properties;
    assert(sitehelper_editor_inspect_properties(&f.editor, &f.project, &properties));
    test_assert_opening_equal(&f.opening, &properties.data.opening.definition);
    editor_selection_set_wall(&f.editor.selection, f.wall_id);
    assert(wall_generate(fixture_wall(&f), &settings));
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(f.editor.selection.kind == EDITOR_SELECTION_WALL && f.editor.selection.wall_id == f.wall_id);
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    assert(wall_remove_opening_by_id(fixture_wall(&f), f.opening_id));
    assert(wall_generate(fixture_wall(&f), &settings));
    assert_empty_projection(&f); /* Even before reconciliation, never returns stale values. */
    sitehelper_editor_reconcile_wall_selection(&f.editor, fixture_wall(&f));
    assert(editor_selection_is_empty(&f.editor.selection));
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(editor_selection_is_empty(&f.editor.selection));
    editor_selection_set_wall(&f.editor.selection, f.wall_id);
    f.editor.current_wall_id = f.wall_id;
    assert(sitehelper_project_remove_wall_by_id(&f.project, f.wall_id));
    assert_empty_projection(&f);
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(editor_selection_is_empty(&f.editor.selection));
    assert(f.editor.current_wall_id == DOMAIN_ID_INVALID);
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    assert_empty_projection(&f);
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(editor_selection_is_empty(&f.editor.selection));
    fixture_destroy(&f);
}

static void test_context_and_member_selection(void)
{
    Fixture f;
    fixture_init(&f);
    Wall *wall = fixture_wall(&f);
    Timber member = wall->framing.studs[0];
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    editor_selection_set_wall_member(&f.editor.selection, f.wall_id, WALL_MEMBER_STUD, &member);
    member.length = 1; /* Setter copied the Timber value. */
    assert(f.editor.selection.opening_id == DOMAIN_ID_INVALID);
    assert(f.editor.selection.wall_member.timber.length == 3200);
    assert_empty_projection(&f);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
    assert(wall_generate(wall, &settings));
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(f.editor.selection.kind == EDITOR_SELECTION_WALL_MEMBER);
    assert(wall_selection_resolve(&f.editor.selection.wall_member, wall));
    assert(sitehelper_project_set_storey_stud_height(&f.project, f.storey_id, 3400));
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(editor_selection_is_empty(&f.editor.selection));

    DomainId other = sitehelper_project_add_storey(&f.project, 6000);
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    assert(sitehelper_editor_set_current_storey(&f.editor, &f.project, other));
    assert(editor_selection_is_empty(&f.editor.selection));
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    assert_empty_projection(&f); /* Existing object in a different Storey. */
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(editor_selection_is_empty(&f.editor.selection));
    assert(sitehelper_editor_set_current_storey(&f.editor, &f.project, f.storey_id));
    editor_selection_set_wall(&f.editor.selection, f.wall_id);
    assert(sitehelper_editor_set_active_view(&f.editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(editor_selection_is_empty(&f.editor.selection));
    editor_selection_set_wall(&f.editor.selection, f.wall_id);
    f.editor.current_storey_id = 99999;
    assert_empty_projection(&f);
    sitehelper_editor_reconcile(&f.editor, &f.project);
    assert(f.editor.current_storey_id == DOMAIN_ID_INVALID);
    assert(editor_selection_is_empty(&f.editor.selection));
    editor_selection_set_wall(&f.editor.selection, DOMAIN_ID_INVALID);
    assert(editor_selection_is_empty(&f.editor.selection));
    editor_selection_set_opening(&f.editor.selection, f.wall_id, DOMAIN_ID_INVALID);
    assert(editor_selection_is_empty(&f.editor.selection));
    fixture_destroy(&f);
}

static void test_wall_component_intents_and_history(void)
{
    Fixture f;
    fixture_init(&f);
    editor_selection_set_wall(&f.editor.selection, f.wall_id);
    EditorProperty fields[] = {EDITOR_PROPERTY_WALL_START_X, EDITOR_PROPERTY_WALL_START_Y,
        EDITOR_PROPERTY_WALL_END_X, EDITOR_PROPERTY_WALL_END_Y};
    PlanPosition expected[] = {{5600, 6800}, {4600, 7800}, {0, 2000}, {1000, 1000}};
    int values[] = {5600, 7800, 0, 1000};
    Wall original;
    test_clone_wall_definition(fixture_wall(&f), &original);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
    assert(wall_generate(&original, &settings));
    for (size_t i = 0; i < 4; i++) {
        EditorPropertyEdit edit = {.property = fields[i], .value.millimetres = values[i]};
        SiteHelperCommand command;
        assert(sitehelper_editor_create_property_command(&f.editor, &f.project, &edit, &command));
        assert(command.type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT);
        assert(command.data.move_wall_endpoint.wall_id == f.wall_id);
        assert(command.data.move_wall_endpoint.endpoint == (i < 2 ? WALL_ENDPOINT_START : WALL_ENDPOINT_END));
        assert(command.data.move_wall_endpoint.new_position.x == expected[i].x);
        assert(command.data.move_wall_endpoint.new_position.y == expected[i].y);
        test_assert_wall_definition_equal(&original, fixture_wall(&f));
        test_assert_framing_semantically_equal(&original.framing, &fixture_wall(&f)->framing);
        SiteHelperCommandResult result;
        assert(sitehelper_command_history_execute(&f.history, &f.project, &command, &result));
        sitehelper_editor_reconcile(&f.editor, &f.project);
        assert(f.editor.selection.kind == EDITOR_SELECTION_WALL);
        EditorProperties properties;
        assert(sitehelper_editor_inspect_properties(&f.editor, &f.project, &properties));
        WallPlanSegment segment = properties.data.wall.segment;
        PlanPosition changed = i < 2 ? segment.start : segment.end;
        assert(changed.x == expected[i].x && changed.y == expected[i].y);
        assert(sitehelper_command_history_undo(&f.history, &f.project));
        test_assert_wall_definition_equal(&original, fixture_wall(&f));
        test_assert_framing_semantically_equal(&original.framing, &fixture_wall(&f)->framing);
        assert(sitehelper_command_history_redo(&f.history, &f.project));
        changed = i < 2 ? fixture_wall(&f)->definition.segment.start : fixture_wall(&f)->definition.segment.end;
        assert(changed.x == expected[i].x && changed.y == expected[i].y);
        assert(sitehelper_command_history_undo(&f.history, &f.project));
    }
    EditorPropertyEdit wrong = {.property = EDITOR_PROPERTY_OPENING_WIDTH, .value.millimetres = 1200};
    SiteHelperCommand command;
    assert(!sitehelper_editor_create_property_command(&f.editor, &f.project, &wrong, &command));
    assert(command.type == SITEHELPER_COMMAND_NONE);
    wall_destroy(&original);
    fixture_destroy(&f);
}

static void test_each_opening_property_intent(void)
{
    Fixture f;
    fixture_init(&f);
    editor_selection_set_opening(&f.editor.selection, f.wall_id, f.opening_id);
    EditorPropertyEdit edits[] = {
        {.property = EDITOR_PROPERTY_OPENING_TYPE, .value.opening_type = OPENING_DOOR},
        {.property = EDITOR_PROPERTY_OPENING_FRAME_POSITION, .value.millimetres = 1200},
        {.property = EDITOR_PROPERTY_OPENING_FRAME_BOTTOM, .value.millimetres = 800},
        {.property = EDITOR_PROPERTY_OPENING_WIDTH, .value.millimetres = 1200},
        {.property = EDITOR_PROPERTY_OPENING_HEIGHT, .value.millimetres = 1500},
        {.property = EDITOR_PROPERTY_OPENING_CUSTOM_ALLOWANCE, .value.custom_allowance = false},
        {.property = EDITOR_PROPERTY_OPENING_WIDTH_ALLOWANCE, .value.millimetres = 40},
        {.property = EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE, .value.millimetres = 45}
    };
    Opening expected[8];
    for (size_t i = 0; i < 8; i++) { expected[i] = f.opening; }
    expected[0].type = OPENING_DOOR;
    expected[1].frame_position = 1200;
    expected[2].frame_bottom = 800;
    expected[3].width = 1200;
    expected[4].height = 1500;
    expected[5].custom_allowance = false;
    expected[6].width_allowance = 40;
    expected[7].height_allowance = 45;
    for (size_t i = 0; i < 8; i++) {
        SiteHelperCommand command;
        assert(sitehelper_editor_create_property_command(&f.editor, &f.project, &edits[i], &command));
        assert(command.type == SITEHELPER_COMMAND_EDIT_OPENING);
        assert(command.data.edit_opening.wall_id == f.wall_id);
        assert(command.data.edit_opening.opening_id == f.opening_id);
        test_assert_opening_equal(&expected[i], &command.data.edit_opening.definition);
        test_assert_opening_equal(&f.opening, &fixture_wall(&f)->definition.openings[0]);
        SiteHelperCommandResult result;
        assert(sitehelper_command_history_execute(&f.history, &f.project, &command, &result));
        sitehelper_editor_reconcile(&f.editor, &f.project);
        assert(f.editor.selection.kind == EDITOR_SELECTION_OPENING);
        assert(f.editor.selection.opening_id == f.opening_id);
        EditorProperties properties;
        assert(sitehelper_editor_inspect_properties(&f.editor, &f.project, &properties));
        test_assert_opening_equal(&expected[i], &properties.data.opening.definition);
        assert(sitehelper_command_history_undo(&f.history, &f.project));
        test_assert_opening_equal(&f.opening, &fixture_wall(&f)->definition.openings[0]);
    }
    /* Typed intent creation leaves domain validation to command execution. */
    EditorPropertyEdit invalid = {.property = EDITOR_PROPERTY_OPENING_WIDTH, .value.millimetres = -1};
    SiteHelperCommand command;
    assert(sitehelper_editor_create_property_command(&f.editor, &f.project, &invalid, &command));
    SiteHelperCommandResult result;
    assert(!sitehelper_command_history_execute(&f.history, &f.project, &command, &result));
    assert(f.history.count == 1 && f.history.cursor == 0);
    invalid.property = (EditorProperty)999;
    assert(!sitehelper_editor_create_property_command(&f.editor, &f.project, &invalid, &command));
    invalid.property = EDITOR_PROPERTY_WALL_START_X;
    assert(!sitehelper_editor_create_property_command(&f.editor, &f.project, &invalid, &command));
    fixture_destroy(&f);
}

static void test_opening_hit_testing_and_select_tool(void)
{
    Fixture f;
    fixture_init(&f);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&f.project, f.storey_id, &settings));
    /* No Timber allocations are needed by the authoritative query. */
    wall_framing_destroy(&fixture_wall(&f)->framing);
    assert(wall_find_opening_at_position(fixture_wall(&f), &settings,
        (WallLocalPosition){2010, 1810}) == f.opening_id); /* Custom allowances. */
    assert(wall_find_opening_at_position(fixture_wall(&f), &settings,
        (WallLocalPosition){2013, 1810}) == DOMAIN_ID_INVALID);
    Opening opening = f.opening;
    opening.custom_allowance = false;
    assert(wall_apply_opening_definition(fixture_wall(&f), &settings, f.opening_id, &opening));
    assert(wall_find_opening_at_position(fixture_wall(&f), &settings,
        (WallLocalPosition){2024, 1829}) == f.opening_id); /* Resolved defaults. */
    assert(wall_find_opening_at_position(fixture_wall(&f), &settings,
        (WallLocalPosition){2026, 1829}) == DOMAIN_ID_INVALID);
    assert(wall_find_opening_at_position(fixture_wall(&f), NULL,
        (WallLocalPosition){1500, 1000}) == DOMAIN_ID_INVALID);
    assert(sitehelper_editor_set_active_view(&f.editor, EDITOR_VIEW_WALL_ELEVATION));
    f.editor.current_wall_id = f.wall_id;
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(&f.editor, &f.project, (Vec2){1500, 1000}, &action));
    assert(action.kind == EDITOR_ACTION_NONE);
    assert(f.editor.selection.kind == EDITOR_SELECTION_OPENING);
    assert(f.editor.selection.wall_id == f.wall_id && f.editor.selection.opening_id == f.opening_id);
    assert(sitehelper_editor_primary_action_in_project(&f.editor, &f.project, (Vec2){10, 10}, &action));
    assert(f.editor.selection.kind == EDITOR_SELECTION_WALL_MEMBER);
    assert(sitehelper_editor_primary_action_in_project(&f.editor, &f.project, (Vec2){9000, 9000}, &action));
    assert(editor_selection_is_empty(&f.editor.selection));
    assert(sitehelper_editor_primary_action_in_project(&f.editor, &f.project, (Vec2){NAN, INFINITY}, &action));
    assert(editor_selection_is_empty(&f.editor.selection));
    assert(sitehelper_editor_set_active_view(&f.editor, EDITOR_VIEW_PLAN));
    assert(sitehelper_editor_primary_action_in_project(&f.editor, &f.project, (Vec2){2800, 4400}, &action));
    assert(f.editor.selection.kind == EDITOR_SELECTION_WALL && f.editor.selection.wall_id == f.wall_id);
    fixture_destroy(&f);
}

int main(void)
{
    test_snapshots_are_fresh_and_navigation_independent();
    test_selection_relocation_regeneration_and_stale_ids();
    test_context_and_member_selection();
    test_wall_component_intents_and_history();
    test_each_opening_property_intent();
    test_opening_hit_testing_and_select_tool();
    puts("editor properties tests passed");
    return 0;
}
