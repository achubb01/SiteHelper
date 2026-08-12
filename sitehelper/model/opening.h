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

    int frame_position;
    int frame_bottom;

    int width;
    int height;

    int width_allowance;
    int height_allowance;

    bool custom_allowance;
} Opening;

#endif