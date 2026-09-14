#include "slab_plan_render.h"

#include <math.h>

static void draw_outline(Renderer2D *renderer, const SlabOutline *outline,
    Colour colour)
{
    for (size_t i = 0; i < outline->vertex_count; i++) {
        size_t next = i + 1 == outline->vertex_count ? 0 : i + 1;
        PlanPosition a = outline->vertices[i], b = outline->vertices[next];
        renderer2d_draw_line(renderer, (Vec2){a.x, a.y},
            (Vec2){b.x, b.y}, colour);
    }
}

static int selection_matches(const SlabPlanHit *selection, DomainId slab_id,
    SlabPlanHitKind kind, size_t index)
{
    return selection != NULL && selection->slab_id == slab_id &&
        selection->kind == kind && selection->feature_index == index;
}

static void draw_marker(Renderer2D *renderer, PlanPoint point, double size,
    Colour colour)
{
    if (size <= 0.0) { return; }
    Camera2D camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 screen = camera_world_to_screen(&camera, viewport, (Vec2){point.x, point.y});
    if (!isfinite(screen.x) || !isfinite(screen.y)) { return; }
    renderer2d_fill_screen_rect(renderer, (Rect2){
        .position = {screen.x - size * 0.5, screen.y - size * 0.5},
        .width = size, .height = size
    }, colour);
}

void slab_plan_render(Renderer2D *renderer, const Slab *slab,
    const SlabPlanHit *selection, const SlabPlanRenderStyle *style)
{
    if (renderer == NULL || slab == NULL || style == NULL ||
        !isfinite(style->marker_size_pixels) || style->marker_size_pixels < 0.0 ||
        slab_validate(slab) != SLAB_SUCCESS) {
        return;
    }
    const SlabDefinition *definition = &slab->definition;
    int same_parent = selection != NULL && selection->slab_id == slab->id;
    Colour outer_colour = style->outline_colour;
    if (selection_matches(selection, slab->id, SLAB_PLAN_HIT_SLAB, SIZE_MAX)) {
        outer_colour = style->selected_colour;
    } else if (same_parent) {
        outer_colour = style->selected_parent_colour;
    }
    draw_outline(renderer, &definition->outline, outer_colour);

    for (size_t i = 0; i < definition->regions.count; i++) {
        Colour colour = selection_matches(selection, slab->id,
            SLAB_PLAN_HIT_REGION, i) ? style->selected_colour : style->region_colour;
        draw_outline(renderer, &definition->regions.items[i].outline, colour);
    }
    for (size_t i = 0; i < definition->penetrations.count; i++) {
        Colour colour = selection_matches(selection, slab->id,
            SLAB_PLAN_HIT_PENETRATION, i) ? style->selected_colour : style->penetration_colour;
        const SlabOutline *outline = &definition->penetrations.items[i].outline;
        draw_outline(renderer, outline, colour);
        /* Boundary markers distinguish voids without polygon Boolean filling. */
        for (size_t v = 0; v < outline->vertex_count; v++) {
            draw_marker(renderer, (PlanPoint){outline->vertices[v].x,
                outline->vertices[v].y}, style->marker_size_pixels * 0.55, colour);
        }
    }
    for (size_t i = 0; i < definition->edge_rebates.count; i++) {
        PlanPoint start, end;
        if (!slab_plan_rebate_endpoints(definition,
                &definition->edge_rebates.items[i], &start, &end)) { continue; }
        Colour colour = selection_matches(selection, slab->id,
            SLAB_PLAN_HIT_EDGE_REBATE, i) ? style->selected_colour : style->rebate_colour;
        renderer2d_draw_line(renderer, (Vec2){start.x, start.y},
            (Vec2){end.x, end.y}, colour);
        draw_marker(renderer, start, style->marker_size_pixels, colour);
        draw_marker(renderer, end, style->marker_size_pixels, colour);
    }
}

void slab_plan_render_storey(Renderer2D *renderer, const Storey *storey,
    const SlabPlanHit *selection, const SlabPlanRenderStyle *style)
{
    if (renderer == NULL || storey == NULL || style == NULL ||
        storey->slabs.count > storey->slabs.capacity ||
        (storey->slabs.capacity == 0 && storey->slabs.items != NULL) ||
        (storey->slabs.capacity != 0 && storey->slabs.items == NULL)) {
        return;
    }
    for (size_t i = 0; i < storey->slabs.count; i++) {
        slab_plan_render(renderer, &storey->slabs.items[i], selection, style);
    }
}
