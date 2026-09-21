#include "presentation_composition.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

static AppPresentationPolicy app_presentation_policy_hidden(void)
{
    return (AppPresentationPolicy){0};
}

int app_presentation_emphasis_visible(AppPresentationEmphasis emphasis)
{
    return emphasis > APP_PRESENTATION_HIDDEN &&
        emphasis <= APP_PRESENTATION_PRIMARY;
}

static void app_presentation_set_plan_compatibility(AppPresentationPolicy *policy)
{
    policy->grid = APP_PRESENTATION_NORMAL;
    policy->slabs = APP_PRESENTATION_NORMAL;
    policy->walls = APP_PRESENTATION_NORMAL;
    policy->dimensions = APP_PRESENTATION_NORMAL;
    policy->symbols = APP_PRESENTATION_NORMAL;
    policy->callouts = APP_PRESENTATION_NORMAL;
    policy->notes = APP_PRESENTATION_NORMAL;
    policy->revision_clouds = APP_PRESENTATION_NORMAL;
    policy->framing_overlays = APP_PRESENTATION_NORMAL;
    policy->slab_overlays = APP_PRESENTATION_NORMAL;
    policy->dimension_overlay = APP_PRESENTATION_NORMAL;
    policy->documentation_overlays = APP_PRESENTATION_NORMAL;
    policy->measurement_overlay = APP_PRESENTATION_NORMAL;
    policy->snap_overlay = APP_PRESENTATION_NORMAL;
}

AppPresentationPolicy app_presentation_policy_for_workspace(
    EditorWorkspace workspace,
    EditorView view
)
{
    AppPresentationPolicy policy = app_presentation_policy_hidden();

    if (workspace < EDITOR_WORKSPACE_GENERAL ||
        workspace >= EDITOR_WORKSPACE_COUNT ||
        view < EDITOR_VIEW_PLAN || view >= EDITOR_VIEW_COUNT ||
        !editor_workspace_supports_view(workspace, view)) {
        return policy;
    }

    if (workspace == EDITOR_WORKSPACE_GENERAL) {
        if (view == EDITOR_VIEW_PLAN) {
            app_presentation_set_plan_compatibility(&policy);
        } else {
            policy.grid = APP_PRESENTATION_NORMAL;
            policy.walls = APP_PRESENTATION_NORMAL;
            policy.framing_overlays = APP_PRESENTATION_NORMAL;
            policy.snap_overlay = APP_PRESENTATION_NORMAL;
        }
        return policy;
    }

    if (workspace == EDITOR_WORKSPACE_FRAMING) {
        policy.grid = APP_PRESENTATION_NORMAL;
        policy.walls = APP_PRESENTATION_PRIMARY;
        policy.framing_overlays = APP_PRESENTATION_PRIMARY;
        policy.snap_overlay = APP_PRESENTATION_PRIMARY;
        if (view == EDITOR_VIEW_PLAN) {
            policy.slabs = APP_PRESENTATION_CONTEXT;
            policy.dimensions = APP_PRESENTATION_NORMAL;
            policy.symbols = APP_PRESENTATION_CONTEXT;
            policy.callouts = APP_PRESENTATION_CONTEXT;
            policy.notes = APP_PRESENTATION_CONTEXT;
            policy.revision_clouds = APP_PRESENTATION_CONTEXT;
            policy.dimension_overlay = APP_PRESENTATION_NORMAL;
            policy.measurement_overlay = APP_PRESENTATION_NORMAL;
        }
        return policy;
    }

    /* The remaining named workspaces are Plan-only in Priority 32. */
    switch (workspace) {
        case EDITOR_WORKSPACE_SLAB:
            policy.grid = APP_PRESENTATION_NORMAL;
            policy.slabs = APP_PRESENTATION_PRIMARY;
            policy.walls = APP_PRESENTATION_CONTEXT;
            policy.dimensions = APP_PRESENTATION_NORMAL;
            policy.symbols = APP_PRESENTATION_CONTEXT;
            policy.callouts = APP_PRESENTATION_CONTEXT;
            policy.notes = APP_PRESENTATION_CONTEXT;
            policy.revision_clouds = APP_PRESENTATION_CONTEXT;
            policy.slab_overlays = APP_PRESENTATION_PRIMARY;
            policy.dimension_overlay = APP_PRESENTATION_NORMAL;
            policy.measurement_overlay = APP_PRESENTATION_NORMAL;
            policy.snap_overlay = APP_PRESENTATION_PRIMARY;
            break;

        case EDITOR_WORKSPACE_ROOF:
            policy.grid = APP_PRESENTATION_NORMAL;
            policy.slabs = APP_PRESENTATION_CONTEXT;
            policy.walls = APP_PRESENTATION_CONTEXT;
            policy.roofs = APP_PRESENTATION_PRIMARY;
            policy.dimensions = APP_PRESENTATION_NORMAL;
            policy.symbols = APP_PRESENTATION_CONTEXT;
            policy.callouts = APP_PRESENTATION_CONTEXT;
            policy.notes = APP_PRESENTATION_CONTEXT;
            policy.revision_clouds = APP_PRESENTATION_CONTEXT;
            policy.roof_overlays = APP_PRESENTATION_PRIMARY;
            policy.dimension_overlay = APP_PRESENTATION_NORMAL;
            policy.measurement_overlay = APP_PRESENTATION_NORMAL;
            policy.snap_overlay = APP_PRESENTATION_PRIMARY;
            break;

        case EDITOR_WORKSPACE_DOCUMENTATION:
            policy.grid = APP_PRESENTATION_NORMAL;
            policy.slabs = APP_PRESENTATION_CONTEXT;
            policy.walls = APP_PRESENTATION_CONTEXT;
            policy.dimensions = APP_PRESENTATION_PRIMARY;
            policy.symbols = APP_PRESENTATION_PRIMARY;
            policy.callouts = APP_PRESENTATION_PRIMARY;
            policy.notes = APP_PRESENTATION_PRIMARY;
            policy.revision_clouds = APP_PRESENTATION_PRIMARY;
            policy.dimension_overlay = APP_PRESENTATION_PRIMARY;
            policy.documentation_overlays = APP_PRESENTATION_PRIMARY;
            policy.measurement_overlay = APP_PRESENTATION_NORMAL;
            policy.snap_overlay = APP_PRESENTATION_NORMAL;
            break;

        case EDITOR_WORKSPACE_GENERAL:
        case EDITOR_WORKSPACE_FRAMING:
        case EDITOR_WORKSPACE_COUNT:
        default:
            break;
    }

    return policy;
}

static Colour app_presentation_context_colour(Colour colour)
{
    colour.r = (unsigned char)((unsigned int)colour.r * 45U / 100U);
    colour.g = (unsigned char)((unsigned int)colour.g * 45U / 100U);
    colour.b = (unsigned char)((unsigned int)colour.b * 45U / 100U);
    return colour;
}

static GridRenderStyle app_presentation_grid_style(
    const GridRenderStyle *style,
    AppPresentationEmphasis emphasis
)
{
    GridRenderStyle result = *style;
    if (emphasis == APP_PRESENTATION_CONTEXT) {
        result.minor_colour = app_presentation_context_colour(result.minor_colour);
        result.major_colour = app_presentation_context_colour(result.major_colour);
        result.axis_colour = app_presentation_context_colour(result.axis_colour);
    }
    return result;
}

static WallRenderStyle app_presentation_wall_style(
    const WallRenderStyle *style,
    AppPresentationEmphasis emphasis
)
{
    WallRenderStyle result = *style;
    if (emphasis == APP_PRESENTATION_CONTEXT) {
        result.timber_colour = app_presentation_context_colour(result.timber_colour);
        /* current_wall_id is navigation state, not a focused-workspace selection.
         * Context walls therefore suppress the remembered-current highlight. */
        result.selected_colour = result.timber_colour;
    }
    return result;
}

static SlabPlanRenderStyle app_presentation_slab_style(
    const SlabPlanRenderStyle *style,
    AppPresentationEmphasis emphasis
)
{
    SlabPlanRenderStyle result = *style;
    if (emphasis == APP_PRESENTATION_CONTEXT) {
        result.outline_colour = app_presentation_context_colour(result.outline_colour);
        result.penetration_colour = app_presentation_context_colour(result.penetration_colour);
        result.region_colour = app_presentation_context_colour(result.region_colour);
        result.rebate_colour = app_presentation_context_colour(result.rebate_colour);
        result.selected_colour = app_presentation_context_colour(result.selected_colour);
        result.selected_parent_colour = app_presentation_context_colour(result.selected_parent_colour);
    }
    return result;
}

static AppAnnotationRenderStyle app_presentation_annotation_style(
    AppPresentationEmphasis emphasis
)
{
    return (AppAnnotationRenderStyle){
        .colour_scale_percent = emphasis == APP_PRESENTATION_CONTEXT ? 45u : 100u
    };
}

static void app_presentation_draw_plan_marker(
    Renderer2D *renderer,
    PlanPoint point,
    double size_pixels,
    Colour colour
)
{
    if (renderer == NULL || size_pixels <= 0.0) {
        return;
    }

    Camera2D camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 screen = camera_world_to_screen(
        &camera,
        viewport,
        (Vec2){point.x, point.y}
    );

    if (!isfinite(screen.x) || !isfinite(screen.y)) {
        return;
    }

    renderer2d_fill_screen_rect(
        renderer,
        (Rect2){
            .position = {
                screen.x - size_pixels * 0.5,
                screen.y - size_pixels * 0.5
            },
            .width = size_pixels,
            .height = size_pixels
        },
        colour
    );
}

static void app_presentation_render_framing_overlays(
    const AppPresentationRenderContext *context
)
{
    Rect2 preview_rect;
    if (sitehelper_editor_get_opening_preview_rect(context->editor, &preview_rect)) {
        renderer2d_draw_rect(
            context->renderer,
            preview_rect,
            (Colour){100, 180, 255, 255}
        );
    }

    WallPlanSegment preview_segment;
    if (sitehelper_editor_get_wall_preview_segment(context->editor, &preview_segment)) {
        renderer2d_draw_line(
            context->renderer,
            (Vec2){preview_segment.start.x, preview_segment.start.y},
            (Vec2){preview_segment.end.x, preview_segment.end.y},
            (Colour){100, 220, 150, 255}
        );
    }
}

static void app_presentation_render_slab_polygon_preview(
    Renderer2D *renderer,
    const PlanPosition *vertices,
    size_t vertex_count,
    PlanPoint cursor,
    int has_cursor,
    Colour colour,
    Colour closing_colour
)
{
    for (size_t i = 1; i < vertex_count; i++) {
        renderer2d_draw_line(
            renderer,
            (Vec2){vertices[i - 1].x, vertices[i - 1].y},
            (Vec2){vertices[i].x, vertices[i].y},
            colour
        );
    }

    if (!has_cursor || vertex_count == 0) {
        return;
    }

    PlanPosition last = vertices[vertex_count - 1];
    renderer2d_draw_line(
        renderer,
        (Vec2){last.x, last.y},
        (Vec2){cursor.x, cursor.y},
        colour
    );

    if (vertex_count >= 2) {
        renderer2d_draw_line(
            renderer,
            (Vec2){cursor.x, cursor.y},
            (Vec2){vertices[0].x, vertices[0].y},
            closing_colour
        );
    }
}

static void app_presentation_render_slab_overlays(
    const AppPresentationRenderContext *context
)
{
    const PlanPosition *vertices = NULL;
    size_t vertex_count = 0;
    PlanPoint cursor = {0};
    int has_cursor = 0;

    if (sitehelper_editor_get_slab_preview(
            context->editor,
            &vertices,
            &vertex_count,
            &cursor,
            &has_cursor)) {
        app_presentation_render_slab_polygon_preview(
            context->renderer,
            vertices,
            vertex_count,
            cursor,
            has_cursor,
            (Colour){100, 220, 150, 255},
            (Colour){80, 150, 115, 255}
        );
    }

    EditorSlabPolygonPreviewKind feature_kind;
    DomainId feature_slab_id;
    if (sitehelper_editor_get_slab_feature_polygon_preview(
            context->editor,
            &feature_kind,
            &feature_slab_id,
            &vertices,
            &vertex_count,
            &cursor,
            &has_cursor)) {
        (void)feature_slab_id;
        Colour colour = feature_kind == EDITOR_SLAB_POLYGON_PREVIEW_PENETRATION
            ? (Colour){230, 120, 120, 255}
            : (Colour){100, 190, 230, 255};
        app_presentation_render_slab_polygon_preview(
            context->renderer,
            vertices,
            vertex_count,
            cursor,
            has_cursor,
            colour,
            (Colour){
                (unsigned char)(colour.r / 2U),
                (unsigned char)(colour.g / 2U),
                (unsigned char)(colour.b / 2U),
                255
            }
        );
    }

    DomainId rebate_slab_id;
    size_t rebate_edge_index;
    PlanPoint rebate_start;
    PlanPoint rebate_end;
    int rebate_has_end;
    if (sitehelper_editor_get_slab_rebate_preview(
            context->editor,
            &rebate_slab_id,
            &rebate_edge_index,
            &rebate_start,
            &rebate_end,
            &rebate_has_end)) {
        (void)rebate_slab_id;
        (void)rebate_edge_index;
        Colour colour = {235, 165, 85, 255};
        if (rebate_has_end) {
            renderer2d_draw_line(
                context->renderer,
                (Vec2){rebate_start.x, rebate_start.y},
                (Vec2){rebate_end.x, rebate_end.y},
                colour
            );
        }
        renderer2d_draw_rect(
            context->renderer,
            (Rect2){
                .position = {rebate_start.x - 12.0, rebate_start.y - 12.0},
                .width = 24.0,
                .height = 24.0
            },
            colour
        );
    }

    EditorSlabGeometryOverlay geometry;
    if (context->project != NULL && sitehelper_editor_get_slab_geometry_overlay(
            context->editor,
            context->project,
            &geometry)) {
        Colour handle_colour = {255, 205, 80, 255};
        Colour active_colour = {255, 245, 150, 255};

        if (geometry.active_vertex_index != SIZE_MAX && geometry.has_preview) {
            for (size_t i = 0; i < geometry.vertex_count; i++) {
                size_t next = i + 1 == geometry.vertex_count ? 0 : i + 1;
                PlanPoint a = i == geometry.active_vertex_index
                    ? geometry.preview
                    : (PlanPoint){geometry.vertices[i].x, geometry.vertices[i].y};
                PlanPoint b = next == geometry.active_vertex_index
                    ? geometry.preview
                    : (PlanPoint){geometry.vertices[next].x, geometry.vertices[next].y};
                renderer2d_draw_line(
                    context->renderer,
                    (Vec2){a.x, a.y},
                    (Vec2){b.x, b.y},
                    active_colour
                );
            }
        }

        for (size_t i = 0; i < geometry.vertex_count; i++) {
            PlanPoint point = {geometry.vertices[i].x, geometry.vertices[i].y};
            Colour colour = geometry.active_vertex_index == i
                ? active_colour
                : handle_colour;
            app_presentation_draw_plan_marker(
                context->renderer,
                point,
                8.0,
                colour
            );
        }

        if (geometry.active_vertex_index != SIZE_MAX && geometry.has_preview) {
            app_presentation_draw_plan_marker(
                context->renderer,
                geometry.preview,
                10.0,
                active_colour
            );
        }
    }
}

static void app_presentation_render_snap_overlay(
    const AppPresentationRenderContext *context
)
{
    if (!sitehelper_editor_has_snap(context->editor)) {
        return;
    }

    const SnapResult *snap_result = sitehelper_editor_get_snap_result(context->editor);
    if (snap_result == NULL) {
        return;
    }

    Colour marker_colour;
    switch (snap_result->type) {
        case SNAP_ENDPOINT:
            marker_colour = (Colour){255, 180, 60, 255};
            break;
        case SNAP_GRID:
            marker_colour = (Colour){80, 200, 255, 255};
            break;
        case SNAP_WALL_CENTRELINE:
            marker_colour = (Colour){220, 120, 255, 255};
            break;
        case SNAP_INTERSECTION:
            marker_colour = (Colour){80, 255, 120, 255};
            break;
        default:
            return;
    }

    const double marker_radius = 40.0;
    Vec2 position = snap_result->position;

    renderer2d_draw_line(
        context->renderer,
        (Vec2){position.x - marker_radius, position.y},
        (Vec2){position.x + marker_radius, position.y},
        marker_colour
    );
    renderer2d_draw_line(
        context->renderer,
        (Vec2){position.x, position.y - marker_radius},
        (Vec2){position.x, position.y + marker_radius},
        marker_colour
    );
}

void app_presentation_render_viewport(
    const AppPresentationRenderContext *context,
    const AppPresentationPolicy *policy
)
{
    if (context == NULL || policy == NULL || context->renderer == NULL ||
        context->editor == NULL) {
        return;
    }

    renderer2d_begin_viewport_clip(context->renderer);

    if (app_presentation_emphasis_visible(policy->grid) &&
        context->grid_style != NULL) {
        GridRenderStyle style = app_presentation_grid_style(
            context->grid_style,
            policy->grid
        );
        grid_render(context->renderer, &style);
    }

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->slabs) &&
        context->slab_style != NULL) {
        SlabPlanRenderStyle style = app_presentation_slab_style(
            context->slab_style,
            policy->slabs
        );
        app_render_slabs(
            context->renderer,
            context->project,
            context->editor,
            &style
        );
    }

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->walls) &&
        context->wall_style != NULL) {
        WallRenderStyle style = app_presentation_wall_style(
            context->wall_style,
            policy->walls
        );
        app_render_walls(
            context->renderer,
            context->project,
            context->editor,
            &style
        );
    }

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->roofs)) {
        app_render_roofs(
            context->renderer,
            context->project,
            context->editor
        );
    }

    /* Roof source-edit handles/derived presentation do not exist yet. The slot
     * is reserved here so they extend this composer rather than the app loop. */
    (void)policy->roof_overlays;

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->dimensions)) {
        app_render_plan_dimensions(
            context->renderer,
            context->project,
            context->editor
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->symbols)) {
        AppAnnotationRenderStyle style = app_presentation_annotation_style(
            policy->symbols
        );
        app_render_plan_symbols(
            context->renderer,
            context->project,
            context->editor,
            &style
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->callouts)) {
        AppAnnotationRenderStyle style = app_presentation_annotation_style(
            policy->callouts
        );
        app_render_plan_callouts(
            context->renderer,
            context->project,
            context->editor,
            &style
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->notes)) {
        AppAnnotationRenderStyle style = app_presentation_annotation_style(
            policy->notes
        );
        app_render_plan_notes(
            context->renderer,
            context->project,
            context->editor,
            &style
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->revision_clouds)) {
        AppAnnotationRenderStyle style = app_presentation_annotation_style(
            policy->revision_clouds
        );
        app_render_plan_revision_clouds(
            context->renderer,
            context->project,
            context->editor,
            &style
        );
    }

    if (app_presentation_emphasis_visible(policy->dimension_overlay)) {
        app_render_plan_dimension_preview(context->renderer, context->editor);
    }

    if (app_presentation_emphasis_visible(policy->documentation_overlays)) {
        app_render_plan_callout_preview(context->renderer, context->editor);
        app_render_plan_revision_cloud_preview(context->renderer, context->editor);
    }

    if (app_presentation_emphasis_visible(policy->framing_overlays)) {
        app_presentation_render_framing_overlays(context);
    }

    if (app_presentation_emphasis_visible(policy->measurement_overlay)) {
        app_render_measurement(context->renderer, context->editor);
    }

    if (app_presentation_emphasis_visible(policy->slab_overlays)) {
        app_presentation_render_slab_overlays(context);
    }

    if (app_presentation_emphasis_visible(policy->snap_overlay)) {
        app_presentation_render_snap_overlay(context);
    }

    renderer2d_end_viewport_clip(context->renderer);
}
