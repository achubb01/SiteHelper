#include <assert.h>
#include <stdio.h>

#include "viewport_input.h"

static GuiLayout test_layout(void)
{
    return gui_layout_create(1200.0, 800.0);
}

static void test_wheel_is_owned_by_viewport_only(void)
{
    GuiLayout layout = test_layout();

    assert(viewport_input_allows_wheel(
        &layout,
        (Vec2){100.0, 100.0}
    ));

    assert(!viewport_input_allows_wheel(
        &layout,
        (Vec2){20.0, 100.0}
    ));

    assert(!viewport_input_allows_wheel(
        &layout,
        (Vec2){1000.0, 100.0}
    ));
}

static void test_middle_drag_is_owned_by_its_origin(void)
{
    GuiLayout layout = test_layout();
    ViewportInput input;

    viewport_input_init(&input);

    viewport_input_begin_middle_drag(
        &input,
        &layout,
        (Vec2){20.0, 100.0}
    );

    assert(!viewport_input_allows_pan(&input));

    viewport_input_begin_middle_drag(
        &input,
        &layout,
        (Vec2){100.0, 100.0}
    );

    assert(viewport_input_allows_pan(&input));

    viewport_input_end_middle_drag(&input);

    assert(!viewport_input_allows_pan(&input));
}

int main(void)
{
    test_wheel_is_owned_by_viewport_only();
    test_middle_drag_is_owned_by_its_origin();

    printf("All viewport input tests passed.\n");

    return 0;
}
