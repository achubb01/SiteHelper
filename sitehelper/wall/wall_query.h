#ifndef WALL_QUERY_H
#define WALL_QUERY_H

#include "sitehelper_model.h"

typedef struct {
    WallMemberKind kind;
    const Timber *timber;
} WallMemberHit;

/* Clear framed U/Z rectangle, including effective allowances. Returns only
 * stable identity. Collection order breaks ties; no framing is required. */
DomainId wall_find_opening_at_position(const Wall *wall,
    const BuildSettings *settings, WallLocalPosition position);

WallMemberHit wall_find_member_at_position(
    const Wall *wall,
    WallLocalPosition position
);

#endif
