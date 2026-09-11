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

    /* Canonical clear framed rectangle, for both types. U runs along the
     * ordered Wall segment; Z is above the Wall baseline, in millimetres.
     * A window sill's TOP is frame_bottom; a header's underside is clear top.
     * Legacy persisted references are converted at the load boundary. */
    int frame_position; /* Left clear edge U. */
    int frame_bottom;   /* Bottom clear edge Z; baseline doors normally use 0. */

    int width;  /* Positive nominal extent U, before allowance. */
    int height; /* Positive nominal extent Z, before allowance. */

    int width_allowance;
    int height_allowance;

    /* Effective extent = nominal + custom allowance when true, otherwise the
     * resolved BuildSettings allowance. Allowances do not move left/bottom. */
    bool custom_allowance;
} Opening;

#endif
