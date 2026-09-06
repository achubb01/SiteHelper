#include "wall.h"

int wall_length_mm(const Wall *wall)
{
    return wall == NULL ? 0 : wall_plan_segment_length_mm(wall->definition.segment);
}

int wall_set_plan_segment(Wall *wall, WallPlanSegment segment)
{
    if (wall == NULL || wall_plan_segment_length_mm(segment) == 0) {
        return 0;
    }
    wall->definition.segment = segment;
    return 1;
}
