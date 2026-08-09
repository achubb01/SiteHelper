#ifndef GUI_RENDER_H
#define GUI_RENDER_H

#include "gui_layout.h"
#include "gui_button.h"
#include "gui_toolbar.h"
#include "renderer2d.h"

typedef struct
{
    Colour toolbar_colour;
    Colour properties_colour;

    Colour button_idle_colour;
    Colour button_hover_colour;
    Colour button_pressed_colour;
    Colour button_active_colour;
    Colour button_disabled_colour;
} GuiRenderStyle;

void gui_render(
    Renderer2D *renderer,
    const GuiLayout *layout,
    const GuiRenderStyle *style
);

void gui_render_button(
    Renderer2D *renderer,
    const GuiButton *button,
    const GuiRenderStyle *style
);

void gui_render_toolbar(
    Renderer2D *renderer,
    const GuiToolbar *toolbar,
    const GuiRenderStyle *style
);

#endif