#include <limits.h>
#include <math.h>

#include "wall_plan_segment.h"

int wall_plan_segment_length_mm(WallPlanSegment segment)
{
    double dx = (double)segment.end.x - (double)segment.start.x;
    double dy = (double)segment.end.y - (double)segment.start.y;
    double length = round(hypot(dx, dy));

    if (!isfinite(length) || length < 1.0 || length > (double)INT_MAX) {
        return 0;
    }
    return (int)length;
}
