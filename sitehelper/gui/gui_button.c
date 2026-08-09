#include <stdlib.h>

#include "gui_button.h"

void gui_button_init(
    GuiButton *button,
    Rect2 bounds
)
{
    if (button == NULL) {
        return;
    }

    *button = (GuiButton){
        .bounds = bounds,
        .state = GUI_BUTTON_IDLE,
        .active = 0,
        .enabled = 1
    };
}

int gui_button_contains_point(
    const GuiButton *button,
    Vec2 point
)
{
    if (
        button == NULL
        || !button->enabled
    ) {
        return 0;
    }

    return rect2_contains_point(
        button->bounds,
        point
    );
}

void gui_button_update_hover(
    GuiButton *button,
    Vec2 mouse_position
)
{
    if (
        button == NULL
        || !button->enabled
    ) {
        return;
    }

    if (button->state == GUI_BUTTON_PRESSED) {
        return;
    }

    if (gui_button_contains_point(
            button,
            mouse_position)) {
        button->state =
            GUI_BUTTON_HOVERED;
    }
    else {
        button->state =
            GUI_BUTTON_IDLE;
    }
}

void gui_button_press(
    GuiButton *button,
    Vec2 mouse_position
)
{
    if (
        button == NULL
        || !button->enabled
    ) {
        return;
    }

    if (gui_button_contains_point(
            button,
            mouse_position)) {
        button->state =
            GUI_BUTTON_PRESSED;
    }
}

int gui_button_release(
    GuiButton *button,
    Vec2 mouse_position
)
{
    if (
        button == NULL
        || !button->enabled
    ) {
        return 0;
    }

    int clicked =
        button->state == GUI_BUTTON_PRESSED
        && gui_button_contains_point(
            button,
            mouse_position
        );

    button->state =
        gui_button_contains_point(
            button,
            mouse_position
        )
        ? GUI_BUTTON_HOVERED
        : GUI_BUTTON_IDLE;

    return clicked;
}

void gui_button_set_active(
    GuiButton *button,
    int active
)
{
    if (button == NULL) {
        return;
    }

    button->active =
        active ? 1 : 0;
}

void gui_button_set_enabled(
    GuiButton *button,
    int enabled
)
{
    if (button == NULL) {
        return;
    }

    button->enabled =
        enabled ? 1 : 0;

    if (!button->enabled) {
        button->state =
            GUI_BUTTON_IDLE;
    }
}