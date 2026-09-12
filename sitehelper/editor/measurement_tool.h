#ifndef MEASUREMENT_TOOL_H
#define MEASUREMENT_TOOL_H

#include "position.h"

/* Calculated plan-space millimetres, never a persistent dimension annotation.
 * No identity or geometry references. End is the live point or completed B. */
typedef struct {
    PlanPoint start, end;
    double distance_mm;
    int completed;
} PlanMeasurementQuery;

typedef struct {
    int active;
    int has_start;
    PlanMeasurementQuery query;
} MeasurementTool;

void measurement_tool_init(MeasurementTool *tool);
void measurement_tool_activate(MeasurementTool *tool); /* Starts empty. */
void measurement_tool_cancel(MeasurementTool *tool);   /* Clears query, stays active. */
/* Rejected nonfinite points/distances leave the previous valid state unchanged.
 * Updates after completion do not move B. Click after completion starts anew. */
int measurement_tool_update(MeasurementTool *tool, PlanPoint point);
int measurement_tool_click(MeasurementTool *tool, PlanPoint point);
/* Copy out a live/completed query; absence/failure clears a nonnull output. */
int measurement_tool_get_query(const MeasurementTool *tool, PlanMeasurementQuery *query);

#endif
