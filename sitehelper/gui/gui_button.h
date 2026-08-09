#ifndef GUI_BUTTON_H
#define GUI_BUTTON_H

#include "geometry.h"

typedef enum
{
    GUI_BUTTON_IDLE,
    GUI_BUTTON_HOVERED,
    GUI_BUTTON_PRESSED
} GuiButtonState;

typedef struct
{
    Rect2 bounds;

    GuiButtonState state;

    int active;
    int enabled;
} GuiButton;

void gui_button_init(
    GuiButton *button,
    Rect2 bounds
);

int gui_button_contains_point(
    const GuiButton *button,
    Vec2 point
);

void gui_button_update_hover(
    GuiButton *button,
    Vec2 mouse_position
);

void gui_button_press(
    GuiButton *button,
    Vec2 mouse_position
);

int gui_button_release(
    GuiButton *button,
    Vec2 mouse_position
);

void gui_button_set_active(
    GuiButton *button,
    int active
);

void gui_button_set_enabled(
    GuiButton *button,
    int enabled
);

#endif