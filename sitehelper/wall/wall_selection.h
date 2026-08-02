#ifndef WALL_SELECTION_H
#define WALL_SELECTION_H

#include <stdbool.h>

#include "appcontext.h"

void wall_selection_init(
    WallSelection *selection
);

void wall_selection_clear(
    WallSelection *selection
);

void wall_selection_set(
    WallSelection *selection,
    const Timber *timber
);

const Timber *wall_selection_get(
    const WallSelection *selection
);

bool wall_selection_contains(
    const WallSelection *selection,
    const Timber *timber
);

#endif