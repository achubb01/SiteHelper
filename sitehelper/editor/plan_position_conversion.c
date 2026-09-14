#include <limits.h>
#include <math.h>
#include <stddef.h>
#include "plan_position_conversion.h"

int plan_position_from_point(PlanPoint point, PlanPosition *output)
{
    if (output == NULL || !isfinite(point.x) || !isfinite(point.y) ||
        point.x < INT_MIN || point.x > INT_MAX ||
        point.y < INT_MIN || point.y > INT_MAX) { return 0; }
    *output=(PlanPosition){(int)point.x,(int)point.y};
    return 1;
}
