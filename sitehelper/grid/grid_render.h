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

double grid_choose_spacing(
    double camera_scale,
    double minimum_screen_spacing
);

void grid_render(
    Renderer2D *renderer,
    const GridRenderStyle *style
);

#endif