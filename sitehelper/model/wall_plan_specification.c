#include "wall_plan_specification.h"

WallPlanSpecification wall_plan_specification_default(void)
{
    return (WallPlanSpecification){
        .thickness_mm = WALL_DEFAULT_NOMINAL_THICKNESS_MM,
        .alignment = WALL_PLAN_ALIGNMENT_CENTER
    };
}

int wall_plan_specification_valid(WallPlanSpecification specification)
{
    return specification.thickness_mm > 0 &&
        specification.alignment >= WALL_PLAN_ALIGNMENT_CENTER &&
        specification.alignment <= WALL_PLAN_ALIGNMENT_RIGHT_FACE;
}
