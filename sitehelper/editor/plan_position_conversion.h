#ifndef PLAN_POSITION_CONVERSION_H
#define PLAN_POSITION_CONVERSION_H

#include "position.h"

/* Editing boundary shared by plan tools. Finite representable double-mm values
 * convert by C truncation toward zero, matching established Wall Tool behavior.
 * Output is unchanged on failure. */
int plan_position_from_point(PlanPoint point, PlanPosition *output);

#endif
