#ifndef WALL_RENDER_H
#define WALL_RENDER_H

#include "renderer2d.h"
#include "wall.h"
#include "wall_selection.h"

typedef struct {
    Colour timber_colour;
    Colour selected_colour;
} WallRenderStyle;

typedef struct {
    Colour body_colour;
    Colour datum_colour;
    bool show_datum;
} WallPlanRenderStyle;

/* Derived physical Plan body; datum is shown only as an editor affordance. */
void wall_plan_render(Renderer2D *renderer, const Wall *wall,
    const WallPlanRenderStyle *style);

/* Generated framing in local U/Z; placement is exclusively the camera's job. */
void wall_elevation_render(
    Renderer2D *renderer,
    const Wall *wall,
    const Timber *selected,
    const WallRenderStyle *style
);

#endif
