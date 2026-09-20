#include <assert.h>
#include <stdio.h>
#include "plan_revision_cloud_tool.h"

int main(void)
{
    PlanRevisionCloudTool tool; plan_revision_cloud_tool_init(&tool);
    plan_revision_cloud_tool_activate(&tool); assert(tool.active);
    assert(plan_revision_cloud_tool_append(&tool,7,(PlanPosition){0,0}));
    assert(plan_revision_cloud_tool_append(&tool,7,(PlanPosition){100,0}));
    assert(plan_revision_cloud_tool_append(&tool,7,(PlanPosition){100,100}));
    assert(tool.vertex_count==3&&tool.storey_id==7);
    assert(plan_revision_cloud_tool_append(&tool,7,(PlanPosition){0,0}));
    assert(tool.vertex_count==3); /* implicit closure, no duplicate first vertex */
    plan_revision_cloud_tool_update(&tool,(PlanPoint){20.5,30.25}); assert(tool.has_preview);
    plan_revision_cloud_tool_cancel(&tool); assert(tool.vertex_count==0&&!tool.active);
    plan_revision_cloud_tool_destroy(&tool);
    puts("All plan revision cloud tool tests passed.");
    return 0;
}
