#include "plan_direction_symbol_tool.h"

#include <math.h>
#include <stddef.h>

void plan_direction_symbol_tool_init(PlanDirectionSymbolTool *tool)
{
    if (tool != NULL) { *tool=(PlanDirectionSymbolTool){0}; }
}

void plan_direction_symbol_tool_activate(PlanDirectionSymbolTool *tool)
{
    if (tool != NULL) {
        *tool=(PlanDirectionSymbolTool){.active=1,
            .stage=PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR};
    }
}

void plan_direction_symbol_tool_cancel(PlanDirectionSymbolTool *tool)
{
    if (tool == NULL) { return; }
    int active=tool->active;
    *tool=(PlanDirectionSymbolTool){.active=active,
        .stage=PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR};
}

int plan_direction_symbol_tool_set_anchor(PlanDirectionSymbolTool *tool, PlanPosition anchor)
{
    if (tool == NULL || !tool->active ||
        tool->stage != PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR) { return 0; }
    tool->anchor=anchor;
    tool->direction_point=anchor;
    tool->pointer=(PlanPoint){anchor.x,anchor.y};
    tool->has_pointer=1;
    tool->stage=PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION;
    return 1;
}

int plan_direction_symbol_tool_update_pointer(PlanDirectionSymbolTool *tool, PlanPoint pointer)
{
    if (tool == NULL || !tool->active ||
        tool->stage != PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION ||
        !isfinite(pointer.x) || !isfinite(pointer.y)) { return 0; }
    tool->pointer=pointer;
    tool->has_pointer=1;
    return 1;
}

int plan_direction_symbol_tool_set_direction_point(PlanDirectionSymbolTool *tool,
    PlanPosition direction_point)
{
    DocumentPlanDirection direction;
    if (tool == NULL || !tool->active ||
        tool->stage != PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION ||
        !document_plan_direction_from_points(tool->anchor,direction_point,&direction)) { return 0; }
    tool->direction_point=direction_point;
    tool->pointer=(PlanPoint){direction_point.x,direction_point.y};
    tool->has_pointer=1;
    return 1;
}

int plan_direction_symbol_tool_ready(const PlanDirectionSymbolTool *tool,
    PlanPosition *anchor, DocumentPlanDirection *direction)
{
    if (tool == NULL || anchor == NULL || direction == NULL || !tool->active ||
        tool->stage != PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION ||
        !document_plan_direction_from_points(tool->anchor,tool->direction_point,direction)) {
        return 0;
    }
    *anchor=tool->anchor;
    return 1;
}
