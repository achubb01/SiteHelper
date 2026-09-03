#include <stddef.h>

#include "viewport_input.h"

void viewport_input_init(
    ViewportInput *input
)
{
    if (input == NULL) {
        return;
    }

    *input = (ViewportInput){0};
}

int viewport_input_contains_point(
    const GuiLayout *layout,
    Vec2 position
)
{
    return layout != NULL && rect2_contains_point(
        layout->viewport,
        position
    );
}

int viewport_input_allows_wheel(
    const GuiLayout *layout,
    Vec2 position
)
{
    return viewport_input_contains_point(layout, position);
}

void viewport_input_begin_middle_drag(
    ViewportInput *input,
    const GuiLayout *layout,
    Vec2 position
)
{
    if (input == NULL) {
        return;
    }

    input->middle_drag_owned = viewport_input_contains_point(
        layout,
        position
    );
}

void viewport_input_end_middle_drag(
    ViewportInput *input
)
{
    if (input == NULL) {
        return;
    }

    input->middle_drag_owned = 0;
}

int viewport_input_allows_pan(
    const ViewportInput *input
)
{
    return input != NULL && input->middle_drag_owned;
}
