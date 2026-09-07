#ifndef WALL_RENDER_H
#define WALL_RENDER_H

#include "renderer2d.h"
#include "wall.h"
#include "wall_selection.h"

typedef struct {
    Colour timber_colour;
    Colour selected_colour;
} WallRenderStyle;

/* Physical ordered Plan X/Y endpoints. */
void wall_plan_render(Renderer2D *renderer, const Wall *wall, Colour colour);

/* Generated framing in local U/Z; placement is exclusively the camera's job. */
void wall_elevation_render(
    Renderer2D *renderer,
    const Wall *wall,
    const Timber *selected,
    const WallRenderStyle *style
);

#endif
