#include <assert.h>
#include <stdio.h>

#include "gui_button.h"

static GuiButton create_test_button(void)
{
    GuiButton button;

    gui_button_init(
        &button,
        (Rect2){
            .position = {10.0, 20.0},
            .width = 100.0,
            .height = 40.0
        }
    );

    return button;
}

static void test_init_sets_default_state(void)
{
    GuiButton button =
        create_test_button();

    assert(
        button.state
        == GUI_BUTTON_IDLE
    );

    assert(button.active == 0);
    assert(button.enabled == 1);
}

static void test_contains_point_inside(void)
{
    GuiButton button =
        create_test_button();

    assert(
        gui_button_contains_point(
            &button,
            (Vec2){50.0, 40.0}
        )
    );
}

static void test_contains_point_outside(void)
{
    GuiButton button =
        create_test_button();

    assert(
        !gui_button_contains_point(
            &button,
            (Vec2){200.0, 40.0}
        )
    );
}

static void test_hover_inside(void)
{
    GuiButton button =
        create_test_button();

    gui_button_update_hover(
        &button,
        (Vec2){50.0, 40.0}
    );

    assert(
        button.state
        == GUI_BUTTON_HOVERED
    );
}

static void test_hover_outside(void)
{
    GuiButton button =
        create_test_button();

    gui_button_update_hover(
        &button,
        (Vec2){200.0, 40.0}
    );

    assert(
        button.state
        == GUI_BUTTON_IDLE
    );
}

static void test_press_inside_sets_pressed(void)
{
    GuiButton button =
        create_test_button();

    gui_button_press(
        &button,
        (Vec2){50.0, 40.0}
    );

    assert(
        button.state
        == GUI_BUTTON_PRESSED
    );
}

static void test_press_outside_does_not_press(void)
{
    GuiButton button =
        create_test_button();

    gui_button_press(
        &button,
        (Vec2){200.0, 40.0}
    );

    assert(
        button.state
        != GUI_BUTTON_PRESSED
    );
}

static void test_release_inside_after_press_returns_click(void)
{
    GuiButton button =
        create_test_button();

    gui_button_press(
        &button,
        (Vec2){50.0, 40.0}
    );

    int clicked =
        gui_button_release(
            &button,
            (Vec2){50.0, 40.0}
        );

    assert(clicked);

    assert(
        button.state
        == GUI_BUTTON_HOVERED
    );
}

static void test_release_outside_after_press_does_not_click(void)
{
    GuiButton button =
        create_test_button();

    gui_button_press(
        &button,
        (Vec2){50.0, 40.0}
    );

    int clicked =
        gui_button_release(
            &button,
            (Vec2){200.0, 40.0}
        );

    assert(!clicked);

    assert(
        button.state
        == GUI_BUTTON_IDLE
    );
}

static void test_disabled_button_ignores_input(void)
{
    GuiButton button =
        create_test_button();

    gui_button_set_enabled(
        &button,
        0
    );

    gui_button_press(
        &button,
        (Vec2){50.0, 40.0}
    );

    assert(
        button.state
        == GUI_BUTTON_IDLE
    );

    assert(
        !gui_button_contains_point(
            &button,
            (Vec2){50.0, 40.0}
        )
    );
}

static void test_active_state_can_be_changed(void)
{
    GuiButton button =
        create_test_button();

    gui_button_set_active(
        &button,
        1
    );

    assert(button.active == 1);

    gui_button_set_active(
        &button,
        0
    );

    assert(button.active == 0);
}

int main(void)
{
    test_init_sets_default_state();
    test_contains_point_inside();
    test_contains_point_outside();
    test_hover_inside();
    test_hover_outside();
    test_press_inside_sets_pressed();
    test_press_outside_does_not_press();
    test_release_inside_after_press_returns_click();
    test_release_outside_after_press_does_not_click();
    test_disabled_button_ignores_input();
    test_active_state_can_be_changed();

    printf(
        "All GUI button tests passed.\n"
    );

    return 0;
}