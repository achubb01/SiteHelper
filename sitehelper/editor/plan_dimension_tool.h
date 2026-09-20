#ifndef PLAN_DIMENSION_TOOL_H
#define PLAN_DIMENSION_TOOL_H

#include "document.h"
#include "position.h"

typedef enum {
    PLAN_DIMENSION_TOOL_PICK_FIRST = 0,
    PLAN_DIMENSION_TOOL_PICK_SECOND,
    PLAN_DIMENSION_TOOL_PLACE_OFFSET
} PlanDimensionToolStage;

typedef struct {
    int active;
    PlanDimensionToolStage stage;
    DocumentDimensionReference first;
    DocumentDimensionReference second;
    PlanPosition first_position;
    PlanPosition second_position;
    PlanPoint pointer;
    int offset_mm;
    int has_pointer;
} PlanDimensionTool;

void plan_dimension_tool_init(PlanDimensionTool *tool);
void plan_dimension_tool_activate(PlanDimensionTool *tool);
void plan_dimension_tool_cancel(PlanDimensionTool *tool);
int plan_dimension_tool_set_first(PlanDimensionTool *tool,
    DocumentDimensionReference reference, PlanPosition position);
int plan_dimension_tool_set_second(PlanDimensionTool *tool,
    DocumentDimensionReference reference, PlanPosition position);
/* During second-point picking pointer is the resolved/snap Plan point. During
 * offset placement pointer is the raw Plan pointer so offset placement is not
 * captured by an unrelated grid/object snap. */
int plan_dimension_tool_update_pointer(PlanDimensionTool *tool, PlanPoint pointer);
int plan_dimension_tool_has_started(const PlanDimensionTool *tool);
int plan_dimension_tool_ready(const PlanDimensionTool *tool);
int plan_dimension_tool_command_data(const PlanDimensionTool *tool,
    DocumentDimensionReference *first, DocumentDimensionReference *second,
    int *offset_mm);

#endif
