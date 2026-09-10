#ifndef PLAN_SEGMENT_H
#define PLAN_SEGMENT_H

#include "position.h"

/* Ordered plan geometry only; direction carries no room-side semantics. */
typedef struct PlanSegment {
    PlanPosition start;
    PlanPosition end;
} PlanSegment;

/* No derived length restriction or subtraction that could overflow. */
static inline int plan_segment_valid(PlanSegment segment)
{
    return segment.start.x != segment.end.x || segment.start.y != segment.end.y;
}

#endif
