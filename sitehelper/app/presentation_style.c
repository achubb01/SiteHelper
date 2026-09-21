#include "presentation_style.h"

#include <stddef.h>

static unsigned int clamp_percent(unsigned int percent)
{
    return percent > 100u ? 100u : percent;
}

AppPresentationStyle app_presentation_style_default(void)
{
    return (AppPresentationStyle){
        .context = {
            .colour_scale_percent = 45u,
            .white_blend_percent = 0u
        },
        .normal = {
            .colour_scale_percent = 100u,
            .white_blend_percent = 0u
        },
        .primary = {
            .colour_scale_percent = 100u,
            .white_blend_percent = 12u
        }
    };
}

Colour app_render_tone_apply(Colour colour, const AppRenderTone *tone)
{
    unsigned int scale = tone != NULL
        ? clamp_percent(tone->colour_scale_percent)
        : 100u;
    unsigned int blend = tone != NULL
        ? clamp_percent(tone->white_blend_percent)
        : 0u;

    unsigned int r = (unsigned int)colour.r * scale / 100u;
    unsigned int g = (unsigned int)colour.g * scale / 100u;
    unsigned int b = (unsigned int)colour.b * scale / 100u;

    r += (255u - r) * blend / 100u;
    g += (255u - g) * blend / 100u;
    b += (255u - b) * blend / 100u;

    colour.r = (unsigned char)r;
    colour.g = (unsigned char)g;
    colour.b = (unsigned char)b;
    return colour;
}
