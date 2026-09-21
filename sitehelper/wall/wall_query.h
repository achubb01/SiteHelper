#ifndef WALL_QUERY_H
#define WALL_QUERY_H

#include "sitehelper_model.h"
#include "wall_plan_geometry.h"

typedef struct {
    WallMemberKind kind;
    const Timber *timber;
} WallMemberHit;

/* Clear framed U/Z rectangle, including effective allowances. Returns only
 * stable identity. Collection order breaks ties; no framing is required. */
DomainId wall_find_opening_at_position(const Wall *wall,
    const BuildSettings *settings, WallLocalPosition position);

/* Physical Plan-body hit query. Later collection entries win equal-distance overlaps. */
DomainId wall_plan_find_wall_at_position(const BuildStructure *structure,
    PlanPoint position, double tolerance_mm);

WallMemberHit wall_find_member_at_position(
    const Wall *wall,
    WallLocalPosition position
);

#endif
