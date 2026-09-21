#ifndef PRESENTATION_COMPOSITION_H
#define PRESENTATION_COMPOSITION_H

#include "app_view.h"
#include "grid_render.h"
#include "presentation_interaction_style.h"

/* Presentation policy is transient UI/rendering policy. It consumes the active
 * workspace/view but never owns or mutates editor or Project state. */
typedef enum
{
    APP_PRESENTATION_HIDDEN,
    APP_PRESENTATION_CONTEXT,
    APP_PRESENTATION_NORMAL,
    APP_PRESENTATION_PRIMARY
} AppPresentationEmphasis;

typedef struct
{
    AppPresentationEmphasis grid;

    /* Persistent/project-backed layers. */
    AppPresentationEmphasis slabs;
    AppPresentationEmphasis walls;
    AppPresentationEmphasis roofs;
    AppPresentationEmphasis dimensions;
    AppPresentationEmphasis symbols;
    AppPresentationEmphasis callouts;
    AppPresentationEmphasis notes;
    AppPresentationEmphasis revision_clouds;

    /* Transient editor overlays, grouped by workflow rather than individual
     * tool wherever the visibility rule is shared. Dimension stays separate
     * because it is a cross-workspace authoring tool. */
    AppPresentationEmphasis framing_overlays;
    AppPresentationEmphasis slab_overlays;
    AppPresentationEmphasis roof_overlays;
    AppPresentationEmphasis dimension_overlay;
    AppPresentationEmphasis documentation_overlays;
    AppPresentationEmphasis measurement_overlay;
    AppPresentationEmphasis snap_overlay;
} AppPresentationPolicy;

typedef struct
{
    Renderer2D *renderer;
    const SiteHelperProject *project;
    const SiteHelperEditor *editor;

    const GridRenderStyle *grid_style;
    const WallRenderStyle *wall_style;
    const SlabPlanRenderStyle *slab_style;
    const AppPresentationStyle *presentation_style;
    const AppInteractionStyle *interaction_style;
} AppPresentationRenderContext;

/* Pure workspace/view -> presentation policy mapping. GENERAL is the legacy
 * compatibility surface; named workspaces are the authoritative user intent
 * established by Priority 32. */
AppPresentationPolicy app_presentation_policy_for_workspace(
    EditorWorkspace workspace,
    EditorView view
);

int app_presentation_emphasis_visible(AppPresentationEmphasis emphasis);

/* Owns viewport clipping, layer ordering and transient viewport overlays only.
 * GUI chrome, toolbar, properties and HUD remain application/UI composition. */
void app_presentation_render_viewport(
    const AppPresentationRenderContext *context,
    const AppPresentationPolicy *policy
);

#endif
