#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"
#include "editor_tool.h"
#include "editor_selection.h"
#include "editor_snap_state.h"
#include "opening_tool.h"
#include "opening_placement.h"
#include "wall_tool.h"
#include "opening_command.h"
#include "editor_action.h"
#include "sitehelper_project.h"

typedef struct
{
    DomainId current_room_id;
    DomainId current_wall_id;

    EditorTool active_tool;

    EditorSelection selection;
    EditorSnapState snap;

    OpeningTool opening_tool;
    OpeningPlacement opening_placement;
    WallTool wall_tool;
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
    WallLocalPosition position
);

void sitehelper_editor_reconcile_wall_selection(
    SiteHelperEditor *editor,
    const Wall *wall
);

void sitehelper_editor_reconcile(
    SiteHelperEditor *editor,
    const SiteHelperProject *project
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

/* With a wall, position is local elevation Vec2 (x = U, y = Z).
 * Without a wall, position is in the active tool's render layout. */
void sitehelper_editor_update_snap(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 position
);

/* Pointer APIs receive render layout coordinates, before camera projection. */
void sitehelper_editor_pointer_move(
    SiteHelperEditor *editor,
    const Wall *wall,
    const BuildSettings *settings,
    Vec2 layout_position
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
    OpeningCommand *command
);

void sitehelper_editor_complete_opening_command(
    SiteHelperEditor *editor
);

int sitehelper_editor_primary_action(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 layout_position,
    EditorAction *action
);

int sitehelper_editor_primary_action_in_room(
    SiteHelperEditor *editor,
    const BuildStructure *structure,
    const Room *room,
    Vec2 layout_position,
    EditorAction *action
);

int sitehelper_editor_has_opening_preview(
    const SiteHelperEditor *editor
);

int sitehelper_editor_get_opening_preview_rect(
    const SiteHelperEditor *editor,
    Rect2 *rect
);

void sitehelper_editor_complete_action(
    SiteHelperEditor *editor,
    const EditorAction *action,
    const SiteHelperCommandResult *result
);

int sitehelper_editor_has_wall_preview(const SiteHelperEditor *editor);
int sitehelper_editor_get_wall_preview_segment(
    const SiteHelperEditor *editor,
    WallPlanSegment *segment
);

void sitehelper_editor_invalidate_transient_state(
    SiteHelperEditor *editor
);

#endif
