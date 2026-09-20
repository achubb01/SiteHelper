#include <assert.h>
#include <stdio.h>

#include "plan_direction_symbol_tool.h"

static void test_direction_normalisation(void)
{
    DocumentPlanDirection direction;
    assert(document_plan_direction_from_points((PlanPosition){100,200},
        (PlanPosition){700,1000},&direction));
    assert(direction.dx==3&&direction.dy==4);
    assert(document_plan_direction_is_canonical(direction));
    assert(!document_plan_direction_is_canonical((DocumentPlanDirection){6,8}));
    assert(!document_plan_direction_from_points((PlanPosition){1,2},
        (PlanPosition){1,2},&direction));
}

static void test_two_click_tool(void)
{
    PlanDirectionSymbolTool tool; plan_direction_symbol_tool_init(&tool);
    plan_direction_symbol_tool_activate(&tool);
    assert(tool.active&&tool.stage==PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR);
    assert(plan_direction_symbol_tool_set_anchor(&tool,(PlanPosition){100,200}));
    assert(plan_direction_symbol_tool_update_pointer(&tool,(PlanPoint){400.5,600.25}));
    assert(!plan_direction_symbol_tool_set_direction_point(&tool,(PlanPosition){100,200}));
    assert(plan_direction_symbol_tool_set_direction_point(&tool,(PlanPosition){700,1000}));
    PlanPosition anchor; DocumentPlanDirection direction;
    assert(plan_direction_symbol_tool_ready(&tool,&anchor,&direction));
    assert(anchor.x==100&&anchor.y==200&&direction.dx==3&&direction.dy==4);
    plan_direction_symbol_tool_cancel(&tool);
    assert(tool.active&&tool.stage==PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR);
}

int main(void)
{
    test_direction_normalisation();
    test_two_click_tool();
    puts("All plan direction symbol tool tests passed.");
    return 0;
}
