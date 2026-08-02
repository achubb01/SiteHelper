#ifndef WALL_RENDER_H
#define WALL_RENDER_H

#include "renderer2d.h"
#include "wall.h"
#include "wall_selection.h"

typedef struct {
    Colour timber_colour;
    Colour selected_colour;
} WallRenderStyle;

void wall_render(
    Renderer2D *renderer,
    const Wall *wall,
    const WallSelection *selection,
    const WallRenderStyle *style
);

#endif