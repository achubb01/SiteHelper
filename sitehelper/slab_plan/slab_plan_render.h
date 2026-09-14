#ifndef SLAB_PLAN_RENDER_H
#define SLAB_PLAN_RENDER_H

#include "renderer2d.h"
#include "slab_plan_query.h"

typedef struct {
    Colour outline_colour;
    Colour penetration_colour;
    Colour region_colour;
    Colour rebate_colour;
    Colour selected_colour;
    Colour selected_parent_colour;
    double marker_size_pixels;
} SlabPlanRenderStyle;

/* CAD-style plan primitives only. Concave polygons are outlined without fill;
 * rebates are their authoritative edge intervals, not inferred footprints. */
void slab_plan_render(Renderer2D *renderer, const Slab *slab,
    const SlabPlanHit *selection, const SlabPlanRenderStyle *style);
void slab_plan_render_storey(Renderer2D *renderer, const Storey *storey,
    const SlabPlanHit *selection, const SlabPlanRenderStyle *style);

#endif
