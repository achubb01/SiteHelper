#include "presentation_composition.h"
#include "presentation_overlay_render.h"

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

static AppRenderTone app_presentation_tone(
    const AppPresentationStyle *style,
    AppPresentationEmphasis emphasis
)
{
    AppPresentationStyle fallback;
    if (style == NULL) {
        fallback = app_presentation_style_default();
        style = &fallback;
    }

    switch (emphasis) {
        case APP_PRESENTATION_CONTEXT: return style->context;
        case APP_PRESENTATION_PRIMARY: return style->primary;
        case APP_PRESENTATION_NORMAL: return style->normal;
        case APP_PRESENTATION_HIDDEN:
        default: return (AppRenderTone){0};
    }
}

static GridRenderStyle app_presentation_grid_style(
    const GridRenderStyle *style,
    const AppRenderTone *tone
)
{
    GridRenderStyle result = *style;
    result.minor_colour = app_render_tone_apply(result.minor_colour, tone);
    result.major_colour = app_render_tone_apply(result.major_colour, tone);
    result.axis_colour = app_render_tone_apply(result.axis_colour, tone);
    return result;
}

static AppInteractionStyle app_presentation_interaction_style(
    const AppInteractionStyle *style
)
{
    AppInteractionStyle result = style != NULL ? *style : app_interaction_style_default();
    if (result.hovered_colour.a == 0) {
        result.hovered_colour = app_interaction_style_default().hovered_colour;
    }
    return result;
}

static WallRenderStyle app_presentation_wall_style(
    const WallRenderStyle *style,
    const AppRenderTone *tone
)
{
    WallRenderStyle result = *style;
    result.timber_colour = app_render_tone_apply(result.timber_colour, tone);
    result.bottom_plate_colour = app_render_tone_apply(
        result.bottom_plate_colour, tone);
    result.top_plate_colour = app_render_tone_apply(
        result.top_plate_colour, tone);
    result.common_stud_colour = app_render_tone_apply(
        result.common_stud_colour, tone);
    result.king_stud_colour = app_render_tone_apply(
        result.king_stud_colour, tone);
    result.trimmer_stud_colour = app_render_tone_apply(
        result.trimmer_stud_colour, tone);
    result.cripple_stud_colour = app_render_tone_apply(
        result.cripple_stud_colour, tone);
    result.noggin_colour = app_render_tone_apply(
        result.noggin_colour, tone);
    result.header_colour = app_render_tone_apply(
        result.header_colour, tone);
    result.sill_colour = app_render_tone_apply(result.sill_colour, tone);
    result.wall_extent_colour = app_render_tone_apply(
        result.wall_extent_colour, tone);
    result.opening_colour = app_render_tone_apply(result.opening_colour, tone);
    result.annotation_colour = app_render_tone_apply(
        result.annotation_colour, tone);
    result.dimension_colour = app_render_tone_apply(
        result.dimension_colour, tone);
    result.debug_axis_colour = app_render_tone_apply(
        result.debug_axis_colour, tone);
    return result;
}

static SlabPlanRenderStyle app_presentation_slab_style(
    const SlabPlanRenderStyle *style,
    const AppRenderTone *tone,
    const AppInteractionStyle *interaction_style
)
{
    SlabPlanRenderStyle result = *style;
    result.outline_colour = app_render_tone_apply(result.outline_colour, tone);
    result.penetration_colour = app_render_tone_apply(result.penetration_colour, tone);
    result.region_colour = app_render_tone_apply(result.region_colour, tone);
    result.rebate_colour = app_render_tone_apply(result.rebate_colour, tone);
    result.selected_colour = interaction_style->selected_colour;
    result.selected_parent_colour = interaction_style->selection_owner_colour;
    return result;
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

    AppInteractionStyle interaction = app_presentation_interaction_style(
        context->interaction_style);

    renderer2d_begin_viewport_clip(context->renderer);

    if (app_presentation_emphasis_visible(policy->grid) &&
        context->grid_style != NULL) {
        AppRenderTone tone = app_presentation_tone(
            context->presentation_style, policy->grid);
        GridRenderStyle style = app_presentation_grid_style(
            context->grid_style, &tone);
        grid_render(context->renderer, &style);
    }

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->slabs) &&
        context->slab_style != NULL) {
        AppRenderTone tone = app_presentation_tone(
            context->presentation_style, policy->slabs);
        SlabPlanRenderStyle style = app_presentation_slab_style(
            context->slab_style, &tone, &interaction);
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
        AppRenderTone tone = app_presentation_tone(
            context->presentation_style, policy->walls);
        WallRenderStyle style = app_presentation_wall_style(
            context->wall_style, &tone);
        AppInteractionStyle wall_interaction = interaction;
        if (policy->walls == APP_PRESENTATION_CONTEXT) {
            /* Priority 32 preserves current_wall_id as navigation across
             * workspace changes. Context geometry must not promote that
             * remembered navigation target into active-looking feedback. */
            wall_interaction.navigation_colour = style.timber_colour;
        }
        app_render_walls(
            context->renderer,
            context->project,
            context->editor,
            &style,
            &wall_interaction
        );
    }

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->roofs)) {
        AppRenderTone tone = app_presentation_tone(
            context->presentation_style, policy->roofs);
        app_render_roofs(
            context->renderer,
            context->project,
            context->editor,
            &tone,
            &interaction
        );
    }

    /* Roof source-edit handles/derived presentation do not exist yet. The slot
     * is reserved here so they extend this composer rather than the app loop. */
    (void)policy->roof_overlays;

    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->dimensions)) {
        AppRenderTone tone = app_presentation_tone(
            context->presentation_style, policy->dimensions);
        app_render_plan_dimensions(
            context->renderer,
            context->project,
            context->editor,
            &tone,
            &interaction
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->symbols)) {
        AppRenderTone style = app_presentation_tone(
            context->presentation_style, policy->symbols);
        app_render_plan_symbols(
            context->renderer,
            context->project,
            context->editor,
            &style,
            &interaction
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->callouts)) {
        AppRenderTone style = app_presentation_tone(
            context->presentation_style, policy->callouts);
        app_render_plan_callouts(
            context->renderer,
            context->project,
            context->editor,
            &style,
            &interaction
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->notes)) {
        AppRenderTone style = app_presentation_tone(
            context->presentation_style, policy->notes);
        app_render_plan_notes(
            context->renderer,
            context->project,
            context->editor,
            &style,
            &interaction
        );
    }
    if (context->project != NULL &&
        app_presentation_emphasis_visible(policy->revision_clouds)) {
        AppRenderTone style = app_presentation_tone(
            context->presentation_style, policy->revision_clouds);
        app_render_plan_revision_clouds(
            context->renderer,
            context->project,
            context->editor,
            &style,
            &interaction
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
        app_render_framing_overlay(context->renderer, context->project,
            context->editor, &interaction);
    }

    if (app_presentation_emphasis_visible(policy->measurement_overlay)) {
        app_render_measurement(context->renderer, context->editor);
    }

    if (app_presentation_emphasis_visible(policy->slab_overlays)) {
        app_render_slab_overlay(
            context->renderer,
            context->project,
            context->editor
        );
    }

    if (app_presentation_emphasis_visible(policy->snap_overlay)) {
        app_render_snap_overlay(context->renderer, context->editor);
    }

    renderer2d_end_viewport_clip(context->renderer);
}
