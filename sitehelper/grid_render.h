#ifndef GRID_RENDER_H
#define GRID_RENDER_H

#include "renderer2d.h"

typedef struct
{
    Colour minor_colour;
    Colour major_colour;
    Colour axis_colour;

    double minimum_screen_spacing;
} GridRenderStyle;

void grid_render(
    Renderer2D *renderer,
    const GridRenderStyle *style
);

#endif