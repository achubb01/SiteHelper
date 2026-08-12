#ifndef WALL_SELECTION_H
#define WALL_SELECTION_H

#include <stdbool.h>

#include "build_structure.h"
#include "timber.h"

typedef struct {
    WallMemberKind kind;
    Timber timber;
} WallSelection;

void wall_selection_init(
    WallSelection *selection
);

void wall_selection_clear(
    WallSelection *selection
);

void wall_selection_set(
    WallSelection *selection,
    WallMemberKind kind,
    const Timber *timber
);

bool wall_selection_is_empty(
    const WallSelection *selection
);

const Timber *wall_selection_resolve(
    const WallSelection *selection,
    const Wall *wall
);

void wall_selection_reconcile(
    WallSelection *selection,
    const Wall *wall
);

#endif