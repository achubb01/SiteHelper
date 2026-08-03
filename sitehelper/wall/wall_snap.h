#ifndef WALL_SNAP_H
#define WALL_SNAP_H

#include <stddef.h>

#include "snap.h"
#include "wall.h"

size_t wall_collect_snap_candidates(
    const Wall *wall,
    SnapCandidate *candidates,
    size_t capacity
);

#endif