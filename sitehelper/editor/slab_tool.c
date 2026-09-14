#include <stdlib.h>
#include <stdint.h>
#include "slab_tool.h"

void slab_tool_init(SlabTool *tool)
{
    if (tool == NULL) { return; }
    *tool=(SlabTool){.thickness_mm=SLAB_TOOL_DEFAULT_THICKNESS_MM,
        .top_level_offset_mm=SLAB_TOOL_DEFAULT_TOP_LEVEL_OFFSET_MM};
}
void slab_tool_destroy(SlabTool *tool)
{
    if (tool == NULL) { return; }
    free(tool->vertices); *tool=(SlabTool){0};
}
void slab_tool_activate(SlabTool *tool)
{
    if (tool == NULL) { return; }
    slab_tool_cancel(tool); tool->active=1;
}
void slab_tool_cancel(SlabTool *tool)
{
    if (tool == NULL) { return; }
    free(tool->vertices);
    int thickness=tool->thickness_mm > 0 ? tool->thickness_mm : SLAB_TOOL_DEFAULT_THICKNESS_MM;
    int offset=tool->top_level_offset_mm;
    *tool=(SlabTool){.thickness_mm=thickness,.top_level_offset_mm=offset};
}
int slab_tool_append(SlabTool *tool, DomainId storey_id, PlanPosition vertex)
{
    if (tool == NULL || !tool->active || storey_id == DOMAIN_ID_INVALID ||
        (tool->vertex_count != 0 && tool->storey_id != storey_id)) { return 0; }
    if (tool->vertex_count != 0) {
        PlanPosition last=tool->vertices[tool->vertex_count-1];
        if (last.x == vertex.x && last.y == vertex.y) { return 1; }
    }
    size_t maximum=SIZE_MAX/sizeof *tool->vertices;
    if (tool->vertex_count == maximum) { return 0; }
    if (tool->vertex_count == tool->vertex_capacity) {
        size_t grown=tool->vertex_capacity == 0 ? 8 :
            tool->vertex_capacity > maximum/2 ? maximum : tool->vertex_capacity*2;
        PlanPosition *items=realloc(tool->vertices,grown*sizeof *items);
        if (items == NULL) { return 0; }
        tool->vertices=items; tool->vertex_capacity=grown;
    }
    if (tool->vertex_count == 0) { tool->storey_id=storey_id; }
    tool->vertices[tool->vertex_count++]=vertex;
    return 1;
}
void slab_tool_update(SlabTool *tool, PlanPoint preview)
{
    if (tool == NULL || !tool->active) { return; }
    tool->preview=preview; tool->has_preview=1;
}
