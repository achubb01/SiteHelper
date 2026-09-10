#ifndef ROOM_SEPARATOR_H
#define ROOM_SEPARATOR_H

#include "domain_id.h"
#include "plan_segment.h"

/* Authoritative non-physical input for future region calculations. No room
 * ownership, construction, openings, or derived boundary relationships. */
typedef struct RoomSeparator {
    DomainId id;
    PlanSegment segment;
} RoomSeparator;

#endif
