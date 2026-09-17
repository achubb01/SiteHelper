#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"
#include "editor_tool.h"
#include "editor_selection.h"
#include "editor_snap_state.h"
#include "opening_tool.h"
#include "opening_placement.h"
#include "wall_tool.h"
#include "measurement_tool.h"
#include "slab_tool.h"
#include "slab_feature_tool.h"
#include "slab_geometry_tool.h"
#include "opening_command.h"
#include "editor_action.h"
#include "sitehelper_project.h"

/* Transient view state; never part of SiteHelperProject. */
typedef enum
{
    EDITOR_VIEW_PLAN,
    EDITOR_VIEW_WALL_ELEVATION,
    EDITOR_VIEW_COUNT
} EditorView;

/* Owns SlabTool and polygon-feature vertex storage after a sketch begins.
 * Initialize/destroy; never shallow-copy an editor containing owned storage. */
typedef struct
{
    /* Navigation/focus, independent of the single transient selection. */
    DomainId current_storey_id;
    /* Independent room navigation; never gates physical wall access. */
    DomainId current_room_id;
    DomainId current_wall_id;

    EditorView active_view;
    EditorTool active_tool;

    /* Non-empty scope must match active_view; changing views clears it. */
    EditorSelection selection;
    EditorSnapState snap;

    OpeningTool opening_tool;
    OpeningPlacement opening_placement;
    WallTool wall_tool;
    MeasurementTool measurement_tool;
    SlabTool slab_tool;
    SlabPolygonFeatureTool slab_penetration_tool;
    SlabPolygonFeatureTool slab_region_tool;
    SlabEdgeRebateTool slab_edge_rebate_tool;
    SlabGeometryTool slab_geometry_tool;
} SiteHelperEditor;

void sitehelper_editor_init(
    SiteHelperEditor *editor
);
void sitehelper_editor_destroy(SiteHelperEditor *editor);

/* Switching clears Room/Wall navigation, selection and previews. Invalid ID
 * clears active Storey; a missing nonzero ID fails without changing state. */
int sitehelper_editor_set_current_storey(SiteHelperEditor *editor,
    const SiteHelperProject *project, DomainId storey_id);

/* Application preview boundary: resolve the active Storey on every update.
 * No effective construction settings are cached in editor state. */
void sitehelper_editor_pointer_move_in_project(SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 view_position);

int sitehelper_editor_tool_available(EditorView view, EditorTool tool);
int sitehelper_editor_set_active_view(SiteHelperEditor *editor, EditorView view);

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

/* Wall-local U/Z hit testing requires the Elevation view. */
void sitehelper_editor_select_wall_member_at_position(
    SiteHelperEditor *editor,
    const Wall *wall,
    WallLocalPosition position
);

/* Non-empty selection context matches Plan or Wall Elevation respectively.
 * Does not infer ownership from navigation; property resolution uses IDs and
 * current Storey, while elevation rendering also checks the viewed Wall. */
int sitehelper_editor_selection_matches_view(const SiteHelperEditor *editor);

/* Explicit roof-authoring selection hook. It queries authoritative support
 * polygons in Plan space and does not change the ordinary SELECT precedence
 * between walls/slabs. This lets a future roof workspace/tool opt into roof
 * selection without making overlapping floor/roof domains ambiguous today. */
int sitehelper_editor_select_roof_at_position(SiteHelperEditor *editor,
    const Storey *storey, PlanPoint point, double tolerance_mm);

void sitehelper_editor_reconcile_wall_selection(
    SiteHelperEditor *editor,
    const Wall *wall
);

void sitehelper_editor_reconcile(
    SiteHelperEditor *editor,
    const SiteHelperProject *project
);

/* Call after transactionally replacing/loading the Project object. Selection is
 * transient and is cleared even if the new project reuses the same IDs/indices;
 * navigation is then reconciled against the replacement. */
void sitehelper_editor_project_replaced(SiteHelperEditor *editor,
    const SiteHelperProject *project);

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

/* Fresh active-Storey Plan candidates (mm); elevation retains framing U/Z
 * candidates. Called once per project pointer/click event before tool handling.
 * No Project mutation; missing context clears the transient snap result. */
void sitehelper_editor_update_snap_in_project(SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 position);

/* Low-level path: generated framing in elevation, grid-only in Plan. */
void sitehelper_editor_update_snap(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 position
);

/* Pointer APIs receive millimetres in the active view after camera unprojection:
 * plan X/Y or wall-local elevation U/Z, with fractional coordinates allowed.
 * Preview geometry and editor snap distances use these same physical units.
 * This low-level helper requires resolved construction settings. Applications
 * should use pointer_move_in_project, which resolves the active Storey and
 * its Plan geometry. */
void sitehelper_editor_pointer_move(
    SiteHelperEditor *editor,
    const Wall *wall,
    const BuildSettings *settings,
    Vec2 view_position
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

/* Plan clicks resolve fresh grid-only snapping at view_position. Use the
 * project-aware entry point below to include physical Plan geometry. */
int sitehelper_editor_primary_action(
    SiteHelperEditor *editor,
    const Wall *wall,
    Vec2 view_position,
    EditorAction *action
);

/* Plan clicks resolve current Storey geometry at the actual event position,
 * then tools consume that result without another low-level snap update.
 * Select in Plan gives visible Walls precedence, then selects slab features in
 * rebate/penetration/region/slab order. Wall selection also updates navigation.
 * Elevation preserves member hit precedence, then tests clear Opening geometry
 * with owning-Storey settings. The lower-level primary_action lacks Project
 * settings and therefore retains member-only elevation selection. */
int sitehelper_editor_primary_action_in_project(
    SiteHelperEditor *editor,
    const SiteHelperProject *project,
    Vec2 view_position,
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

/* Application input consumes only typed millimetres. Preview and commit reject
 * nonpositive values. Clearing restores mouse placement. No Project mutation. */
void sitehelper_editor_clear_wall_length(SiteHelperEditor *editor);
WallLengthStatus sitehelper_editor_set_wall_length(SiteHelperEditor *editor, int length_mm);
WallLengthStatus sitehelper_editor_create_wall_length_action(const SiteHelperEditor *editor,
    int length_mm, EditorAction *action);
void sitehelper_editor_cancel_wall_placement(SiteHelperEditor *editor);

int sitehelper_editor_has_wall_preview(const SiteHelperEditor *editor);
int sitehelper_editor_get_wall_preview_segment(
    const SiteHelperEditor *editor,
    WallPlanSegment *segment
);

/* Copied transient Plan query. Zero distance is valid; no Wall length range
 * restriction. Absence clears output. Application must not inspect tool state. */
int sitehelper_editor_get_measurement(const SiteHelperEditor *editor, PlanMeasurementQuery *query);
int sitehelper_editor_create_slab_action(const SiteHelperEditor *editor, EditorAction *action);
/* Enter commits the active slab/polygon-feature sketch. */
int sitehelper_editor_create_active_polygon_action(const SiteHelperEditor *editor,
    EditorAction *action);
int sitehelper_editor_create_delete_selection_action(const SiteHelperEditor *editor,
    EditorAction *action);
int sitehelper_editor_get_slab_preview(const SiteHelperEditor *editor,
    const PlanPosition **vertices, size_t *count, PlanPoint *preview,
    int *has_preview);
typedef enum {
    EDITOR_SLAB_POLYGON_PREVIEW_NONE = 0,
    EDITOR_SLAB_POLYGON_PREVIEW_PENETRATION,
    EDITOR_SLAB_POLYGON_PREVIEW_REGION
} EditorSlabPolygonPreviewKind;
int sitehelper_editor_get_slab_feature_polygon_preview(
    const SiteHelperEditor *editor, EditorSlabPolygonPreviewKind *kind,
    DomainId *slab_id, const PlanPosition **vertices, size_t *count,
    PlanPoint *preview, int *has_preview);
int sitehelper_editor_get_slab_rebate_preview(const SiteHelperEditor *editor,
    DomainId *slab_id, size_t *edge_index, PlanPoint *start, PlanPoint *end,
    int *has_end);

typedef struct {
    EditorSlabGeometryKind kind;
    DomainId slab_id;
    size_t feature_index;
    const PlanPosition *vertices;
    size_t vertex_count;
    size_t active_vertex_index;
    PlanPoint preview;
    int has_preview;
} EditorSlabGeometryOverlay;

/* Read-only overlay description for the current geometry-edit target. Borrowed
 * vertices remain owned by the Project and are valid only until mutation. */
int sitehelper_editor_get_slab_geometry_overlay(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorSlabGeometryOverlay *output);
/* Cancel an active tool interaction without changing tools or Project state.
 * Returns whether handled. Application gives focused text input first refusal. */
int sitehelper_editor_cancel_tool_interaction(SiteHelperEditor *editor);

void sitehelper_editor_invalidate_transient_state(
    SiteHelperEditor *editor
);

#endif
