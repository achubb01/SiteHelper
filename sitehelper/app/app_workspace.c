#include "app_workspace.h"

static int valid_workspace(EditorWorkspace workspace)
{
    return workspace >= EDITOR_WORKSPACE_GENERAL && workspace < EDITOR_WORKSPACE_COUNT;
}

int app_workspace_set_active(
    AppViews *views,
    SiteHelperEditor *editor,
    Renderer2D *renderer,
    EditorWorkspace workspace
)
{
    if (views == NULL || editor == NULL || renderer == NULL ||
        !valid_workspace(workspace)) {
        return 0;
    }
    if (editor->active_workspace == workspace) {
        return 1;
    }

    EditorView previous_view = editor->active_view;
    if (!editor_workspace_supports_view(workspace, previous_view)) {
        EditorView preferred_view = editor_workspace_preferred_view(workspace);
        if (preferred_view < EDITOR_VIEW_PLAN || preferred_view >= EDITOR_VIEW_COUNT ||
            !app_views_set_active(views, editor, renderer, preferred_view)) {
            return 0;
        }
    }

    if (sitehelper_editor_set_active_workspace(editor, workspace)) {
        return 1;
    }

    /* The destination was validated before the view transition, so this is a
     * defensive rollback path rather than normal control flow. */
    if (editor->active_view != previous_view) {
        (void)app_views_set_active(views, editor, renderer, previous_view);
    }
    return 0;
}

size_t app_workspace_user_choices(EditorWorkspace *workspaces, size_t capacity)
{
    static const EditorWorkspace choices[] = {
        EDITOR_WORKSPACE_FRAMING,
        EDITOR_WORKSPACE_SLAB,
        EDITOR_WORKSPACE_ROOF,
        EDITOR_WORKSPACE_DOCUMENTATION
    };
    const size_t count = sizeof choices / sizeof choices[0];
    if (workspaces != NULL) {
        size_t copy_count = capacity < count ? capacity : count;
        for (size_t i = 0; i < copy_count; i++) {
            workspaces[i] = choices[i];
        }
    }
    return count;
}

static size_t copy_tools(const EditorTool *source, size_t count,
    EditorTool *tools, size_t capacity)
{
    if (tools != NULL) {
        size_t copy_count = capacity < count ? capacity : count;
        for (size_t i = 0; i < copy_count; i++) {
            tools[i] = source[i];
        }
    }
    return count;
}

size_t app_workspace_tool_surface(
    EditorWorkspace workspace,
    EditorTool *tools,
    size_t capacity
)
{
    static const EditorTool general[] = {
        EDITOR_TOOL_SELECT,
        EDITOR_TOOL_OPENING,
        EDITOR_TOOL_WALL,
        EDITOR_TOOL_MEASURE,
        EDITOR_TOOL_SLAB,
        EDITOR_TOOL_SLAB_PENETRATION,
        EDITOR_TOOL_SLAB_REGION,
        EDITOR_TOOL_SLAB_EDGE_REBATE,
        EDITOR_TOOL_SLAB_GEOMETRY,
        EDITOR_TOOL_NOTE,
        EDITOR_TOOL_DIMENSION,
        EDITOR_TOOL_SYMBOL,
        EDITOR_TOOL_CALLOUT,
        EDITOR_TOOL_VIEW_DIRECTION,
        EDITOR_TOOL_REVISION_CLOUD
    };
    static const EditorTool framing[] = {
        EDITOR_TOOL_SELECT,
        EDITOR_TOOL_WALL,
        EDITOR_TOOL_OPENING,
        EDITOR_TOOL_MEASURE,
        EDITOR_TOOL_DIMENSION
    };
    static const EditorTool slab[] = {
        EDITOR_TOOL_SELECT,
        EDITOR_TOOL_SLAB,
        EDITOR_TOOL_SLAB_PENETRATION,
        EDITOR_TOOL_SLAB_REGION,
        EDITOR_TOOL_SLAB_EDGE_REBATE,
        EDITOR_TOOL_SLAB_GEOMETRY,
        EDITOR_TOOL_MEASURE,
        EDITOR_TOOL_DIMENSION
    };
    static const EditorTool roof[] = {
        EDITOR_TOOL_SELECT,
        EDITOR_TOOL_MEASURE,
        EDITOR_TOOL_DIMENSION
    };
    static const EditorTool documentation[] = {
        EDITOR_TOOL_SELECT,
        EDITOR_TOOL_NOTE,
        EDITOR_TOOL_DIMENSION,
        EDITOR_TOOL_SYMBOL,
        EDITOR_TOOL_VIEW_DIRECTION,
        EDITOR_TOOL_CALLOUT,
        EDITOR_TOOL_REVISION_CLOUD,
        EDITOR_TOOL_MEASURE
    };

    switch (workspace) {
        case EDITOR_WORKSPACE_GENERAL:
            return copy_tools(general, sizeof general / sizeof general[0], tools, capacity);
        case EDITOR_WORKSPACE_FRAMING:
            return copy_tools(framing, sizeof framing / sizeof framing[0], tools, capacity);
        case EDITOR_WORKSPACE_SLAB:
            return copy_tools(slab, sizeof slab / sizeof slab[0], tools, capacity);
        case EDITOR_WORKSPACE_ROOF:
            return copy_tools(roof, sizeof roof / sizeof roof[0], tools, capacity);
        case EDITOR_WORKSPACE_DOCUMENTATION:
            return copy_tools(documentation,
                sizeof documentation / sizeof documentation[0], tools, capacity);
        case EDITOR_WORKSPACE_COUNT:
        default:
            return 0;
    }
}

const char *app_workspace_short_label(EditorWorkspace workspace)
{
    switch (workspace) {
        case EDITOR_WORKSPACE_GENERAL: return "GEN";
        case EDITOR_WORKSPACE_FRAMING: return "FRM";
        case EDITOR_WORKSPACE_SLAB: return "SLB";
        case EDITOR_WORKSPACE_ROOF: return "ROOF";
        case EDITOR_WORKSPACE_DOCUMENTATION: return "DOC";
        case EDITOR_WORKSPACE_COUNT:
        default: return "?";
    }
}

const char *app_workspace_tool_short_label(EditorTool tool)
{
    switch (tool) {
        case EDITOR_TOOL_SELECT: return "Sel";
        case EDITOR_TOOL_OPENING: return "Open";
        case EDITOR_TOOL_WALL: return "Wall";
        case EDITOR_TOOL_MEASURE: return "Meas";
        case EDITOR_TOOL_SLAB: return "Slab";
        case EDITOR_TOOL_SLAB_PENETRATION: return "Void";
        case EDITOR_TOOL_SLAB_REGION: return "Reg";
        case EDITOR_TOOL_SLAB_EDGE_REBATE: return "Rebt";
        case EDITOR_TOOL_SLAB_GEOMETRY: return "Geom";
        case EDITOR_TOOL_NOTE: return "Note";
        case EDITOR_TOOL_DIMENSION: return "Dim";
        case EDITOR_TOOL_SYMBOL: return "Mark";
        case EDITOR_TOOL_CALLOUT: return "Call";
        case EDITOR_TOOL_VIEW_DIRECTION: return "View";
        case EDITOR_TOOL_REVISION_CLOUD: return "Rev";
        case EDITOR_TOOL_COUNT:
        default: return "?";
    }
}
