#ifndef APP_INPUT_HUD_H
#define APP_INPUT_HUD_H

#include "app_input.h"
#include "renderer2d.h"

/* Temporary ASCII HUD: UTF-8 code points outside ASCII display as '?'.
 * Rendering/typography can change independently of editing and intent APIs. */
void app_input_draw_hud(Renderer2D *renderer, const AppInput *input, Rect2 viewport);

#endif
