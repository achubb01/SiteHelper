#ifndef WALL_PLAN_SPECIFICATION_H
#define WALL_PLAN_SPECIFICATION_H

enum { WALL_DEFAULT_NOMINAL_THICKNESS_MM = 90 };

typedef enum WallPlanAlignment {
    WALL_PLAN_ALIGNMENT_CENTER = 0,
    WALL_PLAN_ALIGNMENT_LEFT_FACE,
    WALL_PLAN_ALIGNMENT_RIGHT_FACE
} WallPlanAlignment;

typedef struct WallPlanSpecification {
    int thickness_mm;
    WallPlanAlignment alignment;
} WallPlanSpecification;

WallPlanSpecification wall_plan_specification_default(void);
int wall_plan_specification_valid(WallPlanSpecification specification);

#endif
