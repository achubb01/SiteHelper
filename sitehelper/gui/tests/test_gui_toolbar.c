#include <assert.h>
#include <stdio.h>

#include "gui_toolbar.h"

static void test_toolbar_lays_out_buttons_vertically(void)
{
    GuiButton buttons[3];

    GuiToolbar toolbar;

    gui_toolbar_init(
        &toolbar,
        buttons,
        3,
        (Rect2){
            .position = {0.0, 0.0},
            .width = 64.0,
            .height = 600.0
        }
    );

    assert(
        buttons[0].bounds.position.x
        == 8.0
    );

    assert(
        buttons[0].bounds.position.y
        == 8.0
    );

    assert(
        buttons[1].bounds.position.y
        == 64.0
    );

    assert(
        buttons[2].bounds.position.y
        == 120.0
    );

    assert(
        buttons[0].bounds.width
        == 48.0
    );

    assert(
        buttons[0].bounds.height
        == 48.0
    );
}

static void test_toolbar_button_returns_requested_button(void)
{
    GuiButton buttons[2];

    GuiToolbar toolbar;

    gui_toolbar_init(
        &toolbar,
        buttons,
        2,
        (Rect2){
            .position = {0.0, 0.0},
            .width = 64.0,
            .height = 600.0
        }
    );

    assert(
        gui_toolbar_button(
            &toolbar,
            0
        ) == &buttons[0]
    );

    assert(
        gui_toolbar_button(
            &toolbar,
            1
        ) == &buttons[1]
    );
}

static void test_toolbar_button_rejects_invalid_index(void)
{
    GuiButton buttons[2];

    GuiToolbar toolbar;

    gui_toolbar_init(
        &toolbar,
        buttons,
        2,
        (Rect2){
            .position = {0.0, 0.0},
            .width = 64.0,
            .height = 600.0
        }
    );

    assert(
        gui_toolbar_button(
            &toolbar,
            2
        ) == NULL
    );
}

static void test_toolbar_respects_nonzero_origin(void)
{
    GuiButton buttons[1];

    GuiToolbar toolbar;

    gui_toolbar_init(
        &toolbar,
        buttons,
        1,
        (Rect2){
            .position = {20.0, 30.0},
            .width = 64.0,
            .height = 600.0
        }
    );

    assert(
        buttons[0].bounds.position.x
        == 28.0
    );

    assert(
        buttons[0].bounds.position.y
        == 38.0
    );
}

int main(void)
{
    test_toolbar_lays_out_buttons_vertically();
    test_toolbar_button_returns_requested_button();
    test_toolbar_button_rejects_invalid_index();
    test_toolbar_respects_nonzero_origin();

    printf(
        "All GUI toolbar tests passed.\n"
    );

    return 0;
}