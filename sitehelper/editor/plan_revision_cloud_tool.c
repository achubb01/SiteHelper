#include "plan_revision_cloud_tool.h"

#include <stdint.h>
#include <stdlib.h>

void plan_revision_cloud_tool_init(PlanRevisionCloudTool *tool)
{
    if (tool != NULL) { *tool = (PlanRevisionCloudTool){0}; }
}

void plan_revision_cloud_tool_destroy(PlanRevisionCloudTool *tool)
{
    if (tool == NULL) { return; }
    free(tool->vertices);
    *tool = (PlanRevisionCloudTool){0};
}

void plan_revision_cloud_tool_cancel(PlanRevisionCloudTool *tool)
{
    if (tool == NULL) { return; }
    free(tool->vertices);
    *tool = (PlanRevisionCloudTool){0};
}

void plan_revision_cloud_tool_activate(PlanRevisionCloudTool *tool)
{
    if (tool == NULL) { return; }
    plan_revision_cloud_tool_cancel(tool);
    tool->active = 1;
}

int plan_revision_cloud_tool_append(PlanRevisionCloudTool *tool,
    DomainId storey_id, PlanPosition vertex)
{
    if (tool == NULL || !tool->active || storey_id == DOMAIN_ID_INVALID ||
        (tool->vertex_count != 0 && tool->storey_id != storey_id)) {
        return 0;
    }
    if (tool->vertex_count != 0) {
        PlanPosition last = tool->vertices[tool->vertex_count - 1];
        if (last.x == vertex.x && last.y == vertex.y) { return 1; }
        /* Closure is implicit. Clicking the first point again after a valid
         * sketch is treated as a no-op; Enter remains the explicit commit. */
        if (tool->vertex_count >= 3 && tool->vertices[0].x == vertex.x &&
            tool->vertices[0].y == vertex.y) { return 1; }
    }
    const size_t maximum = SIZE_MAX / sizeof *tool->vertices;
    if (tool->vertex_count == maximum) { return 0; }
    if (tool->vertex_count == tool->vertex_capacity) {
        size_t capacity = tool->vertex_capacity == 0 ? 8 :
            tool->vertex_capacity > maximum / 2 ? maximum : tool->vertex_capacity * 2;
        PlanPosition *vertices = realloc(tool->vertices, capacity * sizeof *vertices);
        if (vertices == NULL) { return 0; }
        tool->vertices = vertices;
        tool->vertex_capacity = capacity;
    }
    if (tool->vertex_count == 0) { tool->storey_id = storey_id; }
    tool->vertices[tool->vertex_count++] = vertex;
    return 1;
}

void plan_revision_cloud_tool_update(PlanRevisionCloudTool *tool, PlanPoint preview)
{
    if (tool == NULL || !tool->active) { return; }
    tool->preview = preview;
    tool->has_preview = 1;
}
