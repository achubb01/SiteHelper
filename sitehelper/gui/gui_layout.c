#include "gui_layout.h"

GuiLayout gui_layout_create(
    double window_width,
    double window_height
)
{
    const double toolbar_width = 64.0;
    const double properties_width = 260.0;

    double viewport_width =
        window_width
        - toolbar_width
        - properties_width;

    if (viewport_width < 0.0) {
        viewport_width = 0.0;
    }

    return (GuiLayout){
        .toolbar = {
            .position = {
                .x = 0.0,
                .y = 0.0
            },
            .width = toolbar_width,
            .height = window_height
        },

        .viewport = {
            .position = {
                .x = toolbar_width,
                .y = 0.0
            },
            .width = viewport_width,
            .height = window_height
        },

        .properties = {
            .position = {
                .x =
                    toolbar_width
                    + viewport_width,

                .y = 0.0
            },
            .width = properties_width,
            .height = window_height
        }
    };
}