#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"
#include "editor_tool.h"
#include "editor_selection.h"
#include "editor_snap_state.h"
#include "opening_tool.h"
#include "opening_placement.h"
#include "opening_command.h"
#include "editor_action.h"

typedef struct
{
    DomainId current_room_id;
    DomainId current_wall_id;

    EditorTool active_tool;

    EditorSelection selection;
    EditorSnapState snap;

    OpeningTool opening_tool;
    OpeningPlacement opening_placement;
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

const SnapResult *
sitehelper_editor_get_snap_result(
    const SiteHelperEditor *editor
);

const SnapSettings *
sitehelper_editor_get_snap_settings(
    const SiteHelperEditor *editor
);

int sitehelper_editor_has_snap(
    const SiteHelperEditor *editor
);

void sitehelper_editor_clear_snap(
    SiteHelperEditor *editor
);

void sitehelper_editor_set_snap_result(
    SiteHelperEditor *editor,
    SnapResult result
);

void sitehelper_editor_update_snap(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 world_position
);

void sitehelper_editor_pointer_move(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 world_position
);

const OpeningPlacement *
sitehelper_editor_get_opening_placement(
    const SiteHelperEditor *editor
);

void sitehelper_editor_pointer_leave(
    SiteHelperEditor *editor
);

int sitehelper_editor_create_opening_command(
    const SiteHelperEditor *editor,
    DomainId opening_id,
    OpeningCommand *command
);

void sitehelper_editor_complete_opening_command(
    SiteHelperEditor *editor
);

int sitehelper_editor_primary_action(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 world_position,
    DomainId command_id,
    EditorAction *action
);

#endif