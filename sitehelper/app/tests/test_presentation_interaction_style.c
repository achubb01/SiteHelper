#include <assert.h>
#include <stdio.h>

#include "presentation_interaction_style.h"

static void assert_colour(Colour actual, unsigned char r, unsigned char g,
    unsigned char b, unsigned char a)
{
    assert(actual.r == r);
    assert(actual.g == g);
    assert(actual.b == b);
    assert(actual.a == a);
}

int main(void)
{
    AppInteractionStyle style = app_interaction_style_default();

    assert_colour(style.selected_colour, 255, 220, 40, 255);
    assert_colour(style.hovered_colour, 120, 210, 255, 255);
    assert_colour(style.selection_owner_colour, 150, 150, 105, 255);
    assert_colour(style.navigation_colour, 225, 170, 80, 255);

    assert(style.selected_colour.r != style.navigation_colour.r ||
        style.selected_colour.g != style.navigation_colour.g ||
        style.selected_colour.b != style.navigation_colour.b);

    printf("All presentation interaction style tests passed.\n");
    return 0;
}
