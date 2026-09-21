#ifndef APP_WORKSPACE_H
#define APP_WORKSPACE_H

#include <stddef.h>

#include "app_view.h"
#include "editor_context.h"

/* Application-level workspace transition. The editor owns transient workspace
 * state; AppViews remains responsible for coordinated view/camera changes. */
int app_workspace_set_active(
    AppViews *views,
    SiteHelperEditor *editor,
    Renderer2D *renderer,
    EditorWorkspace workspace
);

/* Workspaces shown in the application switcher. GENERAL is intentionally a
 * compatibility/editor default rather than a user-facing destination. */
size_t app_workspace_user_choices(EditorWorkspace *workspaces, size_t capacity);

/* Ordered command surface for a workspace. This is explicit presentation
 * policy, not EditorTool enum order. Returns the full required count. */
size_t app_workspace_tool_surface(
    EditorWorkspace workspace,
    EditorTool *tools,
    size_t capacity
);

const char *app_workspace_short_label(EditorWorkspace workspace);
const char *app_workspace_tool_short_label(EditorTool tool);

#endif
