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