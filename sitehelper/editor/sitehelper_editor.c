#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "sitehelper_editor.h"
#include "wall_query.h"
#include "wall_plan_transform.h"
#include "wall_snap.h"
#include "plan_snap.h"
#include "slab_plan_query.h"
#include "roof_plan_query.h"
#include "roof.h"
#include "document_plan_query.h"
#include "plan_position_conversion.h"
#include "plan_dimension_geometry.h"

int sitehelper_editor_set_current_storey(SiteHelperEditor *editor,
    const SiteHelperProject *project, DomainId storey_id)
{
    if (editor == NULL || project == NULL || (storey_id != DOMAIN_ID_INVALID &&
        sitehelper_project_find_storey_by_id_const(project, storey_id) == NULL)) { return 0; }
    if (editor->current_storey_id != storey_id) {
        editor->current_storey_id = storey_id;
        editor->current_room_id = editor->current_wall_id = DOMAIN_ID_INVALID;
        sitehelper_editor_clear_selection(editor);
        sitehelper_editor_invalidate_transient_state(editor);
    }
    sitehelper_editor_reconcile(editor, project);
    return 1;
}

int sitehelper_editor_tool_available(EditorView view, EditorTool tool)
{
    return (view == EDITOR_VIEW_PLAN &&
            (tool == EDITOR_TOOL_SELECT || tool == EDITOR_TOOL_WALL || tool == EDITOR_TOOL_MEASURE ||
             tool == EDITOR_TOOL_SLAB || tool == EDITOR_TOOL_SLAB_PENETRATION ||
             tool == EDITOR_TOOL_SLAB_REGION || tool == EDITOR_TOOL_SLAB_EDGE_REBATE ||
             tool == EDITOR_TOOL_SLAB_GEOMETRY || tool == EDITOR_TOOL_NOTE ||
             tool == EDITOR_TOOL_DIMENSION || tool == EDITOR_TOOL_SYMBOL ||
             tool == EDITOR_TOOL_VIEW_DIRECTION || tool == EDITOR_TOOL_CALLOUT ||
             tool == EDITOR_TOOL_REVISION_CLOUD)) ||
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

    if (tool == EDITOR_TOOL_MEASURE) { measurement_tool_activate(&editor->measurement_tool); }
    else { measurement_tool_init(&editor->measurement_tool); }

    if (tool == EDITOR_TOOL_DIMENSION) { plan_dimension_tool_activate(&editor->dimension_tool); }
    else { plan_dimension_tool_cancel(&editor->dimension_tool); editor->dimension_tool.active=0; }

    if (tool == EDITOR_TOOL_CALLOUT) { plan_callout_tool_activate(&editor->callout_tool); }
    else { plan_callout_tool_cancel(&editor->callout_tool); editor->callout_tool.active=0; }

    if (tool == EDITOR_TOOL_VIEW_DIRECTION) {
        plan_direction_symbol_tool_activate(&editor->direction_symbol_tool);
    } else {
        plan_direction_symbol_tool_cancel(&editor->direction_symbol_tool);
        editor->direction_symbol_tool.active=0;
    }

    if (tool == EDITOR_TOOL_REVISION_CLOUD) {
        plan_revision_cloud_tool_activate(&editor->revision_cloud_tool);
    } else {
        plan_revision_cloud_tool_cancel(&editor->revision_cloud_tool);
        editor->revision_cloud_tool.active=0;
    }

    if (tool == EDITOR_TOOL_SLAB) { slab_tool_activate(&editor->slab_tool); }
    else { slab_tool_cancel(&editor->slab_tool); }

    if (tool == EDITOR_TOOL_SLAB_PENETRATION) {
        slab_polygon_feature_tool_activate(&editor->slab_penetration_tool);
    } else { slab_polygon_feature_tool_cancel(&editor->slab_penetration_tool); }
    if (tool == EDITOR_TOOL_SLAB_REGION) {
        slab_polygon_feature_tool_activate(&editor->slab_region_tool);
    } else { slab_polygon_feature_tool_cancel(&editor->slab_region_tool); }
    if (tool == EDITOR_TOOL_SLAB_EDGE_REBATE) {
        slab_edge_rebate_tool_activate(&editor->slab_edge_rebate_tool);
    } else { slab_edge_rebate_tool_cancel(&editor->slab_edge_rebate_tool); }
    if (tool == EDITOR_TOOL_SLAB_GEOMETRY) {
        slab_geometry_tool_activate(&editor->slab_geometry_tool);
    } else { slab_geometry_tool_cancel(&editor->slab_geometry_tool); }

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
    measurement_tool_init(&editor->measurement_tool);
    plan_dimension_tool_init(&editor->dimension_tool);
    plan_callout_tool_init(&editor->callout_tool);
    plan_direction_symbol_tool_init(&editor->direction_symbol_tool);
    plan_revision_cloud_tool_init(&editor->revision_cloud_tool);
    slab_tool_init(&editor->slab_tool);
    slab_polygon_feature_tool_init(&editor->slab_penetration_tool);
    slab_polygon_feature_tool_init(&editor->slab_region_tool);
    slab_edge_rebate_tool_init(&editor->slab_edge_rebate_tool);
    slab_geometry_tool_init(&editor->slab_geometry_tool);

    editor->opening_placement =
        (OpeningPlacement){0};
}

void sitehelper_editor_destroy(SiteHelperEditor *editor)
{
    if (editor == NULL) { return; }
    plan_revision_cloud_tool_destroy(&editor->revision_cloud_tool);
    slab_tool_destroy(&editor->slab_tool);
    slab_polygon_feature_tool_destroy(&editor->slab_penetration_tool);
    slab_polygon_feature_tool_destroy(&editor->slab_region_tool);
    *editor=(SiteHelperEditor){0};
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

    if (editor->active_view != EDITOR_VIEW_WALL_ELEVATION) {
        sitehelper_editor_clear_selection(editor);
        return;
    }

    WallMemberHit hit =
        wall_find_member_at_position(
            wall,
            position
        );

    editor_selection_set_wall_member(
        &editor->selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
        wall->id,
        hit.kind,
        hit.timber
    );
}

int sitehelper_editor_selection_matches_view(const SiteHelperEditor *editor)
{
    if (editor == NULL) { return 0; }
    EditorSelectionScope scope;
    switch (editor->active_view) {
        case EDITOR_VIEW_PLAN: scope = EDITOR_SELECTION_SCOPE_PLAN; break;
        case EDITOR_VIEW_WALL_ELEVATION: scope = EDITOR_SELECTION_SCOPE_WALL_ELEVATION; break;
        default: return 0;
    }
    return editor_selection_matches_scope(&editor->selection, scope);
}

void sitehelper_editor_reconcile_wall_selection(
    SiteHelperEditor *editor,
    const Wall *wall
)
{
    if (editor == NULL) { return; }
    if (!sitehelper_editor_selection_matches_view(editor)) {
        sitehelper_editor_clear_selection(editor);
        return;
    }
    if (wall == NULL) { return; }

    if (editor->selection.wall_id == wall->id &&
        editor->selection.kind == EDITOR_SELECTION_OPENING &&
        wall_find_opening_by_id_const(wall, editor->selection.opening_id) == NULL) {
        sitehelper_editor_clear_selection(editor);
        return;
    }

    const WallSelection *wall_selection =
        editor_selection_get_wall_member(
            &editor->selection,
            editor->selection.scope,
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

    if (!sitehelper_editor_selection_matches_view(editor)) {
        sitehelper_editor_clear_selection(editor);
    }

    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL) {
        editor->current_storey_id = DOMAIN_ID_INVALID;
        editor->current_room_id = DOMAIN_ID_INVALID;
        editor->current_wall_id = DOMAIN_ID_INVALID;
        sitehelper_editor_clear_selection(editor);
        sitehelper_editor_invalidate_transient_state(editor);
        return;
    }

    if (build_find_room_by_id_const(&storey->structure, editor->current_room_id) == NULL) {
        editor->current_room_id = DOMAIN_ID_INVALID;
    }
    const Wall *current_wall = build_find_wall_by_id_const(
        &storey->structure, editor->current_wall_id);

    if (current_wall == NULL) {
        editor->current_wall_id = DOMAIN_ID_INVALID;
    }

    const EditorSelection *selection = &editor->selection;

    if (selection->kind != EDITOR_SELECTION_NONE) {
        if (selection->kind == EDITOR_SELECTION_DOCUMENT) {
            DomainId document_storey = document_model_object_storey_id(
                &project->document, selection->document);
            if (document_storey != storey->id ||
                selection->scope != EDITOR_SELECTION_SCOPE_PLAN) {
                sitehelper_editor_clear_selection(editor);
            }
        } else if (selection->kind == EDITOR_SELECTION_ROOF ||
            selection->kind == EDITOR_SELECTION_ROOF_PORTION) {
            const Roof *selected_roof = roof_collection_find_by_id_const(
                &storey->roofs, selection->roof_id);
            int valid = selected_roof != NULL &&
                selection->scope == EDITOR_SELECTION_SCOPE_PLAN &&
                roof_validate(selected_roof) == ROOF_SUCCESS;
            if (valid && selection->kind == EDITOR_SELECTION_ROOF_PORTION) {
                valid = roof_find_portion_by_id_const(selected_roof,
                    selection->roof_portion_id) != NULL;
            }
            if (!valid) { sitehelper_editor_clear_selection(editor); }
        } else if (selection->kind == EDITOR_SELECTION_SLAB ||
            selection->kind == EDITOR_SELECTION_SLAB_PENETRATION ||
            selection->kind == EDITOR_SELECTION_SLAB_REGION ||
            selection->kind == EDITOR_SELECTION_SLAB_EDGE_REBATE) {
            const Slab *selected_slab = slab_collection_find_by_id_const(
                &storey->slabs, selection->slab_id);
            int valid = selected_slab != NULL &&
                selection->scope == EDITOR_SELECTION_SCOPE_PLAN &&
                slab_validate(selected_slab) == SLAB_SUCCESS;
            if (valid && selection->kind != EDITOR_SELECTION_SLAB) {
                size_t count = 0;
                if (selection->kind == EDITOR_SELECTION_SLAB_PENETRATION) {
                    count = selected_slab->definition.penetrations.count;
                } else if (selection->kind == EDITOR_SELECTION_SLAB_REGION) {
                    count = selected_slab->definition.regions.count;
                } else {
                    count = selected_slab->definition.edge_rebates.count;
                }
                valid = selection->slab_feature_index < count;
            }
            if (!valid) { sitehelper_editor_clear_selection(editor); }
        } else {
            const Wall *selected_wall = build_find_wall_by_id_const(
                &storey->structure, selection->wall_id);

            if (selected_wall == NULL) {
                sitehelper_editor_clear_selection(editor);
            }
            else {
                if (selection->kind == EDITOR_SELECTION_WALL ||
                    selection->kind == EDITOR_SELECTION_OPENING ||
                    selection->kind == EDITOR_SELECTION_WALL_MEMBER) {
                    sitehelper_editor_reconcile_wall_selection(editor, selected_wall);
                }
                else {
                    sitehelper_editor_clear_selection(editor);
                }
            }
        }
    }

    sitehelper_editor_invalidate_transient_state(editor);
}

int sitehelper_editor_select_roof_at_position(SiteHelperEditor *editor,
    const Storey *storey, PlanPoint point, double tolerance_mm)
{
    if (editor == NULL || storey == NULL || editor->active_view != EDITOR_VIEW_PLAN) {
        if (editor != NULL) { sitehelper_editor_clear_selection(editor); }
        return 0;
    }
    RoofPlanHit hit=roof_plan_hit_test_storey(storey,point,tolerance_mm);
    if (hit.kind == ROOF_PLAN_HIT_PORTION) {
        editor_selection_set_roof_portion(&editor->selection,
            EDITOR_SELECTION_SCOPE_PLAN,hit.roof_id,hit.portion_id);
        return 1;
    }
    if (hit.kind == ROOF_PLAN_HIT_ROOF) {
        editor_selection_set_roof(&editor->selection,
            EDITOR_SELECTION_SCOPE_PLAN,hit.roof_id);
        return 1;
    }
    sitehelper_editor_clear_selection(editor);
    return 1;
}

void sitehelper_editor_project_replaced(SiteHelperEditor *editor,
    const SiteHelperProject *project)
{
    if (editor == NULL) { return; }
    sitehelper_editor_clear_selection(editor);
    sitehelper_editor_invalidate_transient_state(editor);
    sitehelper_editor_reconcile(editor, project);
}

static void editor_select_slab_hit(EditorSelection *selection, SlabPlanHit hit)
{
    switch (hit.kind) {
        case SLAB_PLAN_HIT_SLAB:
            editor_selection_set_slab(selection, EDITOR_SELECTION_SCOPE_PLAN,
                hit.slab_id);
            break;
        case SLAB_PLAN_HIT_PENETRATION:
            editor_selection_set_slab_feature(selection, EDITOR_SELECTION_SCOPE_PLAN,
                hit.slab_id, EDITOR_SELECTION_SLAB_PENETRATION, hit.feature_index);
            break;
        case SLAB_PLAN_HIT_REGION:
            editor_selection_set_slab_feature(selection, EDITOR_SELECTION_SCOPE_PLAN,
                hit.slab_id, EDITOR_SELECTION_SLAB_REGION, hit.feature_index);
            break;
        case SLAB_PLAN_HIT_EDGE_REBATE:
            editor_selection_set_slab_feature(selection, EDITOR_SELECTION_SCOPE_PLAN,
                hit.slab_id, EDITOR_SELECTION_SLAB_EDGE_REBATE, hit.feature_index);
            break;
        case SLAB_PLAN_HIT_NONE:
        default:
            editor_selection_clear(selection);
            break;
    }
}

static int editor_slab_geometry_target_from_selection(const EditorSelection *selection,
    EditorSlabGeometryKind *kind, DomainId *slab_id, size_t *feature_index)
{
    if (selection == NULL || kind == NULL || slab_id == NULL || feature_index == NULL ||
        selection->scope != EDITOR_SELECTION_SCOPE_PLAN ||
        selection->slab_id == DOMAIN_ID_INVALID) { return 0; }
    switch (selection->kind) {
        case EDITOR_SELECTION_SLAB:
            *kind=EDITOR_SLAB_GEOMETRY_OUTLINE;
            *feature_index=SIZE_MAX;
            break;
        case EDITOR_SELECTION_SLAB_PENETRATION:
            *kind=EDITOR_SLAB_GEOMETRY_PENETRATION;
            *feature_index=selection->slab_feature_index;
            break;
        case EDITOR_SELECTION_SLAB_REGION:
            *kind=EDITOR_SLAB_GEOMETRY_REGION;
            *feature_index=selection->slab_feature_index;
            break;
        default:
            return 0;
    }
    *slab_id=selection->slab_id;
    return 1;
}

static const SlabOutline *editor_slab_geometry_outline(const Storey *storey,
    DomainId slab_id, EditorSlabGeometryKind kind, size_t feature_index)
{
    const Slab *slab=storey == NULL ? NULL :
        slab_collection_find_by_id_const(&storey->slabs,slab_id);
    if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS) { return NULL; }
    switch (kind) {
        case EDITOR_SLAB_GEOMETRY_OUTLINE:
            return feature_index == SIZE_MAX ? &slab->definition.outline : NULL;
        case EDITOR_SLAB_GEOMETRY_PENETRATION: {
            const SlabPenetration *feature=slab_penetration_at(slab,feature_index);
            return feature == NULL ? NULL : &feature->outline;
        }
        case EDITOR_SLAB_GEOMETRY_REGION: {
            const SlabRegion *feature=slab_region_at(slab,feature_index);
            return feature == NULL ? NULL : &feature->outline;
        }
        case EDITOR_SLAB_GEOMETRY_NONE:
        default:
            return NULL;
    }
}

static int nearest_outline_vertex(const SlabOutline *outline, PlanPoint point,
    double tolerance, size_t *vertex_index)
{
    if (outline == NULL || vertex_index == NULL || outline->vertices == NULL ||
        outline->vertex_count < 3 || !isfinite(point.x) || !isfinite(point.y) ||
        !isfinite(tolerance) || tolerance < 0.0) { return 0; }
    double best=tolerance;
    size_t found=SIZE_MAX;
    for (size_t i=0;i<outline->vertex_count;i++) {
        double dx=point.x-outline->vertices[i].x;
        double dy=point.y-outline->vertices[i].y;
        double distance=hypot(dx,dy);
        if (isfinite(distance) && distance <= best &&
            (found == SIZE_MAX || distance < best)) {
            best=distance;
            found=i;
        }
    }
    if (found == SIZE_MAX) { return 0; }
    *vertex_index=found;
    return 1;
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

/* Tool consumers use the snap resolved for this event by their entry point. */
static int editor_measurement_point(SiteHelperEditor *editor,
    Vec2 position, PlanPoint *point)
{
    if (editor->active_view != EDITOR_VIEW_PLAN ||
        !isfinite(position.x) || !isfinite(position.y)) {
        sitehelper_editor_clear_snap(editor);
        return 0;
    }
    const SnapResult *snap = sitehelper_editor_get_snap_result(editor);
    if (snap != NULL && snap->type != SNAP_NONE) { position = snap->position; }
    if (!isfinite(position.x) || !isfinite(position.y)) {
        sitehelper_editor_clear_snap(editor);
        return 0;
    }
    *point = (PlanPoint){position.x, position.y};
    return 1;
}

void sitehelper_editor_update_snap_in_project(SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 position)
{
    if (editor == NULL) { return; }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL || !isfinite(position.x) || !isfinite(position.y)) {
        sitehelper_editor_clear_snap(editor);
        return;
    }
    if (editor->active_view == EDITOR_VIEW_PLAN) {
        SnapCandidate candidates[PLAN_SNAP_CANDIDATE_CAPACITY];
        size_t count = plan_collect_snap_candidates(storey, position, &editor->snap.settings, candidates);
        sitehelper_editor_set_snap_result(editor,
            editor_snap(position, candidates, count, &editor->snap.settings));
    }
    else {
        const Wall *wall = build_find_wall_by_id_const(&storey->structure, editor->current_wall_id);
        sitehelper_editor_update_snap(editor, wall, position);
    }
}

static void editor_pointer_move_resolved(SiteHelperEditor *editor,
    const Wall *wall, const BuildSettings *settings, Vec2 view_position);

void sitehelper_editor_pointer_move_in_project(SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 view_position)
{
    if (editor == NULL) { return; }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    BuildSettings resolved = {0};
    /* Measurement needs the view context, not construction settings. */
    if (storey == NULL || (editor->active_tool != EDITOR_TOOL_MEASURE &&
        editor->active_tool != EDITOR_TOOL_SLAB &&
        editor->active_tool != EDITOR_TOOL_SLAB_PENETRATION &&
        editor->active_tool != EDITOR_TOOL_SLAB_REGION &&
        editor->active_tool != EDITOR_TOOL_SLAB_EDGE_REBATE &&
        editor->active_tool != EDITOR_TOOL_SLAB_GEOMETRY &&
        editor->active_tool != EDITOR_TOOL_NOTE &&
        editor->active_tool != EDITOR_TOOL_DIMENSION &&
        editor->active_tool != EDITOR_TOOL_SYMBOL &&
        editor->active_tool != EDITOR_TOOL_VIEW_DIRECTION &&
        editor->active_tool != EDITOR_TOOL_CALLOUT &&
        editor->active_tool != EDITOR_TOOL_REVISION_CLOUD &&
        !sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved))) {
        sitehelper_editor_invalidate_transient_state(editor);
        return;
    }
    const Wall *wall = build_find_wall_by_id_const(&storey->structure, editor->current_wall_id);
    if (editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE ||
        (editor->active_tool == EDITOR_TOOL_SLAB_GEOMETRY &&
            !editor->slab_geometry_tool.has_vertex)) {
        sitehelper_editor_clear_snap(editor);
    } else {
        sitehelper_editor_update_snap_in_project(editor, project, view_position);
    }
    if (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION ||
        editor->active_tool == EDITOR_TOOL_SLAB_REGION) {
        SlabPolygonFeatureTool *tool=editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION ?
            &editor->slab_penetration_tool : &editor->slab_region_tool;
        if (tool->vertex_count != 0 && (tool->storey_id != storey->id ||
            slab_collection_find_by_id_const(&storey->slabs,tool->slab_id) == NULL)) {
            slab_polygon_feature_tool_cancel(tool);
            tool->active=1;
            return;
        }
        PlanPoint point;
        if (editor_measurement_point(editor,view_position,&point)) {
            slab_polygon_feature_tool_update(tool,point);
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE) {
        SlabEdgeRebateTool *tool=&editor->slab_edge_rebate_tool;
        tool->has_hover=0;
        if (tool->has_start) {
            const Slab *slab=tool->storey_id == storey->id ?
                slab_collection_find_by_id_const(&storey->slabs,tool->slab_id) : NULL;
            if (slab == NULL) {
                slab_edge_rebate_tool_cancel(tool);
                tool->active=1;
                return;
            }
            SlabPlanEdgeHit hit=slab_plan_project_outer_edge(slab,tool->edge_index,
                (PlanPoint){view_position.x,view_position.y},
                editor->snap.settings.object_snap_tolerance);
            if (hit.has_edge) {
                tool->hover_u_mm=hit.u_mm;
                tool->hover_point=hit.projected_point;
                tool->has_hover=1;
            }
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_SLAB_GEOMETRY) {
        SlabGeometryTool *tool=&editor->slab_geometry_tool;
        if (!tool->has_vertex) { return; }
        const SlabOutline *outline=tool->storey_id == storey->id ?
            editor_slab_geometry_outline(storey,tool->slab_id,tool->kind,
                tool->feature_index) : NULL;
        if (outline == NULL || tool->vertex_index >= outline->vertex_count ||
            outline->vertices[tool->vertex_index].x != tool->original_position.x ||
            outline->vertices[tool->vertex_index].y != tool->original_position.y) {
            slab_geometry_tool_cancel(tool);
            tool->active=1;
            sitehelper_editor_clear_snap(editor);
            return;
        }
        PlanPoint point;
        PlanPosition position;
        if (editor_measurement_point(editor,view_position,&point) &&
            plan_position_from_point(point,&position)) {
            /* Preview the same authoritative integer-mm candidate that a
             * second click would place, rather than a sub-millimetre cursor. */
            slab_geometry_tool_update(tool,(PlanPoint){position.x,position.y});
        } else {
            tool->has_preview=0;
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_DIMENSION) {
        PlanDimensionTool *tool=&editor->dimension_tool;
        if (tool->stage == PLAN_DIMENSION_TOOL_PICK_SECOND) {
            PlanPoint point;
            if (editor_measurement_point(editor,view_position,&point)) {
                (void)plan_dimension_tool_update_pointer(tool,point);
            }
        } else if (tool->stage == PLAN_DIMENSION_TOOL_PLACE_OFFSET) {
            sitehelper_editor_clear_snap(editor);
            (void)plan_dimension_tool_update_pointer(tool,
                (PlanPoint){view_position.x,view_position.y});
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_CALLOUT) {
        PlanCalloutTool *tool=&editor->callout_tool;
        if (tool->stage == PLAN_CALLOUT_TOOL_PICK_LABEL) {
            PlanPoint point;
            if (editor_measurement_point(editor,view_position,&point)) {
                (void)plan_callout_tool_update_pointer(tool,point);
            }
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_VIEW_DIRECTION) {
        PlanDirectionSymbolTool *tool=&editor->direction_symbol_tool;
        if (tool->stage == PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION) {
            PlanPoint point;
            if (editor_measurement_point(editor,view_position,&point)) {
                (void)plan_direction_symbol_tool_update_pointer(tool,point);
            }
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_REVISION_CLOUD) {
        PlanPoint point;
        if (editor_measurement_point(editor,view_position,&point)) {
            plan_revision_cloud_tool_update(&editor->revision_cloud_tool,point);
        } else {
            editor->revision_cloud_tool.has_preview=0;
        }
        return;
    }
    if (editor->active_tool == EDITOR_TOOL_SYMBOL) {
        return; /* Snap state itself is the one-click point-marker preview. */
    }
    editor_pointer_move_resolved(editor, wall, &resolved, view_position);
}

void sitehelper_editor_pointer_move(SiteHelperEditor *editor,
    const Wall *wall, const BuildSettings *settings, Vec2 view_position)
{
    if (editor == NULL) { return; }
    sitehelper_editor_update_snap(editor, wall, view_position);
    editor_pointer_move_resolved(editor, wall, settings, view_position);
}

static void editor_pointer_move_resolved(
    SiteHelperEditor *editor,
    const Wall *wall,
    const BuildSettings *settings,
    Vec2 view_position
)
{
    if (editor == NULL) {
        return;
    }

    if (editor->active_tool == EDITOR_TOOL_MEASURE) {
        PlanPoint point;
        if (editor_measurement_point(editor, view_position, &point)) {
            (void)measurement_tool_update(&editor->measurement_tool, point);
        }
        return;
    }

    if (editor->active_tool == EDITOR_TOOL_WALL) {
        const SnapResult *snap_result = editor_snap_state_get_result(
            &editor->snap
        );

        wall_tool_update(&editor->wall_tool,
            snap_result != NULL && snap_result->type != SNAP_NONE
                ? snap_result->position : view_position);

        wall_tool_update_direction(&editor->wall_tool, view_position);
        editor->opening_placement = (OpeningPlacement){0};
        return;
    }

    if (editor->active_tool == EDITOR_TOOL_SLAB) {
        PlanPoint point;
        if (editor_measurement_point(editor,view_position,&point)) {
            slab_tool_update(&editor->slab_tool,point);
        }
        return;
    }

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

    if (editor->active_tool == EDITOR_TOOL_SLAB) {
        editor->slab_tool.has_preview=0;
        sitehelper_editor_clear_snap(editor);
    } else if (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION ||
        editor->active_tool == EDITOR_TOOL_SLAB_REGION) {
        SlabPolygonFeatureTool *tool=editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION ?
            &editor->slab_penetration_tool : &editor->slab_region_tool;
        tool->has_preview=0;
        sitehelper_editor_clear_snap(editor);
    } else if (editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE) {
        editor->slab_edge_rebate_tool.has_hover=0;
        sitehelper_editor_clear_snap(editor);
    } else if (editor->active_tool == EDITOR_TOOL_SLAB_GEOMETRY) {
        editor->slab_geometry_tool.has_preview=0;
        sitehelper_editor_clear_snap(editor);
    } else if (editor->active_tool == EDITOR_TOOL_DIMENSION) {
        editor->dimension_tool.has_pointer=0;
        sitehelper_editor_clear_snap(editor);
    } else if (editor->active_tool == EDITOR_TOOL_VIEW_DIRECTION) {
        editor->direction_symbol_tool.has_pointer=0;
        sitehelper_editor_clear_snap(editor);
    } else {
        sitehelper_editor_invalidate_transient_state(editor);
    }
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

static int editor_wall_action(const SiteHelperEditor *editor,
    WallPlanSegment segment, EditorAction *action)
{
    WallCommand command;
    if (!wall_command_create(editor->current_storey_id, segment, &command) ||
        !sitehelper_command_from_wall(&command, &action->command)) { return 0; }
    action->kind = EDITOR_ACTION_COMMAND;
    return 1;
}

static int editor_polygon_feature_action(const SiteHelperEditor *editor,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN) {
        return 0;
    }
    *action=(EditorAction){0};
    if (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION) {
        const SlabPolygonFeatureTool *tool=&editor->slab_penetration_tool;
        AddSlabPenetrationCommand add={0};
        if (tool->vertex_count < 3 || !add_slab_penetration_command_create(
                tool->slab_id,tool->vertices,tool->vertex_count,&add)) { return 0; }
        int ok=sitehelper_command_from_add_slab_penetration(&add,&action->command);
        add_slab_penetration_command_destroy(&add);
        if (!ok) { return 0; }
    } else if (editor->active_tool == EDITOR_TOOL_SLAB_REGION) {
        const SlabPolygonFeatureTool *tool=&editor->slab_region_tool;
        AddSlabRegionCommand add={0};
        if (tool->vertex_count < 3 || !add_slab_region_command_create(tool->slab_id,
                tool->vertices,tool->vertex_count,tool->top_level_offset_mm,
                tool->thickness_mm,&add)) { return 0; }
        int ok=sitehelper_command_from_add_slab_region(&add,&action->command);
        add_slab_region_command_destroy(&add);
        if (!ok) { return 0; }
    } else { return 0; }
    action->kind=EDITOR_ACTION_COMMAND;
    return 1;
}

static int editor_polygon_feature_click(SiteHelperEditor *editor,
    const Storey *storey, Vec2 view_position, EditorAction *action)
{
    PlanPoint point;
    PlanPosition vertex;
    if (!editor_measurement_point(editor,view_position,&point) ||
        !plan_position_from_point(point,&vertex)) { return 0; }
    SlabPolygonFeatureTool *tool=editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION ?
        &editor->slab_penetration_tool : &editor->slab_region_tool;
    if (tool->vertex_count != 0 && (tool->storey_id != storey->id ||
        slab_collection_find_by_id_const(&storey->slabs,tool->slab_id) == NULL)) {
        slab_polygon_feature_tool_cancel(tool);
        tool->active=1;
        return 1;
    }
    if (tool->vertex_count == 0) {
        SlabPlanHit hit=slab_plan_hit_test_storey(storey,
            (PlanPoint){view_position.x,view_position.y},
            editor->snap.settings.object_snap_tolerance);
        if (hit.kind == SLAB_PLAN_HIT_NONE) { return 1; }
        const Slab *slab=slab_collection_find_by_id_const(&storey->slabs,hit.slab_id);
        if (slab == NULL) { return 1; }
        return slab_polygon_feature_tool_begin(tool,storey->id,slab->id,vertex,
            slab->definition.top_level_offset_mm,slab->definition.thickness_mm);
    }
    double dx=point.x-tool->vertices[0].x,dy=point.y-tool->vertices[0].y;
    if (hypot(dx,dy) <= editor->snap.settings.object_snap_tolerance) {
        if (tool->vertex_count < 3) { return 1; }
        return editor_polygon_feature_action(editor,action);
    }
    return slab_polygon_feature_tool_append(tool,vertex);
}

static int editor_rebate_click(SiteHelperEditor *editor, const Storey *storey,
    Vec2 view_position, EditorAction *action)
{
    SlabEdgeRebateTool *tool=&editor->slab_edge_rebate_tool;
    PlanPoint point={view_position.x,view_position.y};
    if (!tool->has_start) {
        SlabPlanEdgeHit hit=slab_plan_find_outer_edge(storey,point,
            editor->snap.settings.object_snap_tolerance);
        if (!hit.has_edge) { return 1; }
        tool->storey_id=storey->id;
        tool->slab_id=hit.slab_id;
        tool->edge_index=hit.edge_index;
        tool->start_u_mm=hit.u_mm;
        tool->start_point=hit.projected_point;
        tool->hover_u_mm=hit.u_mm;
        tool->hover_point=hit.projected_point;
        tool->has_start=tool->has_hover=1;
        return 1;
    }
    const Slab *slab=tool->storey_id == storey->id ?
        slab_collection_find_by_id_const(&storey->slabs,tool->slab_id) : NULL;
    if (slab == NULL) {
        slab_edge_rebate_tool_cancel(tool);
        tool->active=1;
        return 1;
    }
    SlabPlanEdgeHit hit=slab_plan_project_outer_edge(slab,tool->edge_index,point,
        editor->snap.settings.object_snap_tolerance);
    if (!hit.has_edge || hit.u_mm == tool->start_u_mm) { return 1; }
    tool->hover_u_mm=hit.u_mm;
    tool->hover_point=hit.projected_point;
    tool->has_hover=1;
    int start=tool->start_u_mm < hit.u_mm ? tool->start_u_mm : hit.u_mm;
    int end=tool->start_u_mm < hit.u_mm ? hit.u_mm : tool->start_u_mm;
    AddSlabEdgeRebateCommand add;
    if (!add_slab_edge_rebate_command_create(tool->slab_id,tool->edge_index,
            start,end,tool->width_mm,tool->depth_mm,&add) ||
        !sitehelper_command_from_add_slab_edge_rebate(&add,&action->command)) {
        return 0;
    }
    action->kind=EDITOR_ACTION_COMMAND;
    return 1;
}

static MoveSlabVertexTarget editor_move_target(EditorSlabGeometryKind kind)
{
    switch (kind) {
        case EDITOR_SLAB_GEOMETRY_OUTLINE: return MOVE_SLAB_VERTEX_OUTLINE;
        case EDITOR_SLAB_GEOMETRY_PENETRATION: return MOVE_SLAB_VERTEX_PENETRATION;
        case EDITOR_SLAB_GEOMETRY_REGION: return MOVE_SLAB_VERTEX_REGION;
        case EDITOR_SLAB_GEOMETRY_NONE:
        default: return MOVE_SLAB_VERTEX_TARGET_COUNT;
    }
}

static int editor_slab_geometry_click(SiteHelperEditor *editor,
    const Storey *storey, Vec2 view_position, EditorAction *action)
{
    SlabGeometryTool *tool=&editor->slab_geometry_tool;
    if (!tool->has_vertex) {
        EditorSlabGeometryKind kind;
        DomainId slab_id;
        size_t feature_index,vertex_index;
        if (!editor_slab_geometry_target_from_selection(&editor->selection,&kind,
                &slab_id,&feature_index)) { return 1; }
        const SlabOutline *outline=editor_slab_geometry_outline(storey,slab_id,kind,
            feature_index);
        if (outline == NULL || !nearest_outline_vertex(outline,
                (PlanPoint){view_position.x,view_position.y},
                editor->snap.settings.object_snap_tolerance,&vertex_index)) { return 1; }
        return slab_geometry_tool_begin(tool,storey->id,slab_id,kind,feature_index,
            vertex_index,outline->vertices[vertex_index]);
    }

    const SlabOutline *outline=tool->storey_id == storey->id ?
        editor_slab_geometry_outline(storey,tool->slab_id,tool->kind,
            tool->feature_index) : NULL;
    if (outline == NULL || tool->vertex_index >= outline->vertex_count ||
        outline->vertices[tool->vertex_index].x != tool->original_position.x ||
        outline->vertices[tool->vertex_index].y != tool->original_position.y) {
        slab_geometry_tool_cancel(tool);
        tool->active=1;
        sitehelper_editor_clear_snap(editor);
        return 1;
    }
    PlanPoint point;
    PlanPosition position;
    if (!editor_measurement_point(editor,view_position,&point) ||
        !plan_position_from_point(point,&position)) { return 0; }
    if (position.x == tool->original_position.x && position.y == tool->original_position.y) {
        slab_geometry_tool_cancel(tool);
        tool->active=1;
        sitehelper_editor_clear_snap(editor);
        return 1;
    }
    MoveSlabVertexTarget target=editor_move_target(tool->kind);
    MoveSlabVertexCommand move;
    if (target == MOVE_SLAB_VERTEX_TARGET_COUNT ||
        !move_slab_vertex_command_create(tool->slab_id,target,tool->feature_index,
            tool->vertex_index,position,&move) ||
        !sitehelper_command_from_move_slab_vertex(&move,&action->command)) { return 0; }
    action->kind=EDITOR_ACTION_COMMAND;
    return 1;
}

static int editor_primary_action_resolved(
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
        case EDITOR_TOOL_MEASURE:
        {
            PlanPoint point;
            return editor_measurement_point(editor, view_position, &point) &&
                measurement_tool_click(&editor->measurement_tool, point);
        }
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
                editor_selection_set_wall(&editor->selection, EDITOR_SELECTION_SCOPE_PLAN, editor->current_wall_id);
                return 1;
            }

            if (!isfinite(view_position.x) || !isfinite(view_position.y) ||
                view_position.x < INT_MIN || view_position.x > INT_MAX ||
                view_position.y < INT_MIN || view_position.y > INT_MAX) {
                sitehelper_editor_clear_selection(editor);
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
            return wall_tool_command_data(&editor->wall_tool, &segment) &&
                editor_wall_action(editor, segment, action);
        }

        case EDITOR_TOOL_SLAB: {
            PlanPoint point;
            PlanPosition vertex;
            if (!editor_measurement_point(editor,view_position,&point) ||
                !plan_position_from_point(point,&vertex)) { return 0; }
            SlabTool *tool=&editor->slab_tool;
            if (tool->vertex_count != 0) {
                double dx=point.x-tool->vertices[0].x, dy=point.y-tool->vertices[0].y;
                if (hypot(dx,dy) <= editor->snap.settings.object_snap_tolerance) {
                    if (tool->vertex_count < 3) { return 1; }
                    return sitehelper_editor_create_slab_action(editor,action);
                }
            }
            return slab_tool_append(tool,editor->current_storey_id,vertex);
        }

        default:
            return 1;
    }
}

int sitehelper_editor_primary_action(SiteHelperEditor *editor,
    const Wall *wall, Vec2 view_position, EditorAction *action)
{
    if (editor == NULL || action == NULL) { return 0; }
    if (editor->active_view == EDITOR_VIEW_PLAN) {
        sitehelper_editor_update_snap(editor, wall, view_position);
    }
    return editor_primary_action_resolved(editor, wall, view_position, action);
}


static DomainId editor_find_plan_dimension_at_position(const SiteHelperProject *project,
    DomainId storey_id, PlanPoint point, double tolerance_mm)
{
    if (project == NULL || storey_id == DOMAIN_ID_INVALID || !(tolerance_mm >= 0.0)) {
        return DOMAIN_ID_INVALID;
    }
    DomainId best=DOMAIN_ID_INVALID;
    double best_distance=tolerance_mm;
    for (size_t i=project->document.dimension_count;i>0;i--) {
        const DocumentPlanDimension *dimension=&project->document.dimensions[i-1];
        if (dimension->storey_id != storey_id) continue;
        PlanPosition a,b; int distance_mm;
        if (!sitehelper_project_resolve_plan_dimension(project,dimension->id,&a,&b,&distance_mm)) continue;
        (void)distance_mm;
        DocumentPlanDimensionGeometry geometry;
        if (!document_plan_dimension_geometry(a,b,dimension->offset_mm,&geometry)) continue;
        double distance=document_plan_dimension_hit_distance(&geometry,point);
        if (distance >= 0.0 && distance <= best_distance &&
            (best == DOMAIN_ID_INVALID || distance < best_distance)) {
            best=dimension->id;
            best_distance=distance;
        }
    }
    return best;
}


static int editor_dimension_reference_at(const SiteHelperEditor *editor,
    const Storey *storey, Vec2 view_position,
    DocumentDimensionReference *reference, PlanPosition *position)
{
    if (editor == NULL || storey == NULL || reference == NULL || position == NULL) {
        return 0;
    }
    Vec2 resolved=view_position;
    const SnapResult *snap=sitehelper_editor_get_snap_result(editor);
    if (snap != NULL && snap->type != SNAP_NONE) { resolved=snap->position; }
    PlanPosition fixed_position;
    if (!plan_position_from_point((PlanPoint){resolved.x,resolved.y},&fixed_position)) {
        return 0;
    }

    /* Endpoint/intersection snaps carry no endpoint source identity. Associate
     * only when the snapped coordinate names exactly one wall endpoint. This
     * preserves a semantic endpoint at a T-junction even though intersection
     * snapping wins the cross-type tie; shared corners and true crossings still
     * remain fixed points rather than choosing an arbitrary wall. */
    if (snap != NULL && (snap->type == SNAP_ENDPOINT || snap->type == SNAP_INTERSECTION)) {
        size_t match_count=0;
        DomainId wall_id=DOMAIN_ID_INVALID;
        DocumentDimensionReferenceKind kind=DOCUMENT_DIMENSION_FIXED_POINT;
        for (size_t i=0;i<storey->structure.wall_count;i++) {
            const Wall *wall=&storey->structure.walls[i];
            WallPlanSegment segment=wall->definition.segment;
            if ((double)segment.start.x == snap->position.x &&
                (double)segment.start.y == snap->position.y) {
                match_count++; wall_id=wall->id; kind=DOCUMENT_DIMENSION_WALL_START;
            }
            if ((double)segment.end.x == snap->position.x &&
                (double)segment.end.y == snap->position.y) {
                match_count++; wall_id=wall->id; kind=DOCUMENT_DIMENSION_WALL_END;
            }
        }
        if (match_count == 1) {
            *reference=(DocumentDimensionReference){.kind=kind,.target_id=wall_id};
            *position=fixed_position;
            return 1;
        }
    }
    *reference=(DocumentDimensionReference){
        .kind=DOCUMENT_DIMENSION_FIXED_POINT,
        .position=fixed_position
    };
    *position=fixed_position;
    return 1;
}

static int editor_dimension_click(SiteHelperEditor *editor,
    const Storey *storey, Vec2 view_position, EditorAction *action)
{
    PlanDimensionTool *tool=&editor->dimension_tool;
    if (tool->stage == PLAN_DIMENSION_TOOL_PLACE_OFFSET) {
        if (!plan_dimension_tool_update_pointer(tool,
                (PlanPoint){view_position.x,view_position.y})) { return 0; }
        DocumentDimensionReference first,second;
        int offset_mm;
        if (!plan_dimension_tool_command_data(tool,&first,&second,&offset_mm)) { return 0; }
        return sitehelper_editor_create_plan_dimension_action(editor,first,second,offset_mm,action);
    }

    DocumentDimensionReference reference;
    PlanPosition position;
    if (!editor_dimension_reference_at(editor,storey,view_position,&reference,&position)) {
        return 0;
    }
    if (tool->stage == PLAN_DIMENSION_TOOL_PICK_FIRST) {
        return plan_dimension_tool_set_first(tool,reference,position);
    }
    if (tool->stage == PLAN_DIMENSION_TOOL_PICK_SECOND) {
        /* A zero-length second pick is handled but leaves the authoring state
         * waiting for a distinct second reference. */
        if (position.x == tool->first_position.x && position.y == tool->first_position.y) {
            return 1;
        }
        return plan_dimension_tool_set_second(tool,reference,position);
    }
    return 0;
}

static int editor_callout_click(SiteHelperEditor *editor, Vec2 view_position)
{
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_CALLOUT) { return 0; }
    PlanPoint point;
    PlanPosition position;
    if (!editor_measurement_point(editor,view_position,&point) ||
        !plan_position_from_point(point,&position)) { return 0; }
    PlanCalloutTool *tool=&editor->callout_tool;
    if (tool->stage == PLAN_CALLOUT_TOOL_PICK_TARGET) {
        return plan_callout_tool_set_target(tool,position);
    }
    if (tool->stage == PLAN_CALLOUT_TOOL_PICK_LABEL) {
        if (position.x == tool->target.x && position.y == tool->target.y) { return 1; }
        return plan_callout_tool_set_label(tool,position);
    }
    return tool->stage == PLAN_CALLOUT_TOOL_READY_TEXT;
}

int sitehelper_editor_primary_action_in_project(
    SiteHelperEditor *editor,
    const SiteHelperProject *project,
    Vec2 view_position,
    EditorAction *action
)
{
    if (editor == NULL || project == NULL || action == NULL) {
        return 0;
    }

    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL) { return 0; }
    const BuildStructure *structure = &storey->structure;

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_CALLOUT) {
        *action=(EditorAction){0};
        if (editor->callout_tool.stage == PLAN_CALLOUT_TOOL_PICK_TARGET) {
            DomainId hit=document_plan_find_callout_at_position(&project->document,storey->id,
                (PlanPoint){view_position.x,view_position.y},
                editor->snap.settings.object_snap_tolerance);
            if (hit != DOMAIN_ID_INVALID) {
                editor_selection_set_callout(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,hit);
                sitehelper_editor_update_snap_in_project(editor,project,view_position);
                return 1;
            }
            if (editor_selection_is_document_kind(&editor->selection,
                    DOCUMENT_OBJECT_CALLOUT)) {
                sitehelper_editor_clear_selection(editor);
            }
        }
        sitehelper_editor_update_snap_in_project(editor,project,view_position);
        return editor_callout_click(editor,view_position);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_SYMBOL) {
        *action=(EditorAction){0};
        sitehelper_editor_update_snap_in_project(editor,project,view_position);
        Vec2 point=editor->snap.result.type == SNAP_NONE ? view_position : editor->snap.result.position;
        PlanPosition anchor;
        if (!plan_position_from_point((PlanPoint){point.x,point.y},&anchor)) { return 0; }
        return sitehelper_editor_create_plan_symbol_action(editor,
            DOCUMENT_PLAN_SYMBOL_POINT_MARKER,anchor,(DocumentPlanDirection){0,0},action);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_VIEW_DIRECTION) {
        *action=(EditorAction){0};
        sitehelper_editor_update_snap_in_project(editor,project,view_position);
        Vec2 point=editor->snap.result.type == SNAP_NONE ? view_position : editor->snap.result.position;
        PlanPosition position;
        if (!plan_position_from_point((PlanPoint){point.x,point.y},&position)) { return 0; }
        PlanDirectionSymbolTool *tool=&editor->direction_symbol_tool;
        if (tool->stage == PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR) {
            return plan_direction_symbol_tool_set_anchor(tool,position);
        }
        if (!plan_direction_symbol_tool_set_direction_point(tool,position)) { return 1; }
        PlanPosition anchor; DocumentPlanDirection direction;
        if (!plan_direction_symbol_tool_ready(tool,&anchor,&direction)) { return 1; }
        return sitehelper_editor_create_plan_symbol_action(editor,
            DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,anchor,direction,action);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_DIMENSION) {
        *action=(EditorAction){0};
        if (editor->dimension_tool.stage == PLAN_DIMENSION_TOOL_PLACE_OFFSET) {
            sitehelper_editor_clear_snap(editor);
        } else {
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
        }
        return editor_dimension_click(editor,storey,view_position,action);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_SLAB_GEOMETRY) {
        *action=(EditorAction){0};
        if (editor->slab_geometry_tool.has_vertex) {
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
        } else {
            sitehelper_editor_clear_snap(editor);
        }
        return editor_slab_geometry_click(editor,storey,view_position,action);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION ||
         editor->active_tool == EDITOR_TOOL_SLAB_REGION ||
         editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE)) {
        *action=(EditorAction){0};
        if (editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE) {
            sitehelper_editor_clear_snap(editor);
        } else {
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
        }
        return editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE ?
            editor_rebate_click(editor,storey,view_position,action) :
            editor_polygon_feature_click(editor,storey,view_position,action);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_REVISION_CLOUD) {
        *action=(EditorAction){0};
        sitehelper_editor_update_snap_in_project(editor,project,view_position);
        Vec2 point=editor->snap.result.type == SNAP_NONE ? view_position : editor->snap.result.position;
        PlanPosition position;
        if (!plan_position_from_point((PlanPoint){point.x,point.y},&position)) { return 0; }
        return plan_revision_cloud_tool_append(&editor->revision_cloud_tool,storey->id,position);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_SELECT) {
        *action = (EditorAction){ .kind = EDITOR_ACTION_NONE };
        sitehelper_editor_clear_selection(editor);
        sitehelper_editor_invalidate_transient_state(editor);
        editor->current_wall_id = DOMAIN_ID_INVALID;
        double nearest = editor->snap.settings.object_snap_tolerance;
        DomainId revision_cloud_id=document_plan_find_revision_cloud_at_position(
            &project->document,storey->id,(PlanPoint){view_position.x,view_position.y},nearest);
        if (revision_cloud_id != DOMAIN_ID_INVALID) {
            editor_selection_set_revision_cloud(&editor->selection,
                EDITOR_SELECTION_SCOPE_PLAN,revision_cloud_id);
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
            return 1;
        }
        DomainId annotation_id = document_plan_find_note_at_position(&project->document,
            storey->id, (PlanPoint){view_position.x, view_position.y}, nearest);
        if (annotation_id != DOMAIN_ID_INVALID) {
            editor_selection_set_annotation(&editor->selection,
                EDITOR_SELECTION_SCOPE_PLAN, annotation_id);
            sitehelper_editor_update_snap_in_project(editor, project, view_position);
            return 1;
        }
        DomainId callout_id=document_plan_find_callout_at_position(&project->document,
            storey->id,(PlanPoint){view_position.x,view_position.y},nearest);
        if (callout_id != DOMAIN_ID_INVALID) {
            editor_selection_set_callout(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,callout_id);
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
            return 1;
        }
        DomainId symbol_id=document_plan_find_symbol_at_position(&project->document,
            storey->id,(PlanPoint){view_position.x,view_position.y},nearest);
        if (symbol_id != DOMAIN_ID_INVALID) {
            editor_selection_set_symbol(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,symbol_id);
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
            return 1;
        }
        DomainId dimension_id=editor_find_plan_dimension_at_position(project,storey->id,
            (PlanPoint){view_position.x,view_position.y},nearest);
        if (dimension_id != DOMAIN_ID_INVALID) {
            editor_selection_set_dimension(&editor->selection,
                EDITOR_SELECTION_SCOPE_PLAN,dimension_id);
            sitehelper_editor_update_snap_in_project(editor,project,view_position);
            return 1;
        }

        /* Document overlays have precedence over physical geometry. Nearest wall
         * segment wins next; later appended walls win exact ties. */
        for (size_t index = structure->wall_count; index > 0; index--) {
            const Wall *wall = &structure->walls[index - 1];
            double distance = plan_segment_distance(wall->definition.segment, view_position);
            if (distance <= nearest &&
                (editor->current_wall_id == DOMAIN_ID_INVALID || distance < nearest)) {
                nearest = distance;
                editor->current_wall_id = wall->id;
            }
        }
        if (editor->current_wall_id != DOMAIN_ID_INVALID) {
            editor_selection_set_wall(&editor->selection,
                EDITOR_SELECTION_SCOPE_PLAN, editor->current_wall_id);
        } else {
            SlabPlanHit hit = slab_plan_hit_test_storey(storey,
                (PlanPoint){view_position.x, view_position.y},
                editor->snap.settings.object_snap_tolerance);
            editor_select_slab_hit(&editor->selection, hit);
        }
        sitehelper_editor_update_snap_in_project(editor, project, view_position);
        return 1;
    }

    const Wall *wall = build_find_wall_by_id_const(
        structure, editor->current_wall_id);

    /* Settings may have changed since the last pointer event. Revalidate the
     * transient Opening candidate with exactly the configuration commands use. */
    if (editor->active_tool == EDITOR_TOOL_OPENING) {
        sitehelper_editor_pointer_move_in_project(editor, project, view_position);
    }

    if (editor->active_view == EDITOR_VIEW_PLAN) {
        sitehelper_editor_update_snap_in_project(editor, project, view_position);
    }

    int success = editor_primary_action_resolved(
        editor,
        wall,
        view_position,
        action
    );
    /* Keep generated-member hit precedence. Empty framed space selects the
     * authoritative Opening independently of generated member allocations. */
    if (success && wall != NULL && editor->active_view == EDITOR_VIEW_WALL_ELEVATION &&
        editor->active_tool == EDITOR_TOOL_SELECT &&
        editor_selection_is_empty(&editor->selection) &&
        isfinite(view_position.x) && isfinite(view_position.y) &&
        view_position.x >= INT_MIN && view_position.x <= INT_MAX &&
        view_position.y >= INT_MIN && view_position.y <= INT_MAX) {
        BuildSettings resolved;
        if (sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved)) {
            DomainId opening_id = wall_find_opening_at_position(wall, &resolved,
                (WallLocalPosition){(int)view_position.x, (int)view_position.y});
            if (opening_id != DOMAIN_ID_INVALID) {
                editor_selection_set_opening(&editor->selection, EDITOR_SELECTION_SCOPE_WALL_ELEVATION, wall->id, opening_id);
            }
        }
    }
    return success;
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
            if (action->command.data.wall.storey_id == editor->current_storey_id) {
                editor->current_wall_id = result->data.add_wall.wall_id;
            }
            wall_tool_cancel(&editor->wall_tool);
            break;

        case SITEHELPER_COMMAND_CREATE_PLAN_NOTE:
            if (action->command.data.create_plan_note.storey_id == editor->current_storey_id) {
                editor_selection_set_annotation(&editor->selection,
                    EDITOR_SELECTION_SCOPE_PLAN, result->data.annotation.annotation_id);
                editor->current_wall_id = DOMAIN_ID_INVALID;
            }
            break;
        case SITEHELPER_COMMAND_DELETE_PLAN_NOTE:
            if (editor_selection_matches_document(&editor->selection, DOCUMENT_OBJECT_NOTE,
                result->data.annotation.annotation_id)) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_EDIT_PLAN_NOTE:
            break;
        case SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION:
            if (action->command.data.create_plan_dimension.storey_id == editor->current_storey_id) {
                editor_selection_set_dimension(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,
                    result->data.dimension.dimension_id);
                editor->current_wall_id=DOMAIN_ID_INVALID;
            }
            plan_dimension_tool_cancel(&editor->dimension_tool);
            if (editor->active_tool == EDITOR_TOOL_DIMENSION) { editor->dimension_tool.active=1; }
            sitehelper_editor_clear_snap(editor);
            break;
        case SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION:
            if (editor_selection_matches_document(&editor->selection, DOCUMENT_OBJECT_DIMENSION,
                result->data.dimension.dimension_id)) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION:
            break;

        case SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL:
            if (action->command.data.create_plan_symbol.storey_id == editor->current_storey_id) {
                editor_selection_set_symbol(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,
                    result->data.symbol.symbol_id);
                editor->current_wall_id=DOMAIN_ID_INVALID;
            }
            if (editor->active_tool == EDITOR_TOOL_VIEW_DIRECTION) {
                plan_direction_symbol_tool_cancel(&editor->direction_symbol_tool);
                editor->direction_symbol_tool.active=1;
                sitehelper_editor_clear_snap(editor);
            }
            break;
        case SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL:
            if (editor_selection_matches_document(&editor->selection, DOCUMENT_OBJECT_SYMBOL,
                result->data.symbol.symbol_id)) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL:
            break;

        case SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT:
            if (action->command.data.create_plan_callout.storey_id == editor->current_storey_id) {
                editor_selection_set_callout(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,
                    result->data.callout.callout_id);
                editor->current_wall_id=DOMAIN_ID_INVALID;
            }
            plan_callout_tool_cancel(&editor->callout_tool);
            if (editor->active_tool == EDITOR_TOOL_CALLOUT) { editor->callout_tool.active=1; }
            sitehelper_editor_clear_snap(editor);
            break;
        case SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT:
            if (editor_selection_matches_document(&editor->selection, DOCUMENT_OBJECT_CALLOUT,
                result->data.callout.callout_id)) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT:
            break;

        case SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD:
            if (action->command.data.create_plan_revision_cloud.storey_id ==
                editor->current_storey_id) {
                editor_selection_set_revision_cloud(&editor->selection,
                    EDITOR_SELECTION_SCOPE_PLAN,result->data.revision_cloud.revision_cloud_id);
                editor->current_wall_id=DOMAIN_ID_INVALID;
            }
            plan_revision_cloud_tool_cancel(&editor->revision_cloud_tool);
            if (editor->active_tool == EDITOR_TOOL_REVISION_CLOUD) {
                editor->revision_cloud_tool.active=1;
            }
            sitehelper_editor_clear_snap(editor);
            break;
        case SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD:
            if (editor_selection_matches_document(&editor->selection,
                    DOCUMENT_OBJECT_REVISION_CLOUD,
                    result->data.revision_cloud.revision_cloud_id)) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD:
            break;

        case SITEHELPER_COMMAND_CREATE_SLAB:
            if (action->command.data.create_slab.storey_id == editor->current_storey_id) {
                editor_selection_set_slab(&editor->selection,
                    EDITOR_SELECTION_SCOPE_PLAN,result->data.slab.slab_id);
            }
            slab_tool_cancel(&editor->slab_tool);
            if (editor->active_tool == EDITOR_TOOL_SLAB) { editor->slab_tool.active=1; }
            break;

        case SITEHELPER_COMMAND_ADD_SLAB_PENETRATION:
            if (result->data.slab_feature.slab_id ==
                editor->slab_penetration_tool.slab_id) {
                editor_selection_set_slab_feature(&editor->selection,
                    EDITOR_SELECTION_SCOPE_PLAN,result->data.slab_feature.slab_id,
                    EDITOR_SELECTION_SLAB_PENETRATION,
                    result->data.slab_feature.feature_index);
            }
            slab_polygon_feature_tool_cancel(&editor->slab_penetration_tool);
            if (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION) {
                editor->slab_penetration_tool.active=1;
            }
            break;

        case SITEHELPER_COMMAND_ADD_SLAB_REGION:
            if (result->data.slab_feature.slab_id == editor->slab_region_tool.slab_id) {
                editor_selection_set_slab_feature(&editor->selection,
                    EDITOR_SELECTION_SCOPE_PLAN,result->data.slab_feature.slab_id,
                    EDITOR_SELECTION_SLAB_REGION,result->data.slab_feature.feature_index);
            }
            slab_polygon_feature_tool_cancel(&editor->slab_region_tool);
            if (editor->active_tool == EDITOR_TOOL_SLAB_REGION) {
                editor->slab_region_tool.active=1;
            }
            break;

        case SITEHELPER_COMMAND_ADD_SLAB_EDGE_REBATE:
            if (result->data.slab_feature.slab_id == editor->slab_edge_rebate_tool.slab_id) {
                editor_selection_set_slab_feature(&editor->selection,
                    EDITOR_SELECTION_SCOPE_PLAN,result->data.slab_feature.slab_id,
                    EDITOR_SELECTION_SLAB_EDGE_REBATE,
                    result->data.slab_feature.feature_index);
            }
            slab_edge_rebate_tool_cancel(&editor->slab_edge_rebate_tool);
            if (editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE) {
                editor->slab_edge_rebate_tool.active=1;
            }
            break;

        case SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION:
            if (editor->selection.kind == EDITOR_SELECTION_SLAB_PENETRATION &&
                editor->selection.slab_id == result->data.slab_feature.slab_id &&
                editor->selection.slab_feature_index == result->data.slab_feature.feature_index) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_DELETE_SLAB_REGION:
            if (editor->selection.kind == EDITOR_SELECTION_SLAB_REGION &&
                editor->selection.slab_id == result->data.slab_feature.slab_id &&
                editor->selection.slab_feature_index == result->data.slab_feature.feature_index) {
                sitehelper_editor_clear_selection(editor);
            }
            break;
        case SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE:
            if (editor->selection.kind == EDITOR_SELECTION_SLAB_EDGE_REBATE &&
                editor->selection.slab_id == result->data.slab_feature.slab_id &&
                editor->selection.slab_feature_index == result->data.slab_feature.feature_index) {
                sitehelper_editor_clear_selection(editor);
            }
            break;

        case SITEHELPER_COMMAND_MOVE_SLAB_VERTEX:
            if (editor->active_tool == EDITOR_TOOL_SLAB_GEOMETRY &&
                editor->slab_geometry_tool.has_vertex &&
                result->data.slab_vertex.slab_id == editor->slab_geometry_tool.slab_id &&
                result->data.slab_vertex.feature_index == editor->slab_geometry_tool.feature_index &&
                result->data.slab_vertex.vertex_index == editor->slab_geometry_tool.vertex_index) {
                slab_geometry_tool_cancel(&editor->slab_geometry_tool);
                editor->slab_geometry_tool.active=1;
            }
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
    measurement_tool_cancel(&editor->measurement_tool);
    plan_dimension_tool_cancel(&editor->dimension_tool);
    if (editor->active_tool == EDITOR_TOOL_DIMENSION) { editor->dimension_tool.active=1; }
    plan_callout_tool_cancel(&editor->callout_tool);
    if (editor->active_tool == EDITOR_TOOL_CALLOUT) { editor->callout_tool.active=1; }
    plan_direction_symbol_tool_cancel(&editor->direction_symbol_tool);
    if (editor->active_tool == EDITOR_TOOL_VIEW_DIRECTION) {
        editor->direction_symbol_tool.active=1;
    }
    plan_revision_cloud_tool_cancel(&editor->revision_cloud_tool);
    if (editor->active_tool == EDITOR_TOOL_REVISION_CLOUD) {
        editor->revision_cloud_tool.active=1;
    }
    slab_tool_cancel(&editor->slab_tool);
    if (editor->active_tool == EDITOR_TOOL_SLAB) { editor->slab_tool.active=1; }
    slab_polygon_feature_tool_cancel(&editor->slab_penetration_tool);
    if (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION) {
        editor->slab_penetration_tool.active=1;
    }
    slab_polygon_feature_tool_cancel(&editor->slab_region_tool);
    if (editor->active_tool == EDITOR_TOOL_SLAB_REGION) {
        editor->slab_region_tool.active=1;
    }
    slab_edge_rebate_tool_cancel(&editor->slab_edge_rebate_tool);
    if (editor->active_tool == EDITOR_TOOL_SLAB_EDGE_REBATE) {
        editor->slab_edge_rebate_tool.active=1;
    }
    slab_geometry_tool_cancel(&editor->slab_geometry_tool);
    if (editor->active_tool == EDITOR_TOOL_SLAB_GEOMETRY) {
        editor->slab_geometry_tool.active=1;
    }
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

void sitehelper_editor_clear_wall_length(SiteHelperEditor *editor)
{
    if (editor != NULL) { wall_tool_clear_length(&editor->wall_tool); }
}

WallLengthStatus sitehelper_editor_set_wall_length(SiteHelperEditor *editor, int length_mm)
{
    if (!sitehelper_editor_has_wall_preview(editor)) { return WALL_LENGTH_INACTIVE; }
    return wall_tool_set_length(&editor->wall_tool, length_mm);
}

WallLengthStatus sitehelper_editor_create_wall_length_action(const SiteHelperEditor *editor,
    int length_mm, EditorAction *action)
{
    if (action == NULL) { return WALL_LENGTH_INACTIVE; }
    *action = (EditorAction){0};
    if (!sitehelper_editor_has_wall_preview(editor)) { return WALL_LENGTH_INACTIVE; }
    WallPlanSegment segment;
    WallLengthStatus status = wall_tool_resolve_length(&editor->wall_tool, length_mm, &segment);
    if (status != WALL_LENGTH_OK) { return status; }
    return editor_wall_action(editor, segment, action) ? WALL_LENGTH_OK : WALL_LENGTH_INACTIVE;
}

void sitehelper_editor_cancel_wall_placement(SiteHelperEditor *editor)
{
    if (editor != NULL) { wall_tool_cancel(&editor->wall_tool); sitehelper_editor_clear_snap(editor); }
}

int sitehelper_editor_get_measurement(const SiteHelperEditor *editor, PlanMeasurementQuery *query)
{
    if (query == NULL) { return 0; }
    *query = (PlanMeasurementQuery){0};
    return editor != NULL && editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_MEASURE &&
        measurement_tool_get_query(&editor->measurement_tool, query);
}

int sitehelper_editor_create_slab_action(const SiteHelperEditor *editor, EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_SLAB || editor->slab_tool.vertex_count < 3) { return 0; }
    CreateSlabCommand create={0};
    if (!create_slab_command_create(editor->slab_tool.storey_id,
        editor->slab_tool.vertices,editor->slab_tool.vertex_count,
        editor->slab_tool.thickness_mm,editor->slab_tool.top_level_offset_mm,&create)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    int ok=sitehelper_command_from_create_slab(&create,&action->command);
    create_slab_command_destroy(&create);
    if (!ok) { *action=(EditorAction){0}; }
    return ok;
}

int sitehelper_editor_create_plan_revision_cloud_action(const SiteHelperEditor *editor,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_REVISION_CLOUD ||
        editor->revision_cloud_tool.vertex_count < 3 ||
        editor->revision_cloud_tool.storey_id == DOMAIN_ID_INVALID) { return 0; }
    CreatePlanRevisionCloudCommand create={0};
    if (!create_plan_revision_cloud_command_create(editor->revision_cloud_tool.storey_id,
            editor->revision_cloud_tool.vertices,editor->revision_cloud_tool.vertex_count,
            &create)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    int ok=sitehelper_command_from_create_plan_revision_cloud(&create,&action->command);
    create_plan_revision_cloud_command_destroy(&create);
    if (!ok) { *action=(EditorAction){0}; }
    return ok;
}

int sitehelper_editor_create_active_polygon_action(const SiteHelperEditor *editor,
    EditorAction *action)
{
    if (editor != NULL && editor->active_tool == EDITOR_TOOL_SLAB) {
        return sitehelper_editor_create_slab_action(editor,action);
    }
    if (editor != NULL && editor->active_tool == EDITOR_TOOL_REVISION_CLOUD) {
        return sitehelper_editor_create_plan_revision_cloud_action(editor,action);
    }
    return editor_polygon_feature_action(editor,action);
}

int sitehelper_editor_create_delete_selection_action(const SiteHelperEditor *editor,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->selection.scope != EDITOR_SELECTION_SCOPE_PLAN) { return 0; }
    SiteHelperCommand command={0};
    int ok=0;
    switch (editor->selection.kind) {
        case EDITOR_SELECTION_DOCUMENT: {
            DomainId id = editor->selection.document.id;
            switch (editor->selection.document.kind) {
                case DOCUMENT_OBJECT_NOTE: {
                    DeletePlanNoteCommand deletion;
                    ok=delete_plan_note_command_create(id,&deletion) &&
                        sitehelper_command_from_delete_plan_note(&deletion,&command);
                    break;
                }
                case DOCUMENT_OBJECT_DIMENSION: {
                    DeletePlanDimensionCommand deletion;
                    ok=delete_plan_dimension_command_create(id,&deletion) &&
                        sitehelper_command_from_delete_plan_dimension(&deletion,&command);
                    break;
                }
                case DOCUMENT_OBJECT_SYMBOL: {
                    DeletePlanSymbolCommand deletion;
                    ok=delete_plan_symbol_command_create(id,&deletion) &&
                        sitehelper_command_from_delete_plan_symbol(&deletion,&command);
                    break;
                }
                case DOCUMENT_OBJECT_CALLOUT: {
                    DeletePlanCalloutCommand deletion;
                    ok=delete_plan_callout_command_create(id,&deletion) &&
                        sitehelper_command_from_delete_plan_callout(&deletion,&command);
                    break;
                }
                case DOCUMENT_OBJECT_REVISION_CLOUD: {
                    DeletePlanRevisionCloudCommand deletion;
                    ok=delete_plan_revision_cloud_command_create(id,&deletion) &&
                        sitehelper_command_from_delete_plan_revision_cloud(&deletion,&command);
                    break;
                }
                case DOCUMENT_OBJECT_NONE:
                default:
                    break;
            }
            break;
        }
        case EDITOR_SELECTION_SLAB: {
            DeleteSlabCommand deletion;
            ok=delete_slab_command_create(editor->selection.slab_id,&deletion) &&
                sitehelper_command_from_delete_slab(&deletion,&command);
            break;
        }
        case EDITOR_SELECTION_SLAB_PENETRATION: {
            DeleteSlabPenetrationCommand deletion;
            ok=delete_slab_penetration_command_create(editor->selection.slab_id,
                editor->selection.slab_feature_index,&deletion) &&
                sitehelper_command_from_delete_slab_penetration(&deletion,&command);
            break;
        }
        case EDITOR_SELECTION_SLAB_REGION: {
            DeleteSlabRegionCommand deletion;
            ok=delete_slab_region_command_create(editor->selection.slab_id,
                editor->selection.slab_feature_index,&deletion) &&
                sitehelper_command_from_delete_slab_region(&deletion,&command);
            break;
        }
        case EDITOR_SELECTION_SLAB_EDGE_REBATE: {
            DeleteSlabEdgeRebateCommand deletion;
            ok=delete_slab_edge_rebate_command_create(editor->selection.slab_id,
                editor->selection.slab_feature_index,&deletion) &&
                sitehelper_command_from_delete_slab_edge_rebate(&deletion,&command);
            break;
        }
        default: break;
    }
    if (!ok) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND,.command=command};
    return 1;
}


int sitehelper_editor_get_plan_dimension_preview(const SiteHelperEditor *editor,
    DocumentPlanDimensionGeometry *geometry, int *distance_mm, int *ready)
{
    if (geometry == NULL || distance_mm == NULL || ready == NULL) { return 0; }
    *geometry=(DocumentPlanDimensionGeometry){0}; *distance_mm=0; *ready=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_DIMENSION ||
        !plan_dimension_tool_has_started(&editor->dimension_tool) ||
        !editor->dimension_tool.has_pointer) { return 0; }
    const PlanDimensionTool *tool=&editor->dimension_tool;
    PlanPosition second=tool->second_position;
    int offset=tool->offset_mm;
    if (tool->stage == PLAN_DIMENSION_TOOL_PICK_SECOND) {
        if (!plan_position_from_point(tool->pointer,&second) ||
            (second.x == tool->first_position.x && second.y == tool->first_position.y)) { return 0; }
        offset=0;
    } else if (tool->stage != PLAN_DIMENSION_TOOL_PLACE_OFFSET) { return 0; }
    int distance=wall_plan_segment_length_mm((WallPlanSegment){tool->first_position,second});
    if (distance <= 0 || !document_plan_dimension_geometry(tool->first_position,second,
            offset,geometry)) { return 0; }
    *distance_mm=distance;
    *ready=tool->stage == PLAN_DIMENSION_TOOL_PLACE_OFFSET;
    return 1;
}

int sitehelper_editor_create_plan_dimension_action(const SiteHelperEditor *editor,
    DocumentDimensionReference first, DocumentDimensionReference second, int offset_mm,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    CreatePlanDimensionCommand create;
    if (!create_plan_dimension_command_create(editor->current_storey_id,first,second,
            offset_mm,&create)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    if (!sitehelper_command_from_create_plan_dimension(&create,&action->command)) {
        *action=(EditorAction){0}; return 0;
    }
    return 1;
}

int sitehelper_editor_create_edit_plan_dimension_action(const SiteHelperEditor *editor,
    DocumentDimensionReference first, DocumentDimensionReference second, int offset_mm,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        !editor_selection_is_document_kind(&editor->selection,DOCUMENT_OBJECT_DIMENSION) ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    EditPlanDimensionCommand edit;
    if (!edit_plan_dimension_command_create(editor->selection.document.id,
            editor->current_storey_id,first,second,offset_mm,&edit)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    if (!sitehelper_command_from_edit_plan_dimension(&edit,&action->command)) {
        *action=(EditorAction){0}; return 0;
    }
    return 1;
}

int sitehelper_editor_create_plan_symbol_action(const SiteHelperEditor *editor,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    CreatePlanSymbolCommand create;
    if (!create_plan_symbol_command_create(editor->current_storey_id,kind,anchor,direction,&create)) {
        return 0;
    }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    if (!sitehelper_command_from_create_plan_symbol(&create,&action->command)) {
        *action=(EditorAction){0}; return 0;
    }
    return 1;
}

int sitehelper_editor_create_edit_plan_symbol_action(const SiteHelperEditor *editor,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        !editor_selection_is_document_kind(&editor->selection,DOCUMENT_OBJECT_SYMBOL) ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    EditPlanSymbolCommand edit;
    if (!edit_plan_symbol_command_create(editor->selection.document.id,
            editor->current_storey_id,kind,anchor,direction,&edit)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    if (!sitehelper_command_from_edit_plan_symbol(&edit,&action->command)) {
        *action=(EditorAction){0}; return 0;
    }
    return 1;
}

int sitehelper_editor_create_plan_callout_action(const SiteHelperEditor *editor,
    PlanPosition target, PlanPosition label_anchor, const char *text, EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    CreatePlanCalloutCommand create={0};
    if (!create_plan_callout_command_create(editor->current_storey_id,target,label_anchor,
            text,&create)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    int ok=sitehelper_command_from_create_plan_callout(&create,&action->command);
    create_plan_callout_command_destroy(&create);
    if (!ok) { *action=(EditorAction){0}; }
    return ok;
}

int sitehelper_editor_create_edit_plan_callout_action(const SiteHelperEditor *editor,
    PlanPosition target, PlanPosition label_anchor, const char *text, EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        !editor_selection_is_document_kind(&editor->selection,DOCUMENT_OBJECT_CALLOUT) ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    EditPlanCalloutCommand edit={0};
    if (!edit_plan_callout_command_create(editor->selection.document.id,
            editor->current_storey_id,target,label_anchor,text,&edit)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    int ok=sitehelper_command_from_edit_plan_callout(&edit,&action->command);
    edit_plan_callout_command_destroy(&edit);
    if (!ok) { *action=(EditorAction){0}; }
    return ok;
}

int sitehelper_editor_get_plan_callout_ready(const SiteHelperEditor *editor,
    PlanPosition *target, PlanPosition *label_anchor)
{
    return editor != NULL && editor->active_view == EDITOR_VIEW_PLAN &&
        editor->active_tool == EDITOR_TOOL_CALLOUT &&
        plan_callout_tool_ready(&editor->callout_tool,target,label_anchor);
}

int sitehelper_editor_get_plan_callout_preview(const SiteHelperEditor *editor,
    PlanPosition *target, PlanPoint *label, int *ready)
{
    if (target == NULL || label == NULL || ready == NULL) { return 0; }
    *target=(PlanPosition){0}; *label=(PlanPoint){0}; *ready=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_CALLOUT ||
        editor->callout_tool.stage == PLAN_CALLOUT_TOOL_PICK_TARGET) { return 0; }
    const PlanCalloutTool *tool=&editor->callout_tool;
    *target=tool->target;
    if (tool->stage == PLAN_CALLOUT_TOOL_READY_TEXT) {
        *label=(PlanPoint){tool->label_anchor.x,tool->label_anchor.y};
        *ready=1;
        return 1;
    }
    if (!tool->has_pointer) { return 0; }
    *label=tool->pointer;
    return 1;
}

void sitehelper_editor_reset_plan_callout_tool(SiteHelperEditor *editor)
{
    if (editor == NULL) { return; }
    plan_callout_tool_cancel(&editor->callout_tool);
    if (editor->active_tool == EDITOR_TOOL_CALLOUT) { editor->callout_tool.active=1; }
    sitehelper_editor_clear_snap(editor);
}

int sitehelper_editor_get_view_direction_preview(const SiteHelperEditor *editor,
    PlanPosition *anchor, PlanPoint *direction_point, int *ready)
{
    if (anchor == NULL || direction_point == NULL || ready == NULL) { return 0; }
    *anchor=(PlanPosition){0}; *direction_point=(PlanPoint){0}; *ready=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_VIEW_DIRECTION ||
        editor->direction_symbol_tool.stage == PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR) {
        return 0;
    }
    const PlanDirectionSymbolTool *tool=&editor->direction_symbol_tool;
    *anchor=tool->anchor;
    if (!tool->has_pointer) { return 0; }
    *direction_point=tool->pointer;
    DocumentPlanDirection direction;
    PlanPosition point;
    if (plan_position_from_point(*direction_point,&point) &&
        document_plan_direction_from_points(*anchor,point,&direction)) {
        *ready=1;
    }
    return 1;
}

int sitehelper_editor_create_plan_note_action(const SiteHelperEditor *editor,
    PlanPosition position, DomainId target_id, const char *text, EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    CreatePlanNoteCommand create={0};
    if (!create_plan_note_command_create(editor->current_storey_id,position,target_id,text,&create)) {
        return 0;
    }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    int ok=sitehelper_command_from_create_plan_note(&create,&action->command);
    create_plan_note_command_destroy(&create);
    if (!ok) { *action=(EditorAction){0}; }
    return ok;
}

int sitehelper_editor_create_edit_plan_note_action(const SiteHelperEditor *editor,
    PlanPosition position, DomainId target_id, const char *text, EditorAction *action)
{
    if (editor == NULL || action == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        !editor_selection_is_document_kind(&editor->selection,DOCUMENT_OBJECT_NOTE) ||
        editor->current_storey_id == DOMAIN_ID_INVALID) { return 0; }
    EditPlanNoteCommand edit={0};
    if (!edit_plan_note_command_create(editor->selection.document.id,
            editor->current_storey_id,position,target_id,text,&edit)) { return 0; }
    *action=(EditorAction){.kind=EDITOR_ACTION_COMMAND};
    int ok=sitehelper_command_from_edit_plan_note(&edit,&action->command);
    edit_plan_note_command_destroy(&edit);
    if (!ok) { *action=(EditorAction){0}; }
    return ok;
}

int sitehelper_editor_prepare_plan_note_authoring(SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 view_position,
    DomainId *annotation_id, PlanPosition *position)
{
    if (annotation_id == NULL || position == NULL) { return 0; }
    *annotation_id=DOMAIN_ID_INVALID;
    if (editor == NULL || project == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_NOTE || editor->current_storey_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    const Storey *storey=sitehelper_project_find_storey_by_id_const(project,
        editor->current_storey_id);
    if (storey == NULL) { return 0; }
    DomainId hit=document_plan_find_note_at_position(&project->document,storey->id,
        (PlanPoint){view_position.x,view_position.y},editor->snap.settings.object_snap_tolerance);
    if (hit != DOMAIN_ID_INVALID) {
        const DocumentAnnotation *note=sitehelper_project_find_annotation_by_id_const(project,hit);
        if (note == NULL || note->kind != DOCUMENT_ANNOTATION_NOTE) { return 0; }
        editor_selection_set_annotation(&editor->selection,EDITOR_SELECTION_SCOPE_PLAN,hit);
        editor->current_wall_id=DOMAIN_ID_INVALID;
        sitehelper_editor_clear_snap(editor);
        *annotation_id=hit;
        *position=note->anchor.position;
        return 1;
    }
    sitehelper_editor_clear_selection(editor);
    editor->current_wall_id=DOMAIN_ID_INVALID;
    sitehelper_editor_update_snap_in_project(editor,project,view_position);
    Vec2 point=editor->snap.result.type == SNAP_NONE ? view_position : editor->snap.result.position;
    return plan_position_from_point((PlanPoint){point.x,point.y},position);
}

int sitehelper_editor_get_slab_preview(const SiteHelperEditor *editor,
    const PlanPosition **vertices, size_t *count, PlanPoint *preview, int *has_preview)
{
    if (vertices == NULL || count == NULL || preview == NULL || has_preview == NULL) { return 0; }
    *vertices=NULL; *count=0; *preview=(PlanPoint){0}; *has_preview=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_SLAB || editor->slab_tool.vertex_count == 0) { return 0; }
    *vertices=editor->slab_tool.vertices; *count=editor->slab_tool.vertex_count;
    *preview=editor->slab_tool.preview; *has_preview=editor->slab_tool.has_preview;
    return 1;
}

int sitehelper_editor_get_slab_feature_polygon_preview(
    const SiteHelperEditor *editor, EditorSlabPolygonPreviewKind *kind,
    DomainId *slab_id, const PlanPosition **vertices, size_t *count,
    PlanPoint *preview, int *has_preview)
{
    if (kind == NULL || slab_id == NULL || vertices == NULL || count == NULL ||
        preview == NULL || has_preview == NULL) { return 0; }
    *kind=EDITOR_SLAB_POLYGON_PREVIEW_NONE; *slab_id=DOMAIN_ID_INVALID;
    *vertices=NULL; *count=0; *preview=(PlanPoint){0}; *has_preview=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN) { return 0; }
    const SlabPolygonFeatureTool *tool;
    if (editor->active_tool == EDITOR_TOOL_SLAB_PENETRATION) {
        tool=&editor->slab_penetration_tool;
        *kind=EDITOR_SLAB_POLYGON_PREVIEW_PENETRATION;
    } else if (editor->active_tool == EDITOR_TOOL_SLAB_REGION) {
        tool=&editor->slab_region_tool;
        *kind=EDITOR_SLAB_POLYGON_PREVIEW_REGION;
    } else { return 0; }
    if (tool->vertex_count == 0) { *kind=EDITOR_SLAB_POLYGON_PREVIEW_NONE; return 0; }
    *slab_id=tool->slab_id; *vertices=tool->vertices; *count=tool->vertex_count;
    *preview=tool->preview; *has_preview=tool->has_preview;
    return 1;
}

int sitehelper_editor_get_slab_rebate_preview(const SiteHelperEditor *editor,
    DomainId *slab_id, size_t *edge_index, PlanPoint *start, PlanPoint *end,
    int *has_end)
{
    if (slab_id == NULL || edge_index == NULL || start == NULL || end == NULL ||
        has_end == NULL) { return 0; }
    *slab_id=DOMAIN_ID_INVALID; *edge_index=SIZE_MAX; *start=(PlanPoint){0};
    *end=(PlanPoint){0}; *has_end=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_SLAB_EDGE_REBATE ||
        !editor->slab_edge_rebate_tool.has_start) { return 0; }
    const SlabEdgeRebateTool *tool=&editor->slab_edge_rebate_tool;
    *slab_id=tool->slab_id; *edge_index=tool->edge_index; *start=tool->start_point;
    *end=tool->hover_point; *has_end=tool->has_hover;
    return 1;
}

int sitehelper_editor_get_slab_geometry_overlay(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorSlabGeometryOverlay *output)
{
    if (output == NULL) { return 0; }
    *output=(EditorSlabGeometryOverlay){.feature_index=SIZE_MAX,
        .active_vertex_index=SIZE_MAX};
    if (editor == NULL || project == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_SLAB_GEOMETRY) { return 0; }
    const Storey *storey=sitehelper_project_find_storey_by_id_const(project,
        editor->current_storey_id);
    if (storey == NULL) { return 0; }

    EditorSlabGeometryKind kind;
    DomainId slab_id;
    size_t feature_index;
    const SlabGeometryTool *tool=&editor->slab_geometry_tool;
    if (tool->has_vertex) {
        if (tool->storey_id != storey->id) { return 0; }
        kind=tool->kind;
        slab_id=tool->slab_id;
        feature_index=tool->feature_index;
    } else if (!editor_slab_geometry_target_from_selection(&editor->selection,&kind,
            &slab_id,&feature_index)) {
        return 0;
    }
    const SlabOutline *outline=editor_slab_geometry_outline(storey,slab_id,kind,
        feature_index);
    if (outline == NULL) { return 0; }
    output->kind=kind;
    output->slab_id=slab_id;
    output->feature_index=feature_index;
    output->vertices=outline->vertices;
    output->vertex_count=outline->vertex_count;
    if (tool->has_vertex && tool->vertex_index < outline->vertex_count) {
        output->active_vertex_index=tool->vertex_index;
        output->preview=tool->preview;
        output->has_preview=tool->has_preview;
    }
    return 1;
}

int sitehelper_editor_get_plan_revision_cloud_preview(
    const SiteHelperEditor *editor, const PlanPosition **vertices, size_t *count,
    PlanPoint *preview, int *has_preview)
{
    if (vertices == NULL || count == NULL || preview == NULL || has_preview == NULL) { return 0; }
    *vertices=NULL; *count=0; *preview=(PlanPoint){0}; *has_preview=0;
    if (editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
        editor->active_tool != EDITOR_TOOL_REVISION_CLOUD ||
        editor->revision_cloud_tool.vertex_count == 0) { return 0; }
    *vertices=editor->revision_cloud_tool.vertices;
    *count=editor->revision_cloud_tool.vertex_count;
    *preview=editor->revision_cloud_tool.preview;
    *has_preview=editor->revision_cloud_tool.has_preview;
    return 1;
}

int sitehelper_editor_cancel_tool_interaction(SiteHelperEditor *editor)
{
    if (editor == NULL) { return 0; }
    switch (editor->active_tool) {
        case EDITOR_TOOL_MEASURE:
            measurement_tool_cancel(&editor->measurement_tool);
            sitehelper_editor_clear_snap(editor);
            return 1;
        case EDITOR_TOOL_WALL:
            if (!sitehelper_editor_has_wall_preview(editor)) { return 0; }
            sitehelper_editor_cancel_wall_placement(editor);
            return 1;
        case EDITOR_TOOL_DIMENSION:
            if (!plan_dimension_tool_has_started(&editor->dimension_tool)) { return 0; }
            plan_dimension_tool_cancel(&editor->dimension_tool);
            editor->dimension_tool.active=1;
            sitehelper_editor_clear_snap(editor);
            return 1;
        case EDITOR_TOOL_CALLOUT:
            if (editor->callout_tool.stage == PLAN_CALLOUT_TOOL_PICK_TARGET) { return 0; }
            plan_callout_tool_cancel(&editor->callout_tool);
            editor->callout_tool.active=1;
            sitehelper_editor_clear_snap(editor);
            return 1;
        case EDITOR_TOOL_VIEW_DIRECTION:
            if (editor->direction_symbol_tool.stage == PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR) {
                return 0;
            }
            plan_direction_symbol_tool_cancel(&editor->direction_symbol_tool);
            editor->direction_symbol_tool.active=1;
            sitehelper_editor_clear_snap(editor);
            return 1;
        case EDITOR_TOOL_REVISION_CLOUD:
            if (editor->revision_cloud_tool.vertex_count == 0) { return 0; }
            plan_revision_cloud_tool_cancel(&editor->revision_cloud_tool);
            editor->revision_cloud_tool.active=1;
            sitehelper_editor_clear_snap(editor);
            return 1;
        case EDITOR_TOOL_SLAB:
            if (editor->slab_tool.vertex_count == 0) { return 0; }
            slab_tool_cancel(&editor->slab_tool); editor->slab_tool.active=1;
            sitehelper_editor_clear_snap(editor); return 1;
        case EDITOR_TOOL_SLAB_PENETRATION:
            if (editor->slab_penetration_tool.vertex_count == 0) { return 0; }
            slab_polygon_feature_tool_cancel(&editor->slab_penetration_tool);
            editor->slab_penetration_tool.active=1;
            sitehelper_editor_clear_snap(editor); return 1;
        case EDITOR_TOOL_SLAB_REGION:
            if (editor->slab_region_tool.vertex_count == 0) { return 0; }
            slab_polygon_feature_tool_cancel(&editor->slab_region_tool);
            editor->slab_region_tool.active=1;
            sitehelper_editor_clear_snap(editor); return 1;
        case EDITOR_TOOL_SLAB_EDGE_REBATE:
            if (!editor->slab_edge_rebate_tool.has_start) { return 0; }
            slab_edge_rebate_tool_cancel(&editor->slab_edge_rebate_tool);
            editor->slab_edge_rebate_tool.active=1;
            sitehelper_editor_clear_snap(editor); return 1;
        case EDITOR_TOOL_SLAB_GEOMETRY:
            if (!editor->slab_geometry_tool.has_vertex) { return 0; }
            slab_geometry_tool_cancel(&editor->slab_geometry_tool);
            editor->slab_geometry_tool.active=1;
            sitehelper_editor_clear_snap(editor); return 1;
        default: return 0;
    }
}
