#ifndef PRESENTATION_OVERLAY_RENDER_H
#define PRESENTATION_OVERLAY_RENDER_H

#include "sitehelper_editor.h"
#include "renderer2d.h"
#include "presentation_interaction_style.h"

/* Transient authoring/edit overlays. These adapters own overlay geometry and
 * base colours only. Presentation composition owns whether/when they render. */
void app_render_framing_overlay(
    Renderer2D *renderer,
    const SiteHelperProject *project,
    const SiteHelperEditor *editor,
    const AppInteractionStyle *interaction_style
);

void app_render_slab_overlay(
    Renderer2D *renderer,
    const SiteHelperProject *project,
    const SiteHelperEditor *editor
);

void app_render_snap_overlay(
    Renderer2D *renderer,
    const SiteHelperEditor *editor
);

#endif
