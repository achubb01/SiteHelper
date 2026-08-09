#include <stdlib.h>

#include "gui_render.h"

void gui_render(
    Renderer2D *renderer,
    const GuiLayout *layout,
    const GuiRenderStyle *style
)
{
    if (
        renderer == NULL
        || layout == NULL
        || style == NULL
    ) {
        return;
    }

    renderer2d_fill_screen_rect(
        renderer,
        layout->toolbar,
        style->toolbar_colour
    );

    renderer2d_fill_screen_rect(
        renderer,
        layout->properties,
        style->properties_colour
    );
}

void gui_render_button(
    Renderer2D *renderer,
    const GuiButton *button,
    const GuiRenderStyle *style
)
{
    if (
        renderer == NULL
        || button == NULL
        || style == NULL
    ) {
        return;
    }

    Colour colour;

    if (!button->enabled) {
        colour =
            style->button_disabled_colour;
    }
    else if (button->active) {
        colour =
            style->button_active_colour;
    }
    else {
        switch (button->state) {
            case GUI_BUTTON_HOVERED:
                colour =
                    style->button_hover_colour;
                break;

            case GUI_BUTTON_PRESSED:
                colour =
                    style->button_pressed_colour;
                break;

            case GUI_BUTTON_IDLE:
            default:
                colour =
                    style->button_idle_colour;
                break;
        }
    }

    renderer2d_fill_screen_rect(
        renderer,
        button->bounds,
        colour
    );
}

void gui_render_toolbar(
    Renderer2D *renderer,
    const GuiToolbar *toolbar,
    const GuiRenderStyle *style
)
{
    if (
        renderer == NULL
        || toolbar == NULL
        || style == NULL
    ) {
        return;
    }

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        gui_render_button(
            renderer,
            &toolbar->buttons[i],
            style
        );
    }
}