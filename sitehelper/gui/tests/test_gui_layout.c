#include <assert.h>
#include <stdio.h>

#include "gui_layout.h"

static void test_creates_expected_layout(void)
{
    GuiLayout layout =
        gui_layout_create(
            1200.0,
            800.0
        );

    assert(layout.toolbar.position.x == 0.0);
    assert(layout.toolbar.width == 64.0);
    assert(layout.toolbar.height == 800.0);

    assert(layout.viewport.position.x == 64.0);
    assert(layout.viewport.width == 876.0);
    assert(layout.viewport.height == 800.0);

    assert(layout.properties.position.x == 940.0);
    assert(layout.properties.width == 260.0);
    assert(layout.properties.height == 800.0);
}

static void test_viewport_changes_with_window_width(void)
{
    GuiLayout layout =
        gui_layout_create(
            1600.0,
            900.0
        );

    assert(layout.toolbar.width == 64.0);
    assert(layout.properties.width == 260.0);

    assert(layout.viewport.width == 1276.0);
    assert(layout.viewport.height == 900.0);
}

static void test_small_window_does_not_create_negative_viewport(void)
{
    GuiLayout layout =
        gui_layout_create(
            200.0,
            600.0
        );

    assert(layout.viewport.width == 0.0);
}

int main(void)
{
    test_creates_expected_layout();
    test_viewport_changes_with_window_width();
    test_small_window_does_not_create_negative_viewport();

    printf(
        "All GUI layout tests passed.\n"
    );

    return 0;
}