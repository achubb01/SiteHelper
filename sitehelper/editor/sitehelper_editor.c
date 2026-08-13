#include <stdlib.h>
#include "sitehelper_editor.h"

int sitehelper_editor_set_active_tool(
    SiteHelperEditor *editor,
    EditorTool tool
)
{
    if (
        editor == NULL
        || tool < EDITOR_TOOL_SELECT
        || tool >= EDITOR_TOOL_COUNT
    ) {
        return 0;
    }

    editor->active_tool = tool;

    return 1;
}

void sitehelper_editor_init(
    SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    *editor = (SiteHelperEditor){
        .current_room_id = DOMAIN_ID_INVALID,
        .current_wall_id = DOMAIN_ID_INVALID,
        .active_tool = EDITOR_TOOL_SELECT
    };
}

EditorTool sitehelper_editor_get_active_tool(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return EDITOR_TOOL_SELECT;
    }

    return editor->active_tool;
}