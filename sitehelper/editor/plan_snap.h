#ifndef PLAN_SNAP_H
#define PLAN_SNAP_H

#include "snap.h"
#include "storey.h"

enum { PLAN_SNAP_CANDIDATE_CAPACITY = 3 };

/* Read-only, transient Plan-mm candidates from this Storey's physical Walls.
 * Output must have PLAN_SNAP_CANDIDATE_CAPACITY slots. Emits at most the nearest
 * enabled endpoint, finite centreline projection and exact-derived junction.
 * Equal squared distances within one type choose lexicographically smallest
 * (x,y), independently of Wall storage order. The generic editor_snap still
 * owns tolerance, cross-type priority and grid fallback.
 * Missing/failed optional topology only omits intersections for this call.
 * No framing, RoomSeparator, Room, opening or guide candidates. */
size_t plan_collect_snap_candidates(const Storey *storey, Vec2 position,
    const SnapSettings *settings,
    SnapCandidate candidates[PLAN_SNAP_CANDIDATE_CAPACITY]);

#endif
