#include "measurement_tool.h"
#include <math.h>
#include <stddef.h>

void measurement_tool_init(MeasurementTool *tool)
{
    if (tool != NULL) { *tool = (MeasurementTool){0}; }
}

void measurement_tool_activate(MeasurementTool *tool)
{
    if (tool != NULL) { *tool = (MeasurementTool){.active = 1}; }
}

void measurement_tool_cancel(MeasurementTool *tool)
{
    if (tool != NULL) { *tool = (MeasurementTool){.active = tool->active}; }
}

static int valid_point(PlanPoint point)
{
    return isfinite(point.x) && isfinite(point.y);
}

static int update_end(MeasurementTool *tool, PlanPoint point, int completed)
{
    double distance = hypot(point.x - tool->query.start.x, point.y - tool->query.start.y);
    if (!isfinite(distance)) { return 0; }
    tool->query.end = point;
    tool->query.distance_mm = distance;
    tool->query.completed = completed;
    return 1;
}

int measurement_tool_update(MeasurementTool *tool, PlanPoint point)
{
    if (tool == NULL || !tool->active || !valid_point(point)) { return 0; }
    if (!tool->has_start || tool->query.completed) { return 1; }
    return update_end(tool, point, 0);
}

int measurement_tool_click(MeasurementTool *tool, PlanPoint point)
{
    if (tool == NULL || !tool->active || !valid_point(point)) { return 0; }
    if (!tool->has_start || tool->query.completed) {
        tool->query = (PlanMeasurementQuery){.start = point, .end = point};
        tool->has_start = 1;
        return 1;
    }
    return update_end(tool, point, 1);
}

int measurement_tool_get_query(const MeasurementTool *tool, PlanMeasurementQuery *query)
{
    if (query == NULL) { return 0; }
    *query = (PlanMeasurementQuery){0};
    if (tool == NULL || !tool->active || !tool->has_start) { return 0; }
    *query = tool->query;
    return 1;
}
