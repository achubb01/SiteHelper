#ifndef VIEWPORT_INPUT_H
#define VIEWPORT_INPUT_H

#include "gui_layout.h"

typedef struct
{
    int middle_drag_owned;
} ViewportInput;

void viewport_input_init(
    ViewportInput *input
);

int viewport_input_contains_point(
    const GuiLayout *layout,
    Vec2 position
);

int viewport_input_allows_wheel(
    const GuiLayout *layout,
    Vec2 position
);

void viewport_input_begin_middle_drag(
    ViewportInput *input,
    const GuiLayout *layout,
    Vec2 position
);

void viewport_input_end_middle_drag(
    ViewportInput *input
);

int viewport_input_allows_pan(
    const ViewportInput *input
);

#endif
