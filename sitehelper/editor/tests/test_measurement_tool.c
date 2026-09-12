#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "measurement_tool.h"

static PlanMeasurementQuery query(const MeasurementTool *tool)
{
    PlanMeasurementQuery result;
    assert(measurement_tool_get_query(tool, &result));
    assert(isfinite(result.start.x) && isfinite(result.start.y));
    assert(isfinite(result.end.x) && isfinite(result.end.y) && isfinite(result.distance_mm));
    return result;
}
static void same(PlanMeasurementQuery a, PlanMeasurementQuery b)
{
    assert(a.start.x == b.start.x && a.start.y == b.start.y && a.end.x == b.end.x && a.end.y == b.end.y);
    assert(a.distance_mm == b.distance_mm && a.completed == b.completed);
}
int main(void)
{
    MeasurementTool tool;
    PlanMeasurementQuery result = {.distance_mm = 999};
    measurement_tool_init(&tool);
    assert(!tool.active && !tool.has_start);
    assert(!measurement_tool_get_query(&tool, &result) && result.distance_mm == 0);
    assert(!measurement_tool_click(&tool, (PlanPoint){0,0}));
    measurement_tool_activate(&tool);
    assert(tool.active && !tool.has_start);
    assert(measurement_tool_update(&tool, (PlanPoint){10,20}) && !tool.has_start);
    assert(measurement_tool_click(&tool, (PlanPoint){-10.25,-20.5}));
    result = query(&tool);
    assert(!result.completed && result.distance_mm == 0 && result.start.x == -10.25);
    assert(measurement_tool_update(&tool, (PlanPoint){-7.25,-16.5}));
    result = query(&tool);
    assert(!result.completed && result.distance_mm == 5);
    assert(measurement_tool_click(&tool, (PlanPoint){-4.25,-12.5}));
    result = query(&tool);
    assert(result.completed && result.distance_mm == 10);
    assert(measurement_tool_update(&tool, (PlanPoint){100,200}));
    same(result, query(&tool));
    assert(measurement_tool_click(&tool, (PlanPoint){1,1}));
    assert(!query(&tool).completed && query(&tool).distance_mm == 0);
    assert(measurement_tool_click(&tool, (PlanPoint){1,1}));
    assert(query(&tool).completed && query(&tool).distance_mm == 0);
    measurement_tool_cancel(&tool);
    assert(tool.active && !tool.has_start && !measurement_tool_get_query(&tool, &result));
    assert(measurement_tool_click(&tool, (PlanPoint){INT_MIN,INT_MIN}));
    assert(measurement_tool_click(&tool, (PlanPoint){INT_MAX,INT_MAX}));
    result = query(&tool);
    assert(result.distance_mm > INT_MAX);
    assert(result.distance_mm == hypot((double)INT_MAX - INT_MIN, (double)INT_MAX - INT_MIN));
    const PlanPoint invalid[] = {{NAN,0},{0,NAN},{INFINITY,0},{0,-INFINITY}};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        assert(!measurement_tool_click(&tool, invalid[i]));
        assert(!measurement_tool_update(&tool, invalid[i]));
        same(result, query(&tool));
    }
    assert(measurement_tool_click(&tool, (PlanPoint){-DBL_MAX,0}));
    result = query(&tool);
    assert(!measurement_tool_update(&tool, (PlanPoint){DBL_MAX,0}));
    assert(!measurement_tool_click(&tool, (PlanPoint){DBL_MAX,0}));
    same(result, query(&tool));
    assert(measurement_tool_click(&tool, (PlanPoint){-DBL_MAX,0}));
    assert(query(&tool).completed && query(&tool).distance_mm == 0);
    measurement_tool_activate(&tool);
    assert(tool.active && !tool.has_start);
    assert(!measurement_tool_click(&tool, (PlanPoint){NAN,0}) && !tool.has_start);
    assert(measurement_tool_click(&tool, (PlanPoint){0,0}));
    assert(measurement_tool_click(&tool, (PlanPoint){1,1}));
    assert(fabs(query(&tool).distance_mm - sqrt(2.0)) < 1e-12);
    measurement_tool_init(&tool);
    assert(!measurement_tool_get_query(&tool, &result));
    assert(!measurement_tool_get_query(NULL, &result));
    assert(!measurement_tool_get_query(&tool, NULL));
    assert(!measurement_tool_update(NULL, (PlanPoint){0}));
    assert(!measurement_tool_click(NULL, (PlanPoint){0}));
    measurement_tool_init(NULL); measurement_tool_activate(NULL); measurement_tool_cancel(NULL);
    puts("measurement tool tests passed");
}
