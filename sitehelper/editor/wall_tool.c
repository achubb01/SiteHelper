#include "wall_tool.h"

#include <stddef.h>
#include <limits.h>
#include <math.h>
#include "plan_position_conversion.h"

void wall_tool_init(WallTool *tool)
{
    if (tool != NULL) {
        *tool = (WallTool){0};
    }
}

void wall_tool_activate(WallTool *tool)
{
    if (tool != NULL) {
        tool->active = 1;
    }
}

void wall_tool_cancel(WallTool *tool)
{
    if (tool != NULL) {
        tool->has_start = 0;
        tool->length_mm = 0;
    }
}

void wall_tool_update(WallTool *tool, Vec2 endpoint)
{
    if (tool != NULL && tool->active && tool->has_start) {
        tool->endpoint = endpoint;
        tool->pointer = endpoint;
    }
}

int wall_tool_begin(WallTool *tool, Vec2 start)
{
    if (tool == NULL || !tool->active) {
        return 0;
    }

    tool->start = start;
    tool->endpoint = start;
    tool->pointer = start;
    tool->length_mm = 0;
    tool->has_start = 1;
    return 1;
}

int wall_tool_command_data(
    const WallTool *tool,
    WallPlanSegment *segment
)
{
    if (tool == NULL || segment == NULL ||
        !tool->active || !tool->has_start) {

        return 0;
    }

    if (tool->length_mm != 0) {
        return wall_tool_resolve_length(tool, tool->length_mm, segment) == WALL_LENGTH_OK;
    }

    WallPlanSegment candidate;
    if (!plan_position_from_point((PlanPoint){tool->start.x,tool->start.y},&candidate.start) ||
        !plan_position_from_point((PlanPoint){tool->endpoint.x,tool->endpoint.y},&candidate.end)) { return 0; }
    if (wall_plan_segment_length_mm(candidate) == 0) {
        return 0;
    }
    *segment = candidate;
    return 1;
}

void wall_tool_update_direction(WallTool *tool, Vec2 pointer)
{
    if (tool != NULL && tool->active && tool->has_start) { tool->pointer = pointer; }
}

void wall_tool_clear_length(WallTool *tool)
{
    if (tool != NULL) { tool->length_mm = 0; }
}

WallLengthStatus wall_tool_set_length(WallTool *tool, int length_mm)
{
    if (tool == NULL || !tool->active || !tool->has_start) { return WALL_LENGTH_INACTIVE; }
    tool->length_mm = 0;
    if (length_mm <= 0) { return WALL_LENGTH_NONPOSITIVE; }
    tool->length_mm = length_mm;
    WallPlanSegment segment;
    return wall_tool_resolve_length(tool, length_mm, &segment);
}

WallLengthStatus wall_tool_resolve_length(const WallTool *tool, int length_mm,
    WallPlanSegment *segment)
{
    if (tool == NULL || segment == NULL || !tool->active || !tool->has_start) {
        return WALL_LENGTH_INACTIVE;
    }
    if (length_mm <= 0) { return WALL_LENGTH_NONPOSITIVE; }
    if (!isfinite(tool->start.x) || !isfinite(tool->start.y) ||
        tool->start.x < INT_MIN || tool->start.x > INT_MAX ||
        tool->start.y < INT_MIN || tool->start.y > INT_MAX ||
        !isfinite(tool->pointer.x) || !isfinite(tool->pointer.y)) {
        return WALL_LENGTH_OUT_OF_RANGE;
    }
    double dx = tool->pointer.x - tool->start.x;
    double dy = tool->pointer.y - tool->start.y;
    /* Scale first so finite very large/subnormal directions normalize safely. */
    double magnitude = fmax(fabs(dx), fabs(dy));
    if (!isfinite(magnitude)) { return WALL_LENGTH_OUT_OF_RANGE; }
    if (magnitude == 0.0) { return WALL_LENGTH_DIRECTIONLESS; }
    dx /= magnitude;
    dy /= magnitude;
    double distance = hypot(dx, dy);
    PlanPosition start;
    if (!plan_position_from_point((PlanPoint){tool->start.x,tool->start.y},&start)) {
        return WALL_LENGTH_OUT_OF_RANGE;
    }
    double x = (double)start.x + (dx / distance) * length_mm;
    double y = (double)start.y + (dy / distance) * length_mm;
    double xs[] = {floor(x), ceil(x)}, ys[] = {floor(y), ceil(y)};
    int found = 0;
    double best_error = INFINITY;
    WallPlanSegment best = {0};
    /* The corners bracket the ideal radius. Each grid edge changes radius by
     * at most 1 mm, so a corner rounds to the requested integer radius. Choose
     * the nearest valid corner to the ideal endpoint (at most sqrt(2) mm away).
     * Range filtering may remove candidates at the coordinate boundary. The
     * model function is the final authority, including floating point edges. */
    for (size_t i = 0; i < 2; i++) {
        for (size_t j = 0; j < 2; j++) {
            if (xs[i] < INT_MIN || xs[i] > INT_MAX || ys[j] < INT_MIN || ys[j] > INT_MAX) { continue; }
            WallPlanSegment candidate = {start, {(int)xs[i], (int)ys[j]}};
            if (wall_plan_segment_length_mm(candidate) != length_mm) { continue; }
            double error = hypot(xs[i] - x, ys[j] - y);
            if (error < best_error) { best = candidate; best_error = error; found = 1; }
        }
    }
    if (!found) { return WALL_LENGTH_OUT_OF_RANGE; }
    *segment = best;
    return WALL_LENGTH_OK;
}
