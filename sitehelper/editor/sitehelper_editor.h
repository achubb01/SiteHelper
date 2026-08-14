#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"
#include "editor_tool.h"
#include "editor_selection.h"

typedef struct
{
    DomainId current_room_id;
    DomainId current_wall_id;

    EditorTool active_tool;

    EditorSelection selection;
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

void sitehelper_editor_clear_selection(
    SiteHelperEditor *editor
);

void sitehelper_editor_select_wall_member_at_position(
    SiteHelperEditor *editor,
    const Wall *wall,
    Position position
);

void sitehelper_editor_reconcile_wall_selection(
    SiteHelperEditor *editor,
    const Wall *wall
);

const EditorSelection *
sitehelper_editor_get_selection(
    const SiteHelperEditor *editor
);

#endif