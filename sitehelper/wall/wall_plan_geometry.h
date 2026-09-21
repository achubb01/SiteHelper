#ifndef WALL_PLAN_GEOMETRY_H
#define WALL_PLAN_GEOMETRY_H

#include "wall.h"

typedef struct WallPlanGeometry {
    /* Counter-clockwise physical envelope in derived Plan millimetres. */
    PlanPoint corners[4];
    PlanPoint left_start, left_end;
    PlanPoint right_start, right_end;
} WallPlanGeometry;

/* Derives physical faces from authoritative datum + specification. Calculated
 * coordinates are PlanPoint doubles because a diagonal integer datum generally
 * has irrational perpendicular offsets. Nothing here is persisted. */
int wall_plan_geometry_build(const WallDefinition *definition,
    WallPlanGeometry *geometry);

/* 0 inside/on the body; otherwise shortest distance to the physical envelope. */
double wall_plan_geometry_distance(const WallPlanGeometry *geometry,
    PlanPoint point);

#endif
