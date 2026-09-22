#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"
#include "editor_context.h"
#include "editor_selection.h"
#include "editor_snap_state.h"
#include "opening_tool.h"
#include "opening_placement.h"
#include "wall_tool.h"
#include "measurement_tool.h"
#include "plan_dimension_tool.h"
#include "plan_callout_tool.h"
#include "plan_direction_symbol_tool.h"
#include "plan_revision_cloud_tool.h"
#include "slab_tool.h"
#include "slab_feature_tool.h"
#include "slab_geometry_tool.h"
#include "opening_command.h"
#include "editor_action.h"
#include "sitehelper_project.h"
#include "plan_dimension_geometry.h"

typedef struct
{
    /* Transient Wall-elevation hover. Member identity remains by value for the
     * same regeneration-safety reason as EditorSelection. Opening identity is
     * stable. Both are cleared whenever pointer/tool/view context is invalidated. */
    DomainId wall_id;
    DomainId opening_id;
    WallSelection member;
} WallElevationHover;

typedef enum
{
    WALL_OPENING_EDIT_HANDLE_NONE = 0,
    WALL_OPENING_EDIT_HANDLE_MOVE,
    WALL_OPENING_EDIT_HANDLE_LEFT,
    WALL_OPENING_EDIT_HANDLE_RIGHT,
    WALL_OPENING_EDIT_HANDLE_BOTTOM,
    WALL_OPENING_EDIT_HANDLE_TOP
} WallOpeningEditHandle;

typedef struct
{
    /* Direct manipulation is an editor transaction preview. The Project remains
     * unchanged until one EDIT_OPENING command is emitted on release. */
    DomainId wall_id;
    DomainId opening_id;
    WallOpeningEditHandle hovered_handle;
    WallOpeningEditHandle active_handle;
    WallLocalPosition anchor;
    Opening original;
    Opening candidate;
    WallOpeningValidation validation;
    bool active;
} WallOpeningEdit;

/* Owns SlabTool and polygon-feature vertex storage after a sketch begins.
 * Initialize/destroy; never shallow-copy an editor containing owned storage. */
typedef struct
{
    /* Navigation/focus, independent of the single transient selection. */
    DomainId current_storey_id;
    /* Independent room navigation; never gates physical wall access. */
    DomainId current_room_id;
    DomainId current_wall_id;

    EditorWorkspace active_workspace;
    EditorView active_view;
    EditorTool active_tool;

    /* Non-empty scope must match active_view; changing views clears it. */
    EditorSelection selection;
    WallElevationHover wall_elevation_hover;
    WallOpeningEdit wall_opening_edit;
    EditorSnapState snap;

    OpeningTool opening_tool;
    OpeningPlacement opening_placement;
    WallTool wall_tool;
    MeasurementTool measurement_tool;
    PlanDimensionTool dimension_tool;
    PlanCalloutTool callout_tool;
    PlanDirectionSymbolTool direction_symbol_tool;
    PlanRevisionCloudTool revision_cloud_tool;
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

/* Effective tool availability combines transient workspace exposure with
 * mechanical view capability. */
int sitehelper_editor_tool_available(const SiteHelperEditor *editor, EditorTool tool);

EditorWorkspace sitehelper_editor_get_active_workspace(const SiteHelperEditor *editor);
/* Does not change view/camera. Fails if the current view is unsupported by the
 * destination workspace. Successful changes preserve navigation, clear selection
 * and in-progress interactions, and fall back to SELECT if the tool is hidden. */
int sitehelper_editor_set_active_workspace(SiteHelperEditor *editor,
    EditorWorkspace workspace);
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

/* Explicit roof-source selection hook. It queries authoritative support
 * polygons in Plan space. Workspace-aware SELECT routing calls this from the
 * Roof workspace so overlapping slab/wall geometry does not determine roof
 * selection precedence. */
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

/* Framing hover is presentation-only and exists only for SELECT in Wall
 * Elevation. Getters require the explicit viewed Wall owner. */
const WallSelection *sitehelper_editor_get_hovered_wall_member(
    const SiteHelperEditor *editor, DomainId wall_id);
DomainId sitehelper_editor_get_hovered_opening(
    const SiteHelperEditor *editor, DomainId wall_id);

/* Selected-opening direct manipulation exists only in the Framing workspace,
 * Wall Elevation and SELECT. Grip tolerance is supplied in wall-local mm by the
 * application so grip hit areas can remain stable in screen pixels. */
void sitehelper_editor_update_opening_edit_hover_in_project(
    SiteHelperEditor *editor, const SiteHelperProject *project,
    Vec2 view_position, double grip_tolerance_mm);
int sitehelper_editor_begin_opening_edit_in_project(
    SiteHelperEditor *editor, const SiteHelperProject *project,
    Vec2 view_position, double grip_tolerance_mm);
void sitehelper_editor_update_opening_edit_in_project(
    SiteHelperEditor *editor, const SiteHelperProject *project,
    Vec2 view_position);
int sitehelper_editor_create_opening_edit_action(
    const SiteHelperEditor *editor, EditorAction *action);
void sitehelper_editor_cancel_opening_edit(SiteHelperEditor *editor);
int sitehelper_editor_get_opening_edit(
    const SiteHelperEditor *editor, DomainId wall_id, DomainId opening_id,
    WallOpeningEdit *edit);

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
/* Note authoring boundary for application text UI. Commands own copied text;
 * these helpers do not introduce keyboard focus or persistent typography. */

/* Transient authoring preview. Before the second reference is fixed, the
 * second point follows the snapped pointer with zero offset. Afterwards the
 * same derived geometry follows the raw pointer's signed perpendicular offset. */
int sitehelper_editor_get_plan_dimension_preview(const SiteHelperEditor *editor,
    DocumentPlanDimensionGeometry *geometry, int *distance_mm, int *ready);

int sitehelper_editor_create_plan_dimension_action(const SiteHelperEditor *editor,
    DocumentDimensionReference first, DocumentDimensionReference second, int offset_mm,
    EditorAction *action);
int sitehelper_editor_create_edit_plan_dimension_action(const SiteHelperEditor *editor,
    DocumentDimensionReference first, DocumentDimensionReference second, int offset_mm,
    EditorAction *action);

int sitehelper_editor_create_plan_symbol_action(const SiteHelperEditor *editor,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    EditorAction *action);
int sitehelper_editor_create_edit_plan_symbol_action(const SiteHelperEditor *editor,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    EditorAction *action);
/* Two-click view-direction authoring preview. First click fixes anchor; second
 * point defines an exact primitive integer direction vector. */
int sitehelper_editor_get_view_direction_preview(const SiteHelperEditor *editor,
    PlanPosition *anchor, PlanPoint *direction_point, int *ready);

int sitehelper_editor_create_plan_callout_action(const SiteHelperEditor *editor,
    PlanPosition target, PlanPosition label_anchor, const char *text, EditorAction *action);
int sitehelper_editor_create_edit_plan_callout_action(const SiteHelperEditor *editor,
    PlanPosition target, PlanPosition label_anchor, const char *text, EditorAction *action);
/* Returns a completed two-point authoring geometry waiting for text. The caller
 * should transfer it to keyboard focus then reset the transient tool. */
int sitehelper_editor_get_plan_callout_ready(const SiteHelperEditor *editor,
    PlanPosition *target, PlanPosition *label_anchor);
/* Read-only transient preview after the target click. */
int sitehelper_editor_get_plan_callout_preview(const SiteHelperEditor *editor,
    PlanPosition *target, PlanPoint *label, int *ready);
void sitehelper_editor_reset_plan_callout_tool(SiteHelperEditor *editor);

int sitehelper_editor_create_plan_note_action(const SiteHelperEditor *editor,
    PlanPosition position, DomainId target_id, const char *text, EditorAction *action);
int sitehelper_editor_create_edit_plan_note_action(const SiteHelperEditor *editor,
    PlanPosition position, DomainId target_id, const char *text, EditorAction *action);
/* Resolve a Note-tool click without mutating Project authority. Existing notes
 * are selected and return their exact anchor; empty space returns a snapped
 * Plan position for a new note and clears any stale selection. */
int sitehelper_editor_prepare_plan_note_authoring(SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 view_position,
    DomainId *annotation_id, PlanPosition *position);
/* Revision-cloud authoring is an explicit closed Plan boundary. Clicks append
 * snapped integer-mm vertices; Enter produces the create command and Esc cancels.
 * Presentation scallops remain derived in the application renderer. */
int sitehelper_editor_create_plan_revision_cloud_action(
    const SiteHelperEditor *editor, EditorAction *action);
int sitehelper_editor_get_plan_revision_cloud_preview(
    const SiteHelperEditor *editor, const PlanPosition **vertices, size_t *count,
    PlanPoint *preview, int *has_preview);
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
