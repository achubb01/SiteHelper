#ifndef PRESENTATION_INTERACTION_STYLE_H
#define PRESENTATION_INTERACTION_STYLE_H

#include "renderer2d.h"

/* Interaction presentation is deliberately separate from layer emphasis.
 * These colours communicate editor meaning, not model-layer prominence. */
typedef struct
{
    Colour selected_colour;
    Colour hovered_colour;
    Colour selection_owner_colour;
    Colour navigation_colour;
} AppInteractionStyle;

AppInteractionStyle app_interaction_style_default(void);

#endif
