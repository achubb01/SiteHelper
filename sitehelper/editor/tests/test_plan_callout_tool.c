#include <assert.h>
#include <stdio.h>

#include "plan_callout_tool.h"

int main(void)
{
    PlanCalloutTool tool; plan_callout_tool_init(&tool);
    assert(!tool.active);
    plan_callout_tool_activate(&tool);
    assert(tool.active&&tool.stage==PLAN_CALLOUT_TOOL_PICK_TARGET);
    assert(plan_callout_tool_set_target(&tool,(PlanPosition){100,200}));
    assert(tool.stage==PLAN_CALLOUT_TOOL_PICK_LABEL);
    assert(plan_callout_tool_update_pointer(&tool,(PlanPoint){400.25,600.5}));
    assert(!plan_callout_tool_set_label(&tool,(PlanPosition){100,200}));
    assert(tool.stage==PLAN_CALLOUT_TOOL_PICK_LABEL);
    assert(plan_callout_tool_set_label(&tool,(PlanPosition){400,600}));
    PlanPosition target,label;
    assert(plan_callout_tool_ready(&tool,&target,&label));
    assert(target.x==100&&target.y==200&&label.x==400&&label.y==600);
    plan_callout_tool_cancel(&tool);
    assert(tool.active&&tool.stage==PLAN_CALLOUT_TOOL_PICK_TARGET);
    puts("All plan callout tool tests passed.");
    return 0;
}
