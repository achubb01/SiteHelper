#ifndef GUI_RENDER_H
#define GUI_RENDER_H

#include "gui_layout.h"
#include "renderer2d.h"

typedef struct
{
    Colour toolbar_colour;
    Colour properties_colour;
} GuiRenderStyle;

void gui_render(
    Renderer2D *renderer,
    const GuiLayout *layout,
    const GuiRenderStyle *style
);

#endif