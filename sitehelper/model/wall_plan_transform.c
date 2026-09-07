#include <math.h>
#include <stddef.h>

#include "wall_plan_transform.h"

int wall_plan_segment_u_to_plan(WallPlanSegment segment, double u, PlanPoint *point)
{
    if (point == NULL || !isfinite(u)) {
        return 0;
    }
    int length = wall_plan_segment_length_mm(segment);
    if (length == 0) {
        return 0;
    }

    double dx = (double)segment.end.x - (double)segment.start.x;
    double dy = (double)segment.end.y - (double)segment.start.y;
    double t = u / (double)length;
    PlanPoint result = {
        .x = (double)segment.start.x + t * dx,
        .y = (double)segment.start.y + t * dy
    };
    if (!isfinite(result.x) || !isfinite(result.y)) {
        return 0;
    }
    *point = result;
    return 1;
}

int wall_plan_segment_plan_to_u(WallPlanSegment segment, PlanPoint point, double *u)
{
    if (u == NULL || !isfinite(point.x) || !isfinite(point.y)) {
        return 0;
    }
    int length = wall_plan_segment_length_mm(segment);
    if (length == 0) {
        return 0;
    }

    double dx = (double)segment.end.x - (double)segment.start.x;
    double dy = (double)segment.end.y - (double)segment.start.y;
    double px = point.x - (double)segment.start.x;
    double py = point.y - (double)segment.start.y;
    double t = (px * dx + py * dy) / (dx * dx + dy * dy);
    double result = t * (double)length;
    if (!isfinite(result)) {
        return 0;
    }
    *u = result;
    return 1;
}
