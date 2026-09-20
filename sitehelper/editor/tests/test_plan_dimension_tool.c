#include <assert.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>

#include "plan_dimension_tool.h"

static DocumentDimensionReference fixed(int x,int y)
{ return (DocumentDimensionReference){.kind=DOCUMENT_DIMENSION_FIXED_POINT,.position={x,y}}; }

static void test_basic_authoring_and_cancel(void)
{
    PlanDimensionTool tool; plan_dimension_tool_init(&tool);
    assert(!tool.active); plan_dimension_tool_activate(&tool);
    assert(tool.active&&tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);
    assert(plan_dimension_tool_set_first(&tool,fixed(0,0),(PlanPosition){0,0}));
    assert(plan_dimension_tool_update_pointer(&tool,(PlanPoint){3000,0}));
    assert(plan_dimension_tool_set_second(&tool,fixed(3000,0),(PlanPosition){3000,0}));
    assert(plan_dimension_tool_update_pointer(&tool,(PlanPoint){1500,400}));
    DocumentDimensionReference a,b; int offset;
    assert(plan_dimension_tool_command_data(&tool,&a,&b,&offset));
    assert(offset==400&&a.position.x==0&&b.position.x==3000);
    assert(plan_dimension_tool_update_pointer(&tool,(PlanPoint){1500,-250}));
    assert(plan_dimension_tool_command_data(&tool,&a,&b,&offset)&&offset==-250);
    plan_dimension_tool_cancel(&tool);
    assert(tool.active&&tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);
}

static void test_rejects_inconsistent_fixed_reference_and_unrepresentable_distance(void)
{
    PlanDimensionTool tool; plan_dimension_tool_init(&tool); plan_dimension_tool_activate(&tool);
    assert(!plan_dimension_tool_set_first(&tool,fixed(100,200),(PlanPosition){101,200}));
    assert(tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);
    assert(plan_dimension_tool_set_first(&tool,fixed(INT_MIN,0),(PlanPosition){INT_MIN,0}));
    assert(!plan_dimension_tool_set_second(&tool,fixed(INT_MAX,0),(PlanPosition){INT_MAX,0}));
    assert(tool.stage==PLAN_DIMENSION_TOOL_PICK_SECOND);
}

static void test_failed_offset_update_is_transactional(void)
{
    PlanDimensionTool tool; plan_dimension_tool_init(&tool); plan_dimension_tool_activate(&tool);
    assert(plan_dimension_tool_set_first(&tool,fixed(0,0),(PlanPosition){0,0}));
    assert(plan_dimension_tool_set_second(&tool,fixed(1000,0),(PlanPosition){1000,0}));
    assert(plan_dimension_tool_update_pointer(&tool,(PlanPoint){500,250}));
    PlanPoint previous_pointer=tool.pointer;
    int previous_offset=tool.offset_mm;
    assert(previous_offset==250);
    assert(!plan_dimension_tool_update_pointer(&tool,(PlanPoint){0,DBL_MAX}));
    assert(tool.pointer.x==previous_pointer.x&&tool.pointer.y==previous_pointer.y);
    assert(tool.offset_mm==previous_offset);
}

int main(void)
{
    test_basic_authoring_and_cancel();
    test_rejects_inconsistent_fixed_reference_and_unrepresentable_distance();
    test_failed_offset_update_is_transactional();
    puts("All plan dimension tool tests passed.");
    return 0;
}
