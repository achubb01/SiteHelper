#ifndef WALL_PLAN_TRANSFORM_H
#define WALL_PLAN_TRANSFORM_H

#include "wall_plan_segment.h"

/*
 * Convert between wall-local U (millimetres) and calculated physical plan X/Y.
 * Positive U follows start toward end. U = 0 maps exactly to start and
 * U = wall_plan_segment_length_mm(segment) maps exactly to end, using the
 * authoritative rounded length rather than raw Euclidean distance.
 * The inverse projects off-axis points onto the wall axis. Neither function
 * clamps to the finite segment.
 *
 * Return 1 on success; 0 for a null output, a segment rejected by the checked
 * length helper, non-finite input, or non-finite calculation. On failure the
 * output is unchanged.
 */
int wall_plan_segment_u_to_plan(WallPlanSegment segment, double u, PlanPoint *point);
int wall_plan_segment_plan_to_u(WallPlanSegment segment, PlanPoint point, double *u);

#endif
