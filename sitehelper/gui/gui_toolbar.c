#include "gui_toolbar.h"
#include "gui_toolbar.h"

void gui_toolbar_init(
    GuiToolbar *toolbar,
    GuiButton *buttons,
    size_t button_count,
    Rect2 bounds
)
{
    if (toolbar == NULL) {
        return;
    }

    *toolbar = (GuiToolbar){
        .buttons = buttons,
        .button_count = button_count,
        .bounds = bounds,
        .padding = 8.0,
        .button_size = 48.0,
        .spacing = 8.0
    };

    gui_toolbar_layout(toolbar);
}

void gui_toolbar_layout(
    GuiToolbar *toolbar
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
    ) {
        return;
    }

    double x =
        toolbar->bounds.position.x
        + toolbar->padding;

    double y =
        toolbar->bounds.position.y
        + toolbar->padding;

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        gui_button_init(
            &toolbar->buttons[i],
            (Rect2){
                .position = {
                    .x = x,
                    .y = y
                },
                .width = toolbar->button_size,
                .height = toolbar->button_size
            }
        );

        y +=
            toolbar->button_size
            + toolbar->spacing;
    }
}

GuiButton *gui_toolbar_button(
    GuiToolbar *toolbar,
    size_t index
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
        || index >= toolbar->button_count
    ) {
        return NULL;
    }

    return &toolbar->buttons[index];
}

const GuiButton *gui_toolbar_button_const(
    const GuiToolbar *toolbar,
    size_t index
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
        || index >= toolbar->button_count
    ) {
        return NULL;
    }

    return &toolbar->buttons[index];
}

void gui_toolbar_mouse_move(
    GuiToolbar *toolbar,
    Vec2 mouse_position
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
    ) {
        return;
    }

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        gui_button_update_hover(
            &toolbar->buttons[i],
            mouse_position
        );
    }
}

void gui_toolbar_mouse_press(
    GuiToolbar *toolbar,
    Vec2 mouse_position
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
    ) {
        return;
    }

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        gui_button_press(
            &toolbar->buttons[i],
            mouse_position
        );
    }
}

int gui_toolbar_mouse_release(
    GuiToolbar *toolbar,
    Vec2 mouse_position
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
    ) {
        return -1;
    }

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        if (gui_button_release(
                &toolbar->buttons[i],
                mouse_position)) {
            return (int)i;
        }
    }

    return -1;
}