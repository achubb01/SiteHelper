#include "plan_callout_tool.h"

#include <math.h>
#include <stddef.h>

void plan_callout_tool_init(PlanCalloutTool *tool)
{
    if (tool != NULL) { *tool=(PlanCalloutTool){0}; }
}

void plan_callout_tool_activate(PlanCalloutTool *tool)
{
    if (tool == NULL) { return; }
    *tool=(PlanCalloutTool){.active=1,.stage=PLAN_CALLOUT_TOOL_PICK_TARGET};
}

void plan_callout_tool_cancel(PlanCalloutTool *tool)
{
    int active=tool != NULL ? tool->active : 0;
    if (tool != NULL) { *tool=(PlanCalloutTool){.active=active,.stage=PLAN_CALLOUT_TOOL_PICK_TARGET}; }
}

int plan_callout_tool_set_target(PlanCalloutTool *tool, PlanPosition target)
{
    if (tool == NULL || !tool->active || tool->stage != PLAN_CALLOUT_TOOL_PICK_TARGET) { return 0; }
    tool->target=target;
    tool->stage=PLAN_CALLOUT_TOOL_PICK_LABEL;
    tool->has_pointer=0;
    return 1;
}

int plan_callout_tool_update_pointer(PlanCalloutTool *tool, PlanPoint pointer)
{
    if (tool == NULL || !tool->active || tool->stage != PLAN_CALLOUT_TOOL_PICK_LABEL ||
        !isfinite(pointer.x) || !isfinite(pointer.y)) { return 0; }
    tool->pointer=pointer;
    tool->has_pointer=1;
    return 1;
}

int plan_callout_tool_set_label(PlanCalloutTool *tool, PlanPosition label_anchor)
{
    if (tool == NULL || !tool->active || tool->stage != PLAN_CALLOUT_TOOL_PICK_LABEL ||
        (label_anchor.x == tool->target.x && label_anchor.y == tool->target.y)) { return 0; }
    tool->label_anchor=label_anchor;
    tool->stage=PLAN_CALLOUT_TOOL_READY_TEXT;
    tool->pointer=(PlanPoint){label_anchor.x,label_anchor.y};
    tool->has_pointer=1;
    return 1;
}

int plan_callout_tool_ready(const PlanCalloutTool *tool,
    PlanPosition *target, PlanPosition *label_anchor)
{
    if (tool == NULL || target == NULL || label_anchor == NULL || !tool->active ||
        tool->stage != PLAN_CALLOUT_TOOL_READY_TEXT) { return 0; }
    *target=tool->target;
    *label_anchor=tool->label_anchor;
    return 1;
}
