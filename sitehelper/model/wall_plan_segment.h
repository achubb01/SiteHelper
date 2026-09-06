#ifndef WALL_PLAN_SEGMENT_H
#define WALL_PLAN_SEGMENT_H

#include "position.h"

typedef struct WallPlanSegment {
    PlanPosition start; /* Physical point at wall-local U = 0. */
    PlanPosition end;   /* Positive U points from start toward end. */
} WallPlanSegment;

/* Nearest whole millimetre; 0 means zero length or outside the int range. */
int wall_plan_segment_length_mm(WallPlanSegment segment);

#endif
