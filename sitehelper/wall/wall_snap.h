#ifndef WALL_SNAP_H
#define WALL_SNAP_H

#include <stddef.h>

#include "snap.h"
#include "wall.h"

/* Local elevation geometry: Vec2.x = U, Vec2.y = Z, in millimetres.
 * Face centres may be fractional. No plan origin or layout offset is applied. */
size_t wall_collect_snap_candidates(
    const Wall *wall,
    SnapCandidate *candidates,
    size_t capacity
);

#endif
