#ifndef GUI_TOOLBAR_H
#define GUI_TOOLBAR_H

#include <stddef.h>

#include "gui_button.h"

typedef struct
{
    GuiButton *buttons;
    size_t button_count;

    Rect2 bounds;

    double padding;
    double button_size;
    double spacing;
} GuiToolbar;

void gui_toolbar_init(
    GuiToolbar *toolbar,
    GuiButton *buttons,
    size_t button_count,
    Rect2 bounds
);

void gui_toolbar_layout(
    GuiToolbar *toolbar
);

GuiButton *gui_toolbar_button(
    GuiToolbar *toolbar,
    size_t index
);

const GuiButton *gui_toolbar_button_const(
    const GuiToolbar *toolbar,
    size_t index
);

void gui_toolbar_mouse_move(
    GuiToolbar *toolbar,
    Vec2 mouse_position
);

void gui_toolbar_mouse_press(
    GuiToolbar *toolbar,
    Vec2 mouse_position
);

int gui_toolbar_mouse_release(
    GuiToolbar *toolbar,
    Vec2 mouse_position
);

#endif