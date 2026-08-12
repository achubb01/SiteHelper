#ifndef WALL_QUERY_H
#define WALL_QUERY_H

#include "sitehelper_model.h"

typedef struct {
    WallMemberKind kind;
    const Timber *timber;
} WallMemberHit;

WallMemberHit wall_find_member_at_position(
    const Wall *wall,
    Position position
);

#endif