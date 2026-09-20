#ifndef PLAN_CALLOUT_TOOL_H
#define PLAN_CALLOUT_TOOL_H

#include "position.h"

typedef enum {
    PLAN_CALLOUT_TOOL_PICK_TARGET = 0,
    PLAN_CALLOUT_TOOL_PICK_LABEL,
    PLAN_CALLOUT_TOOL_READY_TEXT
} PlanCalloutToolStage;

typedef struct {
    int active;
    PlanCalloutToolStage stage;
    PlanPosition target;
    PlanPosition label_anchor;
    PlanPoint pointer;
    int has_pointer;
} PlanCalloutTool;

void plan_callout_tool_init(PlanCalloutTool *tool);
void plan_callout_tool_activate(PlanCalloutTool *tool);
void plan_callout_tool_cancel(PlanCalloutTool *tool);
int plan_callout_tool_set_target(PlanCalloutTool *tool, PlanPosition target);
int plan_callout_tool_update_pointer(PlanCalloutTool *tool, PlanPoint pointer);
int plan_callout_tool_set_label(PlanCalloutTool *tool, PlanPosition label_anchor);
int plan_callout_tool_ready(const PlanCalloutTool *tool,
    PlanPosition *target, PlanPosition *label_anchor);

#endif
