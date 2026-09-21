#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"

static void test_view_capability_is_mechanical(void)
{
    assert(editor_view_supports_tool(EDITOR_VIEW_PLAN, EDITOR_TOOL_WALL));
    assert(editor_view_supports_tool(EDITOR_VIEW_PLAN, EDITOR_TOOL_SLAB));
    assert(editor_view_supports_tool(EDITOR_VIEW_PLAN, EDITOR_TOOL_NOTE));
    assert(!editor_view_supports_tool(EDITOR_VIEW_PLAN, EDITOR_TOOL_OPENING));

    assert(editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_SELECT));
    assert(editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_OPENING));
    assert(!editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_WALL));
    assert(!editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_MEASURE));

    assert(!editor_view_supports_tool(EDITOR_VIEW_COUNT, EDITOR_TOOL_SELECT));
    assert(!editor_view_supports_tool(EDITOR_VIEW_PLAN, EDITOR_TOOL_COUNT));
}

static void test_workspace_view_compatibility(void)
{
    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_GENERAL, EDITOR_VIEW_PLAN));
    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_GENERAL,
        EDITOR_VIEW_WALL_ELEVATION));
    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_PLAN));
    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_FRAMING,
        EDITOR_VIEW_WALL_ELEVATION));

    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_SLAB, EDITOR_VIEW_PLAN));
    assert(!editor_workspace_supports_view(EDITOR_WORKSPACE_SLAB,
        EDITOR_VIEW_WALL_ELEVATION));
    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_ROOF, EDITOR_VIEW_PLAN));
    assert(!editor_workspace_supports_view(EDITOR_WORKSPACE_ROOF,
        EDITOR_VIEW_WALL_ELEVATION));
    assert(editor_workspace_supports_view(EDITOR_WORKSPACE_DOCUMENTATION,
        EDITOR_VIEW_PLAN));
    assert(!editor_workspace_supports_view(EDITOR_WORKSPACE_DOCUMENTATION,
        EDITOR_VIEW_WALL_ELEVATION));

    assert(!editor_workspace_supports_view(EDITOR_WORKSPACE_COUNT, EDITOR_VIEW_PLAN));
    assert(!editor_workspace_supports_view(EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_COUNT));
}

static void test_workspace_preferred_view_is_explicit(void)
{
    for (int workspace = EDITOR_WORKSPACE_GENERAL;
         workspace < EDITOR_WORKSPACE_COUNT; workspace++) {
        assert(editor_workspace_preferred_view((EditorWorkspace)workspace) ==
            EDITOR_VIEW_PLAN);
    }
    assert(editor_workspace_preferred_view(EDITOR_WORKSPACE_COUNT) == EDITOR_VIEW_COUNT);
}

static void test_workspace_tool_exposure_is_domain_policy(void)
{
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_GENERAL, EDITOR_TOOL_WALL));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_GENERAL, EDITOR_TOOL_OPENING));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_GENERAL, EDITOR_TOOL_SLAB));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_GENERAL, EDITOR_TOOL_NOTE));

    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_FRAMING, EDITOR_TOOL_WALL));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_FRAMING, EDITOR_TOOL_OPENING));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_FRAMING, EDITOR_TOOL_MEASURE));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_FRAMING, EDITOR_TOOL_SLAB));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_FRAMING, EDITOR_TOOL_NOTE));

    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_SLAB, EDITOR_TOOL_SLAB));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_SLAB,
        EDITOR_TOOL_SLAB_PENETRATION));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_SLAB,
        EDITOR_TOOL_SLAB_EDGE_REBATE));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_SLAB, EDITOR_TOOL_WALL));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_SLAB, EDITOR_TOOL_OPENING));

    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_ROOF, EDITOR_TOOL_SELECT));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_ROOF, EDITOR_TOOL_MEASURE));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_ROOF, EDITOR_TOOL_DIMENSION));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_ROOF, EDITOR_TOOL_SLAB));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_ROOF, EDITOR_TOOL_WALL));

    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_DOCUMENTATION, EDITOR_TOOL_NOTE));
    assert(editor_workspace_exposes_tool(EDITOR_WORKSPACE_DOCUMENTATION,
        EDITOR_TOOL_REVISION_CLOUD));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_DOCUMENTATION, EDITOR_TOOL_WALL));
    assert(!editor_workspace_exposes_tool(EDITOR_WORKSPACE_DOCUMENTATION, EDITOR_TOOL_SLAB));
}


static void test_workspace_selection_policy_is_domain_scoped(void)
{
    for (int kind = EDITOR_SELECTION_NONE; kind <= EDITOR_SELECTION_DOCUMENT; kind++) {
        assert(editor_workspace_accepts_selection(EDITOR_WORKSPACE_GENERAL,
            (EditorSelectionKind)kind));
    }

    assert(editor_workspace_accepts_selection(EDITOR_WORKSPACE_FRAMING,
        EDITOR_SELECTION_WALL));
    assert(editor_workspace_accepts_selection(EDITOR_WORKSPACE_FRAMING,
        EDITOR_SELECTION_OPENING));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_FRAMING,
        EDITOR_SELECTION_SLAB));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_FRAMING,
        EDITOR_SELECTION_DOCUMENT));

    assert(editor_workspace_accepts_selection(EDITOR_WORKSPACE_SLAB,
        EDITOR_SELECTION_SLAB_REGION));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_SLAB,
        EDITOR_SELECTION_WALL));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_SLAB,
        EDITOR_SELECTION_ROOF));

    assert(editor_workspace_accepts_selection(EDITOR_WORKSPACE_ROOF,
        EDITOR_SELECTION_ROOF_PORTION));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_ROOF,
        EDITOR_SELECTION_SLAB));

    assert(editor_workspace_accepts_selection(EDITOR_WORKSPACE_DOCUMENTATION,
        EDITOR_SELECTION_DOCUMENT));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_DOCUMENTATION,
        EDITOR_SELECTION_WALL));

    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_COUNT,
        EDITOR_SELECTION_NONE));
    assert(!editor_workspace_accepts_selection(EDITOR_WORKSPACE_FRAMING,
        (EditorSelectionKind)99));
}

static void test_effective_tool_availability_requires_workspace_and_view(void)
{
    assert(editor_context_tool_available(EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_PLAN,
        EDITOR_TOOL_WALL));
    assert(!editor_context_tool_available(EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_PLAN,
        EDITOR_TOOL_OPENING));
    assert(editor_context_tool_available(EDITOR_WORKSPACE_FRAMING,
        EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_OPENING));

    assert(editor_context_tool_available(EDITOR_WORKSPACE_SLAB, EDITOR_VIEW_PLAN,
        EDITOR_TOOL_SLAB_REGION));
    assert(!editor_context_tool_available(EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_PLAN,
        EDITOR_TOOL_SLAB_REGION));
    assert(!editor_context_tool_available(EDITOR_WORKSPACE_SLAB,
        EDITOR_VIEW_WALL_ELEVATION, EDITOR_TOOL_SELECT));

    assert(editor_context_tool_available(EDITOR_WORKSPACE_DOCUMENTATION, EDITOR_VIEW_PLAN,
        EDITOR_TOOL_CALLOUT));
    assert(!editor_context_tool_available(EDITOR_WORKSPACE_DOCUMENTATION, EDITOR_VIEW_PLAN,
        EDITOR_TOOL_WALL));
}

static void test_editor_defaults_to_compatibility_workspace(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);

    assert(sitehelper_editor_get_active_workspace(&editor) == EDITOR_WORKSPACE_GENERAL);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
    assert(sitehelper_editor_tool_available(&editor, EDITOR_TOOL_WALL));
    assert(sitehelper_editor_tool_available(&editor, EDITOR_TOOL_SLAB));
    assert(sitehelper_editor_tool_available(&editor, EDITOR_TOOL_NOTE));
}

static void test_workspace_change_preserves_navigation_and_clears_selection(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 11;
    editor.current_room_id = 12;
    editor.current_wall_id = 13;
    editor_selection_set_wall(&editor.selection, EDITOR_SELECTION_SCOPE_PLAN, 13);

    assert(sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_FRAMING));
    assert(editor.current_storey_id == 11);
    assert(editor.current_room_id == 12);
    assert(editor.current_wall_id == 13);
    assert(editor_selection_is_empty(&editor.selection));
    assert(editor.active_view == EDITOR_VIEW_PLAN);
}

static void test_workspace_change_cancels_compatible_tool_interaction(void)
{
    SiteHelperEditor editor;
    EditorAction action;
    PlanMeasurementQuery query;
    sitehelper_editor_init(&editor);

    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_MEASURE));
    assert(sitehelper_editor_primary_action(&editor, NULL, (Vec2){100.0, 200.0}, &action));
    assert(sitehelper_editor_get_measurement(&editor, &query));

    assert(sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_SLAB));
    assert(editor.active_tool == EDITOR_TOOL_MEASURE);
    assert(editor.measurement_tool.active);
    assert(!sitehelper_editor_get_measurement(&editor, &query));
}

static void test_workspace_change_falls_back_from_hidden_tool(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);

    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_SLAB));
    assert(editor.slab_tool.active);
    assert(sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_FRAMING));
    assert(editor.active_tool == EDITOR_TOOL_SELECT);
    assert(!editor.slab_tool.active);
    assert(!sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_SLAB));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_WALL));
}

static void test_workspace_change_does_not_silently_change_view(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);

    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(editor.active_view == EDITOR_VIEW_WALL_ELEVATION);
    assert(!sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_SLAB));
    assert(editor.active_workspace == EDITOR_WORKSPACE_GENERAL);
    assert(editor.active_view == EDITOR_VIEW_WALL_ELEVATION);
}

static void test_view_change_respects_active_workspace(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);

    assert(sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_SLAB));
    assert(!sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(editor.active_workspace == EDITOR_WORKSPACE_SLAB);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
}

static void test_application_can_coordinate_cross_workspace_view_transition(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);

    assert(sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_FRAMING));
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(!sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_SLAB));

    assert(sitehelper_editor_set_active_view(&editor,
        editor_workspace_preferred_view(EDITOR_WORKSPACE_SLAB)));
    assert(sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_SLAB));
    assert(editor.active_workspace == EDITOR_WORKSPACE_SLAB);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
}

static void test_invalid_workspace_is_rejected_without_state_change(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);

    assert(!sitehelper_editor_set_active_workspace(&editor, EDITOR_WORKSPACE_COUNT));
    assert(editor.active_workspace == EDITOR_WORKSPACE_GENERAL);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
    assert(editor.active_tool == EDITOR_TOOL_SELECT);
}

int main(void)
{
    test_view_capability_is_mechanical();
    test_workspace_view_compatibility();
    test_workspace_preferred_view_is_explicit();
    test_workspace_tool_exposure_is_domain_policy();
    test_workspace_selection_policy_is_domain_scoped();
    test_effective_tool_availability_requires_workspace_and_view();
    test_editor_defaults_to_compatibility_workspace();
    test_workspace_change_preserves_navigation_and_clears_selection();
    test_workspace_change_cancels_compatible_tool_interaction();
    test_workspace_change_falls_back_from_hidden_tool();
    test_workspace_change_does_not_silently_change_view();
    test_view_change_respects_active_workspace();
    test_application_can_coordinate_cross_workspace_view_transition();
    test_invalid_workspace_is_rejected_without_state_change();

    printf("All editor context tests passed.\n");
    return 0;
}
