#ifndef EDITOR_CONTEXT_H
#define EDITOR_CONTEXT_H

#include "editor_tool.h"
#include "editor_selection.h"

/* Presentation/coordinate-space state; transient and independent of workspace. */
typedef enum
{
    EDITOR_VIEW_PLAN,
    EDITOR_VIEW_WALL_ELEVATION,
    EDITOR_VIEW_COUNT
} EditorView;

/* Task/domain intent for the editor UI. This is transient navigation state,
 * never project authority or persistence. GENERAL preserves the pre-workspace
 * command surface until the application provides explicit workspace switching. */
typedef enum
{
    EDITOR_WORKSPACE_GENERAL,
    EDITOR_WORKSPACE_FRAMING,
    EDITOR_WORKSPACE_SLAB,
    EDITOR_WORKSPACE_ROOF,
    EDITOR_WORKSPACE_DOCUMENTATION,
    EDITOR_WORKSPACE_COUNT
} EditorWorkspace;

/* Mechanical coordinate/presentation capability only. */
int editor_view_supports_tool(EditorView view, EditorTool tool);

/* Workspace/view compatibility is independent of camera ownership. */
int editor_workspace_supports_view(EditorWorkspace workspace, EditorView view);
EditorView editor_workspace_preferred_view(EditorWorkspace workspace);

/* UX/domain exposure only; does not imply the current view can run the tool. */
int editor_workspace_exposes_tool(EditorWorkspace workspace, EditorTool tool);

/* Selection target policy. GENERAL preserves the legacy mixed Plan selection;
 * focused workspaces accept only targets from their construction/document domain. */
int editor_workspace_accepts_selection(EditorWorkspace workspace,
    EditorSelectionKind kind);

/* Effective interaction requires all context predicates to hold. */
int editor_context_tool_available(EditorWorkspace workspace, EditorView view,
    EditorTool tool);

#endif
