#include <stdlib.h>
#include "sitehelper_editor.h"
#include "wall_query.h"
#include "wall_snap.h"

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

    if (tool == EDITOR_TOOL_OPENING) {
        opening_tool_activate(
            &editor->opening_tool
        );
    }
    else {
        opening_tool_cancel(
            &editor->opening_tool
        );

        editor->opening_placement =
            (OpeningPlacement){0};
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

    editor_selection_init(
        &editor->selection
    );

    editor_snap_state_init(
        &editor->snap
    );

    opening_tool_init(
        &editor->opening_tool
    );

    editor->opening_placement =
        (OpeningPlacement){0};
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

void sitehelper_editor_clear_selection(
    SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    editor_selection_clear(
        &editor->selection
    );
}

void sitehelper_editor_select_wall_member_at_position(
    SiteHelperEditor *editor,
    const Wall *wall,
    Position position
)
{
    if (
        editor == NULL
        || wall == NULL
        || wall->id == DOMAIN_ID_INVALID
    ) {
        return;
    }

    WallMemberHit hit =
        wall_find_member_at_position(
            wall,
            position
        );

    editor_selection_set_wall_member(
        &editor->selection,
        wall->id,
        hit.kind,
        hit.timber
    );
}

void sitehelper_editor_reconcile_wall_selection(
    SiteHelperEditor *editor,
    const Wall *wall
)
{
    if (
        editor == NULL
        || wall == NULL
    ) {
        return;
    }

    const WallSelection *wall_selection =
        editor_selection_get_wall_member(
            &editor->selection,
            wall->id
        );

    if (wall_selection == NULL) {
        return;
    }

    wall_selection_reconcile(
        &editor->selection.wall_member,
        wall
    );

    if (
        wall_selection_is_empty(
            &editor->selection.wall_member
        )
    ) {
        editor_selection_clear(
            &editor->selection
        );
    }
}

const EditorSelection *
sitehelper_editor_get_selection(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return NULL;
    }

    return &editor->selection;
}

const SnapResult *
sitehelper_editor_get_snap_result(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return NULL;
    }

    return editor_snap_state_get_result(
        &editor->snap
    );
}

const SnapSettings *
sitehelper_editor_get_snap_settings(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return NULL;
    }

    return editor_snap_state_get_settings(
        &editor->snap
    );
}

int sitehelper_editor_has_snap(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return 0;
    }

    return editor_snap_state_has_snap(
        &editor->snap
    );
}

void sitehelper_editor_clear_snap(
    SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    editor_snap_state_clear(
        &editor->snap
    );
}

void sitehelper_editor_set_snap_result(
    SiteHelperEditor *editor,
    SnapResult result
)
{
    if (editor == NULL) {
        return;
    }

    editor_snap_state_set_result(
        &editor->snap,
        result
    );
}

void sitehelper_editor_update_snap(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 world_position
)
{
    if (editor == NULL) {
        return;
    }

    enum {
        MAX_SNAP_CANDIDATES = 256
    };

    SnapCandidate candidates[
        MAX_SNAP_CANDIDATES
    ];

    size_t candidate_count = 0;

    if (wall != NULL) {
        candidate_count =
            wall_collect_snap_candidates(
                wall,
                candidates,
                MAX_SNAP_CANDIDATES
            );
    }

    const SnapSettings *settings =
        editor_snap_state_get_settings(
            &editor->snap
        );

    if (settings == NULL) {
        editor_snap_state_clear(
            &editor->snap
        );

        return;
    }

    SnapResult result =
        editor_snap(
            world_position,
            candidates,
            candidate_count,
            settings
        );

    editor_snap_state_set_result(
        &editor->snap,
        result
    );
}

void sitehelper_editor_pointer_move(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 world_position
)
{
    if (editor == NULL) {
        return;
    }

    sitehelper_editor_update_snap(
        editor,
        wall,
        world_position
    );

    switch (editor->active_tool) {
        case EDITOR_TOOL_OPENING:
        {
            const SnapResult *snap_result =
                editor_snap_state_get_result(
                    &editor->snap
                );

            if (
                wall == NULL
                || snap_result == NULL
                || snap_result->type == SNAP_NONE
            ) {
                editor->opening_placement =
                    (OpeningPlacement){0};

                return;
            }

            opening_tool_update_preview(
                &editor->opening_tool,
                snap_result->position
            );

            editor->opening_placement =
                opening_find_placement(
                    wall,
                    snap_result->position,
                    &editor->opening_tool
                );

            break;
        }

        case EDITOR_TOOL_SELECT:
        case EDITOR_TOOL_WALL:
        default:
            editor->opening_placement =
                (OpeningPlacement){0};
            break;
    }
}

const OpeningPlacement *
sitehelper_editor_get_opening_placement(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return NULL;
    }

    return &editor->opening_placement;
}

void sitehelper_editor_pointer_leave(
    SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    sitehelper_editor_clear_snap(
        editor
    );

    editor->opening_placement =
        (OpeningPlacement){0};
}

int sitehelper_editor_create_opening_command(
    const SiteHelperEditor *editor,
    DomainId opening_id,
    OpeningCommand *command
)
{
    if (
        editor == NULL
        || command == NULL
        || editor->active_tool
            != EDITOR_TOOL_OPENING
    ) {
        return 0;
    }

    return opening_command_create(
        &editor->opening_placement,
        editor->opening_tool.type,
        opening_id,
        command
    );
}

void sitehelper_editor_complete_opening_command(
    SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    editor->opening_placement =
        (OpeningPlacement){0};
}

int sitehelper_editor_primary_action(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 world_position,
    DomainId command_id,
    EditorAction *action
)
{
    if (
        editor == NULL
        || action == NULL
    ) {
        return 0;
    }

    *action = (EditorAction){
        .kind = EDITOR_ACTION_NONE
    };

    switch (editor->active_tool) {
        case EDITOR_TOOL_SELECT:
        {
            if (wall == NULL) {
                return 1;
            }

            Position position = {
                .x = (int)world_position.x,
                .y = (int)world_position.y
            };

            sitehelper_editor_select_wall_member_at_position(
                editor,
                wall,
                position
            );

            return 1;
        }

        case EDITOR_TOOL_OPENING:
        {
            if (!sitehelper_editor_create_opening_command(
                    editor,
                    command_id,
                    &action->opening_command)) {
                return 0;
            }

            action->kind =
                EDITOR_ACTION_OPENING_COMMAND;

            return 1;
        }
        
        case EDITOR_TOOL_WALL:
        default:
            return 1;
    }
}

int sitehelper_editor_primary_action_requires_id(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return 0;
    }

    switch (editor->active_tool) {
        case EDITOR_TOOL_OPENING:
            return 1;

        case EDITOR_TOOL_SELECT:
        case EDITOR_TOOL_WALL:
        default:
            return 0;
    }
}

int sitehelper_editor_has_opening_preview(
    const SiteHelperEditor *editor
)
{
    if (editor == NULL) {
        return 0;
    }

    return
        editor->active_tool
            == EDITOR_TOOL_OPENING
        && editor->opening_placement.valid;
}

int sitehelper_editor_get_opening_preview_rect(
    const SiteHelperEditor *editor,
    Rect2 *rect
)
{
    if (
        editor == NULL
        || rect == NULL
        || !sitehelper_editor_has_opening_preview(
            editor
        )
    ) {
        return 0;
    }

    *rect = (Rect2){
        .position = {
            .x = editor->opening_placement.left,
            .y = editor->opening_placement.bottom
        },

        .width =
            (double)editor->opening_placement.width,

        .height =
            (double)editor->opening_placement.height
    };

    return 1;
}