#include "plan_dimension_tool.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

static int reference_matches_position(DocumentDimensionReference reference, PlanPosition position)
{
    return reference.kind != DOCUMENT_DIMENSION_FIXED_POINT ||
        (reference.position.x == position.x && reference.position.y == position.y);
}

static int positions_have_valid_distance(PlanPosition first, PlanPosition second)
{
    const double dx=(double)second.x-(double)first.x;
    const double dy=(double)second.y-(double)first.y;
    const double distance=round(hypot(dx,dy));
    return isfinite(distance) && distance >= 1.0 && distance <= (double)INT_MAX;
}

void plan_dimension_tool_init(PlanDimensionTool *tool)
{
    if (tool != NULL) { *tool=(PlanDimensionTool){0}; }
}

void plan_dimension_tool_activate(PlanDimensionTool *tool)
{
    if (tool != NULL) {
        *tool=(PlanDimensionTool){.active=1,.stage=PLAN_DIMENSION_TOOL_PICK_FIRST};
    }
}

void plan_dimension_tool_cancel(PlanDimensionTool *tool)
{
    if (tool != NULL) {
        int active=tool->active;
        *tool=(PlanDimensionTool){.active=active,.stage=PLAN_DIMENSION_TOOL_PICK_FIRST};
    }
}

int plan_dimension_tool_set_first(PlanDimensionTool *tool,
    DocumentDimensionReference reference, PlanPosition position)
{
    if (tool == NULL || !tool->active ||
        tool->stage != PLAN_DIMENSION_TOOL_PICK_FIRST ||
        !document_dimension_reference_is_locally_valid(&reference) ||
        !reference_matches_position(reference,position)) { return 0; }
    tool->first=reference;
    tool->first_position=position;
    tool->second=(DocumentDimensionReference){0};
    tool->second_position=position;
    tool->pointer=(PlanPoint){position.x,position.y};
    tool->offset_mm=0;
    tool->has_pointer=1;
    tool->stage=PLAN_DIMENSION_TOOL_PICK_SECOND;
    return 1;
}

int plan_dimension_tool_set_second(PlanDimensionTool *tool,
    DocumentDimensionReference reference, PlanPosition position)
{
    if (tool == NULL || !tool->active ||
        tool->stage != PLAN_DIMENSION_TOOL_PICK_SECOND ||
        !document_dimension_reference_is_locally_valid(&reference) ||
        !reference_matches_position(reference,position) ||
        !positions_have_valid_distance(tool->first_position,position)) {
        return 0;
    }
    tool->second=reference;
    tool->second_position=position;
    tool->pointer=(PlanPoint){position.x,position.y};
    tool->offset_mm=0;
    tool->has_pointer=1;
    tool->stage=PLAN_DIMENSION_TOOL_PLACE_OFFSET;
    return 1;
}

int plan_dimension_tool_update_pointer(PlanDimensionTool *tool, PlanPoint pointer)
{
    if (tool == NULL || !tool->active || !isfinite(pointer.x) || !isfinite(pointer.y) ||
        tool->stage == PLAN_DIMENSION_TOOL_PICK_FIRST) { return 0; }
    if (tool->stage == PLAN_DIMENSION_TOOL_PLACE_OFFSET) {
        const double dx=(double)tool->second_position.x-tool->first_position.x;
        const double dy=(double)tool->second_position.y-tool->first_position.y;
        const double length=hypot(dx,dy);
        if (!(length > 0.0) || !isfinite(length)) { return 0; }
        const double offset=(-dy/length)*(pointer.x-tool->first_position.x) +
            (dx/length)*(pointer.y-tool->first_position.y);
        if (!isfinite(offset) || offset < INT_MIN || offset > INT_MAX) { return 0; }
        /* Commit transient pointer state only after the offset is representable.
         * A failed hover/click must not leave a stale offset paired with a new
         * invalid pointer. */
        tool->offset_mm=(int)lround(offset);
    }
    tool->pointer=pointer;
    tool->has_pointer=1;
    return 1;
}

int plan_dimension_tool_has_started(const PlanDimensionTool *tool)
{
    return tool != NULL && tool->active && tool->stage != PLAN_DIMENSION_TOOL_PICK_FIRST;
}

int plan_dimension_tool_ready(const PlanDimensionTool *tool)
{
    return tool != NULL && tool->active && tool->stage == PLAN_DIMENSION_TOOL_PLACE_OFFSET;
}

int plan_dimension_tool_command_data(const PlanDimensionTool *tool,
    DocumentDimensionReference *first, DocumentDimensionReference *second,
    int *offset_mm)
{
    if (!plan_dimension_tool_ready(tool) || first == NULL || second == NULL ||
        offset_mm == NULL) { return 0; }
    *first=tool->first;
    *second=tool->second;
    *offset_mm=tool->offset_mm;
    return 1;
}
