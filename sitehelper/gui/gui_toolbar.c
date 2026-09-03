#include "gui_toolbar.h"
#include "gui_toolbar.h"

void gui_toolbar_init(
    GuiToolbar *toolbar,
    GuiButton *buttons,
    const GuiButtonId *button_ids,
    size_t button_count,
    Rect2 bounds
)
{
    if (toolbar == NULL) {
        return;
    }

    *toolbar = (GuiToolbar){
        .buttons = buttons,
        .button_ids = button_ids,
        .button_count = button_count,
        .bounds = bounds,
        .padding = 8.0,
        .button_size = 48.0,
        .spacing = 8.0
    };

    if (
        toolbar->buttons == NULL
        || toolbar->button_ids == NULL
    ) {
        return;
    }

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        gui_button_init(
            &toolbar->buttons[i],
            (Rect2){0}
        );

        toolbar->buttons[i].id =
            toolbar->button_ids[i];
    }

    gui_toolbar_layout(toolbar);
}

void gui_toolbar_layout(
    GuiToolbar *toolbar
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
        || toolbar->button_ids == NULL
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
        toolbar->buttons[i].bounds = (Rect2){
            .position = {
                .x = x,
                .y = y
            },
            .width = toolbar->button_size,
            .height = toolbar->button_size
        };

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

GuiToolbarResult gui_toolbar_mouse_press(
    GuiToolbar *toolbar,
    Vec2 mouse_position
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
    ) {
        return (GuiToolbarResult){
            .action_id = GUI_BUTTON_ID_NONE
        };
    }

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        GuiButton *button = &toolbar->buttons[i];

        if (!rect2_contains_point(
                button->bounds,
                mouse_position)) {
            continue;
        }

        toolbar->primary_press_handled = 1;

        gui_button_press(button, mouse_position);

        return (GuiToolbarResult){
            .handled = 1,
            .action_id = GUI_BUTTON_ID_NONE
        };
    }

    return (GuiToolbarResult){
        .action_id = GUI_BUTTON_ID_NONE
    };
}

GuiToolbarResult gui_toolbar_mouse_release(
    GuiToolbar *toolbar,
    Vec2 mouse_position
)
{
    if (
        toolbar == NULL
        || toolbar->buttons == NULL
    ) {
        return (GuiToolbarResult){
            .action_id = GUI_BUTTON_ID_NONE
        };
    }

    int handled = toolbar->primary_press_handled;

    toolbar->primary_press_handled = 0;

    for (
        size_t i = 0;
        i < toolbar->button_count;
        i++
    ) {
        if (toolbar->buttons[i].state == GUI_BUTTON_PRESSED) {
            handled = 1;
        }

        if (gui_button_release(
                &toolbar->buttons[i],
                mouse_position)) {
            return (GuiToolbarResult){
                .handled = 1,
                .action_id = toolbar->buttons[i].id
            };
        }
    }

    return (GuiToolbarResult){
        .handled = handled,
        .action_id = GUI_BUTTON_ID_NONE
    };
}
