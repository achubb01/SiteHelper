#ifndef PRESENTATION_STYLE_H
#define PRESENTATION_STYLE_H

#include "renderer2d.h"

/* A render tone is deliberately small: domain adapters keep ownership of their
 * base palette, while presentation composition can subdue or lift that palette
 * without knowing domain geometry. */
typedef struct
{
    unsigned int colour_scale_percent;
    unsigned int white_blend_percent;
} AppRenderTone;

typedef struct
{
    AppRenderTone context;
    AppRenderTone normal;
    AppRenderTone primary;
} AppPresentationStyle;

AppPresentationStyle app_presentation_style_default(void);
Colour app_render_tone_apply(Colour colour, const AppRenderTone *tone);

#endif
