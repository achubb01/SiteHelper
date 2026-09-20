#ifndef PLAN_REVISION_CLOUD_TOOL_H
#define PLAN_REVISION_CLOUD_TOOL_H

#include <stddef.h>

#include "domain_id.h"
#include "geometry.h"
#include "position.h"

typedef struct {
    PlanPosition *vertices;
    size_t vertex_count;
    size_t vertex_capacity;
    DomainId storey_id;
    PlanPoint preview;
    int has_preview;
    int active;
} PlanRevisionCloudTool;

void plan_revision_cloud_tool_init(PlanRevisionCloudTool *tool);
void plan_revision_cloud_tool_destroy(PlanRevisionCloudTool *tool);
void plan_revision_cloud_tool_activate(PlanRevisionCloudTool *tool);
void plan_revision_cloud_tool_cancel(PlanRevisionCloudTool *tool);
int plan_revision_cloud_tool_append(PlanRevisionCloudTool *tool,
    DomainId storey_id, PlanPosition vertex);
void plan_revision_cloud_tool_update(PlanRevisionCloudTool *tool, PlanPoint preview);

#endif
