#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"
#include "editor_tool.h"

typedef struct
{
    DomainId current_room_id;
    DomainId current_wall_id;

    EditorTool active_tool;
} SiteHelperEditor;

void sitehelper_editor_init(
    SiteHelperEditor *editor
);

int sitehelper_editor_set_active_tool(
    SiteHelperEditor *editor,
    EditorTool tool
);

EditorTool sitehelper_editor_get_active_tool(
    const SiteHelperEditor *editor
);

#endif