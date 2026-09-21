#include "presentation_interaction_style.h"

AppInteractionStyle app_interaction_style_default(void)
{
    return (AppInteractionStyle){
        .selected_colour = {255, 220, 40, 255},
        .selection_owner_colour = {150, 150, 105, 255},
        .navigation_colour = {225, 170, 80, 255}
    };
}
