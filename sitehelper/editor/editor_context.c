#include "editor_context.h"

static int valid_view(EditorView view)
{
    return view >= EDITOR_VIEW_PLAN && view < EDITOR_VIEW_COUNT;
}

static int valid_workspace(EditorWorkspace workspace)
{
    return workspace >= EDITOR_WORKSPACE_GENERAL && workspace < EDITOR_WORKSPACE_COUNT;
}

static int valid_tool(EditorTool tool)
{
    return tool >= EDITOR_TOOL_SELECT && tool < EDITOR_TOOL_COUNT;
}

int editor_view_supports_tool(EditorView view, EditorTool tool)
{
    if (!valid_view(view) || !valid_tool(tool)) { return 0; }

    if (view == EDITOR_VIEW_PLAN) {
        return tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_WALL ||
            tool == EDITOR_TOOL_MEASURE || tool == EDITOR_TOOL_SLAB ||
            tool == EDITOR_TOOL_SLAB_PENETRATION || tool == EDITOR_TOOL_SLAB_REGION ||
            tool == EDITOR_TOOL_SLAB_EDGE_REBATE || tool == EDITOR_TOOL_SLAB_GEOMETRY ||
            tool == EDITOR_TOOL_NOTE || tool == EDITOR_TOOL_DIMENSION ||
            tool == EDITOR_TOOL_SYMBOL || tool == EDITOR_TOOL_VIEW_DIRECTION ||
            tool == EDITOR_TOOL_CALLOUT || tool == EDITOR_TOOL_REVISION_CLOUD;
    }

    return tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_OPENING;
}

int editor_workspace_supports_view(EditorWorkspace workspace, EditorView view)
{
    if (!valid_workspace(workspace) || !valid_view(view)) { return 0; }

    switch (workspace) {
        case EDITOR_WORKSPACE_GENERAL:
        case EDITOR_WORKSPACE_FRAMING:
            return view == EDITOR_VIEW_PLAN || view == EDITOR_VIEW_WALL_ELEVATION;
        case EDITOR_WORKSPACE_SLAB:
        case EDITOR_WORKSPACE_ROOF:
        case EDITOR_WORKSPACE_DOCUMENTATION:
            return view == EDITOR_VIEW_PLAN;
        case EDITOR_WORKSPACE_COUNT:
        default:
            return 0;
    }
}

EditorView editor_workspace_preferred_view(EditorWorkspace workspace)
{
    if (!valid_workspace(workspace)) { return EDITOR_VIEW_COUNT; }
    return EDITOR_VIEW_PLAN;
}

int editor_workspace_exposes_tool(EditorWorkspace workspace, EditorTool tool)
{
    if (!valid_workspace(workspace) || !valid_tool(tool)) { return 0; }

    switch (workspace) {
        case EDITOR_WORKSPACE_GENERAL:
            /* Compatibility surface for 32B. 32C replaces the fixed global
             * toolbar with explicit workspace switching. */
            return 1;

        case EDITOR_WORKSPACE_FRAMING:
            return tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_WALL ||
                tool == EDITOR_TOOL_OPENING || tool == EDITOR_TOOL_MEASURE ||
                tool == EDITOR_TOOL_DIMENSION;

        case EDITOR_WORKSPACE_SLAB:
            return tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_MEASURE ||
                tool == EDITOR_TOOL_SLAB || tool == EDITOR_TOOL_SLAB_PENETRATION ||
                tool == EDITOR_TOOL_SLAB_REGION || tool == EDITOR_TOOL_SLAB_EDGE_REBATE ||
                tool == EDITOR_TOOL_SLAB_GEOMETRY || tool == EDITOR_TOOL_DIMENSION;

        case EDITOR_WORKSPACE_ROOF:
            /* Roof source-edit tools do not exist yet. Selection, measurement
             * and dimensions are the current shared interactions. */
            return tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_MEASURE ||
                tool == EDITOR_TOOL_DIMENSION;

        case EDITOR_WORKSPACE_DOCUMENTATION:
            return tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_MEASURE ||
                tool == EDITOR_TOOL_NOTE || tool == EDITOR_TOOL_DIMENSION ||
                tool == EDITOR_TOOL_SYMBOL || tool == EDITOR_TOOL_VIEW_DIRECTION ||
                tool == EDITOR_TOOL_CALLOUT || tool == EDITOR_TOOL_REVISION_CLOUD;

        case EDITOR_WORKSPACE_COUNT:
        default:
            return 0;
    }
}

int editor_workspace_accepts_selection(EditorWorkspace workspace,
    EditorSelectionKind kind)
{
    if (!valid_workspace(workspace) || kind < EDITOR_SELECTION_NONE ||
        kind > EDITOR_SELECTION_DOCUMENT) {
        return 0;
    }
    if (kind == EDITOR_SELECTION_NONE) { return 1; }

    switch (workspace) {
        case EDITOR_WORKSPACE_GENERAL:
            return 1;
        case EDITOR_WORKSPACE_FRAMING:
            return kind == EDITOR_SELECTION_WALL_MEMBER ||
                kind == EDITOR_SELECTION_WALL || kind == EDITOR_SELECTION_OPENING;
        case EDITOR_WORKSPACE_SLAB:
            return kind == EDITOR_SELECTION_SLAB ||
                kind == EDITOR_SELECTION_SLAB_PENETRATION ||
                kind == EDITOR_SELECTION_SLAB_REGION ||
                kind == EDITOR_SELECTION_SLAB_EDGE_REBATE;
        case EDITOR_WORKSPACE_ROOF:
            return kind == EDITOR_SELECTION_ROOF ||
                kind == EDITOR_SELECTION_ROOF_PORTION;
        case EDITOR_WORKSPACE_DOCUMENTATION:
            return kind == EDITOR_SELECTION_DOCUMENT;
        case EDITOR_WORKSPACE_COUNT:
        default:
            return 0;
    }
}

int editor_context_tool_available(EditorWorkspace workspace, EditorView view,
    EditorTool tool)
{
    return editor_workspace_supports_view(workspace, view) &&
        editor_workspace_exposes_tool(workspace, tool) &&
        editor_view_supports_tool(view, tool);
}
