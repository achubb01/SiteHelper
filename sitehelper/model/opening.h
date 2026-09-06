#ifndef OPENING_H
#define OPENING_H

#include <stdbool.h>
#include "domain_id.h"

typedef enum
{
    OPENING_DOOR,
    OPENING_WINDOW
} OpeningType;

typedef struct
{
    DomainId id;

    OpeningType type;

    int frame_position; /* Wall-local U, in millimetres. */
    int frame_bottom;   /* Wall-local Z, in millimetres. */

    int width;  /* Extent along U, in millimetres. */
    int height; /* Extent along Z, in millimetres. */

    int width_allowance;
    int height_allowance;

    bool custom_allowance;
} Opening;

#endif
