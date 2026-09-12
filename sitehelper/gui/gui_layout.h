#ifndef GUI_LAYOUT_H
#define GUI_LAYOUT_H

#include "geometry.h"

/* Layout bounds, window dimensions and GUI hit positions are screen pixels.
 * They must not be passed to construction APIs without camera unprojection. */
typedef struct
{
    Rect2 toolbar;
    Rect2 viewport;
    Rect2 properties;
} GuiLayout;

GuiLayout gui_layout_create(
    double window_width,
    double window_height
);

#endif