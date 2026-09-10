#include <stdlib.h>
#include <math.h>
#include "sitehelper_editor.h"
#include "wall_query.h"
#include "wall_plan_transform.h"
#include "wall_snap.h"

int sitehelper_editor_tool_available(EditorView view, EditorTool tool)
{
    return (view == EDITOR_VIEW_PLAN &&
            (tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_WALL)) ||
        (view == EDITOR_VIEW_WALL_ELEVATION &&
            (tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_OPENING));
}

int sitehelper_editor_set_active_view(SiteHelperEditor *editor, EditorView view)
{
    if (editor == NULL || view < EDITOR_VIEW_PLAN || view >= EDITOR_VIEW_COUNT) {
        return 0;
    }
    if (editor->active_view == view) {
        return 1;
    }
    editor->active_view = view;
    sitehelper_editor_invalidate_transient_state(editor);
    sitehelper_editor_clear_selection(editor);
    if (!sitehelper_editor_tool_available(view, editor->active_tool)) {
        sitehelper_editor_set_active_tool(editor, EDITOR_TOOL_SELECT);
    }
    return 1;
}

/* Physical plan proximity; shares the editor's object tolerance. */
static double plan_segment_distance(WallPlanSegment segment, Vec2 point)
{
    PlanPoint plan_point = { .x = point.x, .y = point.y };
    double u;
    if (!wall_plan_segment_plan_to_u(segment, plan_point, &u)) {
        return INFINITY;
    }
    u = fmax(0.0, fmin((double)wall_plan_segment_length_mm(segment), u));
    PlanPoint nearest;
    if (!wall_plan_segment_u_to_plan(segment, u, &nearest)) {
        return INFINITY;
    }
    return hypot(plan_point.x - nearest.x, plan_point.y - nearest.y);
}

int sitehelper_editor_set_active_tool(
    SiteHelperEditor *editor,
    EditorTool tool
)
{
    if (
        editor == NULL
        || tool < EDITOR_TOOL_SELECT
        || tool >= EDITOR_TOOL_COUNT
        || !sitehelper_editor_tool_available(editor->active_view, tool)
    ) {
        return 0;
    }

    sitehelper_editor_invalidate_transient_state(editor);

    if (tool == EDITOR_TOOL_OPENING) {
        opening_tool_activate(
            &editor->opening_tool
        );
    }
    else {
        opening_tool_cancel(&editor->opening_tool);
        editor->opening_placement = (OpeningPlacement){0};
    }

    if (tool == EDITOR_TOOL_WALL) {
        wall_tool_activate(&editor->wall_tool);
    }
    else {
        wall_tool_cancel(&editor->wall_tool);
        editor->wall_tool.active = 0;
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
        .active_view = EDITOR_VIEW_PLAN,
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

    wall_tool_init(&editor->wall_tool);

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
    WallLocalPosition position
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

void sitehelper_editor_reconcile(
    SiteHelperEditor *editor,
    const SiteHelperProject *project
)
{
    if (editor == NULL) {
        return;
    }

    if (project == NULL) {
        editor->current_room_id = DOMAIN_ID_INVALID;
        editor->current_wall_id = DOMAIN_ID_INVALID;
        sitehelper_editor_clear_selection(editor);
        sitehelper_editor_invalidate_transient_state(editor);
        return;
    }

    if (build_find_room_by_id_const(&project->structure, editor->current_room_id) == NULL) {
        editor->current_room_id = DOMAIN_ID_INVALID;
    }
    const Wall *current_wall = build_find_wall_by_id_const(
        &project->structure, editor->current_wall_id);

    if (current_wall == NULL) {
        editor->current_wall_id = DOMAIN_ID_INVALID;
    }

    const EditorSelection *selection = &editor->selection;

    if (selection->kind == EDITOR_SELECTION_WALL_MEMBER) {
        const Wall *selected_wall = build_find_wall_by_id_const(
            &project->structure, selection->wall_id);

        if (selected_wall == NULL) {
            sitehelper_editor_clear_selection(editor);
        }
        else {
            wall_selection_reconcile(
                &editor->selection.wall_member,
                selected_wall
            );

            if (wall_selection_is_empty(&editor->selection.wall_member)) {
                sitehelper_editor_clear_selection(editor);
            }
        }
    }

    sitehelper_editor_invalidate_transient_state(editor);
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
    Vec2 position
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

    if (editor->active_view == EDITOR_VIEW_WALL_ELEVATION && wall != NULL) {
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
            position,
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
    const BuildSettings *settings,
    Vec2 view_position
)
{
    if (editor == NULL) {
        return;
    }

    if (editor->active_tool == EDITOR_TOOL_WALL) {
        sitehelper_editor_update_snap(editor, NULL, view_position);

        const SnapResult *snap_result = editor_snap_state_get_result(
            &editor->snap
        );

        if (snap_result != NULL && snap_result->type != SNAP_NONE) {
            wall_tool_update(&editor->wall_tool, snap_result->position);
        }

        editor->opening_placement = (OpeningPlacement){0};
        return;
    }

    sitehelper_editor_update_snap(editor, wall, view_position);

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
                    snap_result->position,
                    &editor->opening_tool
                );

            if (
                editor->opening_placement.has_candidate
            ) {
                WallOpeningProposal proposal = {
                    .type = editor->opening_tool.type,
                    .frame_position =
                        (int)editor->opening_placement.left,
                    .frame_bottom =
                        (int)editor->opening_placement.bottom,
                    .width = editor->opening_placement.width,
                    .height = editor->opening_placement.height
                };

                editor->opening_placement.validation =
                    wall_validate_opening(
                        wall,
                        settings,
                        &proposal
                    );
            }

            break;
        }

        case EDITOR_TOOL_SELECT:
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

    sitehelper_editor_invalidate_transient_state(
        editor
    );
}

int sitehelper_editor_create_opening_command(
    const SiteHelperEditor *editor,
    OpeningCommand *command
)
{
    if (
        editor == NULL
        || command == NULL
        || editor->active_tool
            != EDITOR_TOOL_OPENING
        || !opening_placement_is_valid(
            &editor->opening_placement
        )
    ) {
        return 0;
    }

    return opening_command_create(
        editor->current_wall_id,
        editor->opening_tool.type,
        (int)editor->opening_placement.left,
        (int)editor->opening_placement.bottom,
        editor->opening_placement.width,
        editor->opening_placement.height,
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
    Vec2 view_position,
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

            if (editor->active_view == EDITOR_VIEW_PLAN) {
                sitehelper_editor_clear_selection(editor);
                editor->current_wall_id = plan_segment_distance(
                    wall->definition.segment, view_position
                ) <= editor->snap.settings.object_snap_tolerance
                    ? wall->id : DOMAIN_ID_INVALID;
                return 1;
            }

            WallLocalPosition position = {
                .u = (int)view_position.x, .z = (int)view_position.y
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
            OpeningCommand opening_command;

            if (!sitehelper_editor_create_opening_command(
                    editor,
                    &opening_command)) {
                return 0;
            }

            if (!sitehelper_command_from_opening(
                    &opening_command,
                    &action->command)) {
                return 0;
            }

            action->kind =
                EDITOR_ACTION_COMMAND;

            return 1;
        }
        
        case EDITOR_TOOL_WALL:
        {
            const SnapResult *snap_result = editor_snap_state_get_result(
                &editor->snap
            );

            Vec2 position = snap_result != NULL && snap_result->type != SNAP_NONE
                ? snap_result->position
                : view_position;

            if (!editor->wall_tool.has_start) {
                return wall_tool_begin(&editor->wall_tool, position);
            }

            wall_tool_update(&editor->wall_tool, position);
            WallPlanSegment segment;
            WallCommand wall_command;

            if (!wall_tool_command_data(
                    &editor->wall_tool,
                    &segment) ||
                !wall_command_create(
                    segment,
                    &wall_command) ||
                !sitehelper_command_from_wall(
                    &wall_command,
                    &action->command)) {

                return 0;
            }

            action->kind = EDITOR_ACTION_COMMAND;
            return 1;
        }

        default:
            return 1;
    }
}

int sitehelper_editor_primary_action_in_project(
    SiteHelperEditor *editor,
    const BuildStructure *structure,
    Vec2 view_position,
    EditorAction *action
)
{
    if (editor == NULL || structure == NULL || action == NULL) {
        return 0;
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_SELECT) {
        *action = (EditorAction){ .kind = EDITOR_ACTION_NONE };
        sitehelper_editor_clear_selection(editor);
        sitehelper_editor_invalidate_transient_state(editor);
        editor->current_wall_id = DOMAIN_ID_INVALID;
        double nearest = editor->snap.settings.object_snap_tolerance;

        /* Nearest segment wins; later appended walls win exact ties. */
        for (size_t index = structure->wall_count; index > 0; index--) {
            const Wall *wall = &structure->walls[index - 1];
            double distance = plan_segment_distance(wall->definition.segment, view_position);
            if (distance <= nearest &&
                (editor->current_wall_id == DOMAIN_ID_INVALID || distance < nearest)) {
                nearest = distance;
                editor->current_wall_id = wall->id;
            }
        }
        return 1;
    }

    const Wall *wall = build_find_wall_by_id_const(
        structure, editor->current_wall_id);

    return sitehelper_editor_primary_action(
        editor,
        wall,
        view_position,
        action
    );
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
        && editor->opening_placement.has_candidate;
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

void sitehelper_editor_complete_action(
    SiteHelperEditor *editor,
    const EditorAction *action,
    const SiteHelperCommandResult *result
)
{
    if (
        editor == NULL
        || action == NULL
        || result == NULL
        || action->command.type != result->type
    ) {
        return;
    }

    if (
        action->kind
        != EDITOR_ACTION_COMMAND
    ) {
        return;
    }

    switch (action->command.type) {

        case SITEHELPER_COMMAND_ADD_OPENING:

            sitehelper_editor_complete_opening_command(
                editor
            );

            break;

        case SITEHELPER_COMMAND_ADD_WALL:
            editor->current_wall_id = result->data.add_wall.wall_id;
            wall_tool_cancel(&editor->wall_tool);
            break;

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            break;
    }
}

void sitehelper_editor_invalidate_transient_state(
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

    editor->opening_tool.preview_valid = 0;
    wall_tool_cancel(&editor->wall_tool);
}

int sitehelper_editor_has_wall_preview(const SiteHelperEditor *editor)
{
    return editor != NULL && editor->active_tool == EDITOR_TOOL_WALL &&
        editor->wall_tool.has_start;
}

int sitehelper_editor_get_wall_preview_segment(
    const SiteHelperEditor *editor,
    WallPlanSegment *segment
)
{
    return editor != NULL &&
        wall_tool_command_data(&editor->wall_tool, segment);
}
