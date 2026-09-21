#include <assert.h>
#include <stdio.h>

#include "presentation_style.h"

static void assert_colour(Colour colour, unsigned char r, unsigned char g,
    unsigned char b, unsigned char a)
{
    assert(colour.r == r);
    assert(colour.g == g);
    assert(colour.b == b);
    assert(colour.a == a);
}

int main(void)
{
    AppPresentationStyle style = app_presentation_style_default();
    assert(style.context.colour_scale_percent == 45u);
    assert(style.context.white_blend_percent == 0u);
    assert(style.normal.colour_scale_percent == 100u);
    assert(style.normal.white_blend_percent == 0u);
    assert(style.primary.colour_scale_percent == 100u);
    assert(style.primary.white_blend_percent == 12u);

    Colour base = {100, 120, 140, 200};
    assert_colour(app_render_tone_apply(base, &style.context), 45, 54, 63, 200);
    assert_colour(app_render_tone_apply(base, &style.normal), 100, 120, 140, 200);
    assert_colour(app_render_tone_apply(base, &style.primary), 118, 136, 153, 200);
    assert_colour(app_render_tone_apply(base, NULL), 100, 120, 140, 200);

    AppRenderTone clamped = {
        .colour_scale_percent = 150u,
        .white_blend_percent = 150u
    };
    assert_colour(app_render_tone_apply(base, &clamped), 255, 255, 255, 200);

    puts("All presentation style tests passed.");
    return 0;
}
