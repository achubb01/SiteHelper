#ifndef GUI_LAYOUT_H
#define GUI_LAYOUT_H

#include "geometry.h"

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