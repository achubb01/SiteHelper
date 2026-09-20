#ifndef PLAN_DIRECTION_SYMBOL_TOOL_H
#define PLAN_DIRECTION_SYMBOL_TOOL_H

#include "document.h"
#include "position.h"

typedef enum {
    PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR = 0,
    PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION
} PlanDirectionSymbolToolStage;

typedef struct {
    int active;
    PlanDirectionSymbolToolStage stage;
    PlanPosition anchor;
    PlanPosition direction_point;
    PlanPoint pointer;
    int has_pointer;
} PlanDirectionSymbolTool;

void plan_direction_symbol_tool_init(PlanDirectionSymbolTool *tool);
void plan_direction_symbol_tool_activate(PlanDirectionSymbolTool *tool);
void plan_direction_symbol_tool_cancel(PlanDirectionSymbolTool *tool);
int plan_direction_symbol_tool_set_anchor(PlanDirectionSymbolTool *tool, PlanPosition anchor);
int plan_direction_symbol_tool_update_pointer(PlanDirectionSymbolTool *tool, PlanPoint pointer);
int plan_direction_symbol_tool_set_direction_point(PlanDirectionSymbolTool *tool,
    PlanPosition direction_point);
int plan_direction_symbol_tool_ready(const PlanDirectionSymbolTool *tool,
    PlanPosition *anchor, DocumentPlanDirection *direction);

#endif
