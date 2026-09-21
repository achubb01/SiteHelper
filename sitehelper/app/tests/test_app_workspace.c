#include <assert.h>
#include <stdio.h>

#include "app_workspace.h"

static void assert_camera(Camera2D actual, Camera2D expected)
{
    assert(actual.position.x == expected.position.x);
    assert(actual.position.y == expected.position.y);
    assert(actual.scale == expected.scale);
}

static void test_user_choices_exclude_compatibility_workspace(void)
{
    EditorWorkspace choices[EDITOR_WORKSPACE_COUNT] = {0};
    size_t count = app_workspace_user_choices(choices, EDITOR_WORKSPACE_COUNT);
    assert(count == 4);
    assert(choices[0] == EDITOR_WORKSPACE_FRAMING);
    assert(choices[1] == EDITOR_WORKSPACE_SLAB);
    assert(choices[2] == EDITOR_WORKSPACE_ROOF);
    assert(choices[3] == EDITOR_WORKSPACE_DOCUMENTATION);
    for (size_t i = 0; i < count; i++) {
        assert(choices[i] != EDITOR_WORKSPACE_GENERAL);
    }
}

static void test_command_surfaces_match_workspace_exposure(void)
{
    for (int workspace = EDITOR_WORKSPACE_GENERAL;
         workspace < EDITOR_WORKSPACE_COUNT; workspace++) {
        EditorTool tools[EDITOR_TOOL_COUNT];
        size_t count = app_workspace_tool_surface((EditorWorkspace)workspace,
            tools, EDITOR_TOOL_COUNT);
        assert(count > 0 && count <= EDITOR_TOOL_COUNT);

        int seen[EDITOR_TOOL_COUNT] = {0};
        for (size_t i = 0; i < count; i++) {
            assert(tools[i] >= EDITOR_TOOL_SELECT && tools[i] < EDITOR_TOOL_COUNT);
            assert(!seen[tools[i]]);
            seen[tools[i]] = 1;
            assert(editor_workspace_exposes_tool((EditorWorkspace)workspace, tools[i]));
        }
        for (int tool = EDITOR_TOOL_SELECT; tool < EDITOR_TOOL_COUNT; tool++) {
            assert(seen[tool] == editor_workspace_exposes_tool(
                (EditorWorkspace)workspace, (EditorTool)tool));
        }
    }
    assert(app_workspace_tool_surface(EDITOR_WORKSPACE_COUNT, NULL, 0) == 0);
}

static void test_supported_view_transition_does_not_touch_camera(void)
{
    SiteHelperEditor editor;
    AppViews views;
    sitehelper_editor_init(&editor);
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Camera2D initial = {.position = {100.0, 200.0}, .scale = 0.5};
    app_views_init(&views, initial);
    renderer2d_set_camera(renderer, initial);

    assert(app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_FRAMING));
    assert(editor.active_workspace == EDITOR_WORKSPACE_FRAMING);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
    assert_camera(renderer2d_get_camera(renderer), initial);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
}

static void test_cross_workspace_transition_uses_saved_view_cameras(void)
{
    SiteHelperEditor editor;
    AppViews views;
    sitehelper_editor_init(&editor);
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);

    Camera2D initial = {.position = {-20.0, -30.0}, .scale = 0.25};
    Camera2D plan = {.position = {1000.0, 2000.0}, .scale = 0.75};
    Camera2D elevation = {.position = {-500.0, 700.0}, .scale = 1.5};
    app_views_init(&views, initial);
    renderer2d_set_camera(renderer, plan);

    assert(app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_FRAMING));
    assert(app_views_set_active(&views, &editor, renderer,
        EDITOR_VIEW_WALL_ELEVATION));
    assert_camera(renderer2d_get_camera(renderer), initial);
    renderer2d_set_camera(renderer, elevation);

    assert(app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_SLAB));
    assert(editor.active_workspace == EDITOR_WORKSPACE_SLAB);
    assert(editor.active_view == EDITOR_VIEW_PLAN);
    assert_camera(renderer2d_get_camera(renderer), plan);

    assert(app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_FRAMING));
    assert(app_views_set_active(&views, &editor, renderer,
        EDITOR_VIEW_WALL_ELEVATION));
    assert_camera(renderer2d_get_camera(renderer), elevation);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
}

static void test_workspace_transition_applies_editor_transition_policy(void)
{
    SiteHelperEditor editor;
    AppViews views;
    sitehelper_editor_init(&editor);
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Camera2D camera = {.scale = 1.0};
    app_views_init(&views, camera);
    renderer2d_set_camera(renderer, camera);

    assert(app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_SLAB));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_SLAB));
    editor_selection_set_slab(&editor.selection, EDITOR_SELECTION_SCOPE_PLAN, 42);

    assert(app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_DOCUMENTATION));
    assert(editor.active_tool == EDITOR_TOOL_SELECT);
    assert(editor_selection_is_empty(&editor.selection));

    EditorWorkspace before_workspace = editor.active_workspace;
    EditorView before_view = editor.active_view;
    Camera2D before_camera = renderer2d_get_camera(renderer);
    assert(!app_workspace_set_active(&views, &editor, renderer,
        EDITOR_WORKSPACE_COUNT));
    assert(editor.active_workspace == before_workspace);
    assert(editor.active_view == before_view);
    assert_camera(renderer2d_get_camera(renderer), before_camera);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
}

int main(void)
{
    test_user_choices_exclude_compatibility_workspace();
    test_command_surfaces_match_workspace_exposure();
    test_supported_view_transition_does_not_touch_camera();
    test_cross_workspace_transition_uses_saved_view_cameras();
    test_workspace_transition_applies_editor_transition_policy();
    printf("All application workspace tests passed.\n");
    return 0;
}
