#include "slab_feature_tool.h"

#include <stdint.h>
#include <stdlib.h>

void slab_polygon_feature_tool_init(SlabPolygonFeatureTool *tool)
{
    if (tool != NULL) { *tool=(SlabPolygonFeatureTool){0}; }
}

void slab_polygon_feature_tool_destroy(SlabPolygonFeatureTool *tool)
{
    if (tool == NULL) { return; }
    free(tool->vertices);
    *tool=(SlabPolygonFeatureTool){0};
}

void slab_polygon_feature_tool_cancel(SlabPolygonFeatureTool *tool)
{
    if (tool == NULL) { return; }
    free(tool->vertices);
    *tool=(SlabPolygonFeatureTool){0};
}

void slab_polygon_feature_tool_activate(SlabPolygonFeatureTool *tool)
{
    if (tool == NULL) { return; }
    slab_polygon_feature_tool_cancel(tool);
    tool->active=1;
}

static int append_vertex(SlabPolygonFeatureTool *tool, PlanPosition vertex)
{
    if (tool->vertex_count != 0) {
        PlanPosition last=tool->vertices[tool->vertex_count-1];
        if (last.x == vertex.x && last.y == vertex.y) { return 1; }
    }
    size_t maximum=SIZE_MAX/sizeof *tool->vertices;
    if (tool->vertex_count == maximum) { return 0; }
    if (tool->vertex_count == tool->vertex_capacity) {
        size_t grown=tool->vertex_capacity == 0 ? 8 :
            tool->vertex_capacity > maximum/2 ? maximum : tool->vertex_capacity*2;
        PlanPosition *vertices=realloc(tool->vertices,grown*sizeof *vertices);
        if (vertices == NULL) { return 0; }
        tool->vertices=vertices;
        tool->vertex_capacity=grown;
    }
    tool->vertices[tool->vertex_count++]=vertex;
    return 1;
}

int slab_polygon_feature_tool_begin(SlabPolygonFeatureTool *tool,
    DomainId storey_id, DomainId slab_id, PlanPosition vertex,
    int top_level_offset_mm, int thickness_mm)
{
    if (tool == NULL || !tool->active || tool->vertex_count != 0 ||
        storey_id == DOMAIN_ID_INVALID || slab_id == DOMAIN_ID_INVALID ||
        thickness_mm <= 0) { return 0; }
    if (!append_vertex(tool,vertex)) { return 0; }
    tool->storey_id=storey_id;
    tool->slab_id=slab_id;
    tool->top_level_offset_mm=top_level_offset_mm;
    tool->thickness_mm=thickness_mm;
    return 1;
}

int slab_polygon_feature_tool_append(SlabPolygonFeatureTool *tool,
    PlanPosition vertex)
{
    if (tool == NULL || !tool->active || tool->vertex_count == 0 ||
        tool->slab_id == DOMAIN_ID_INVALID || tool->storey_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    return append_vertex(tool,vertex);
}

void slab_polygon_feature_tool_update(SlabPolygonFeatureTool *tool,
    PlanPoint preview)
{
    if (tool == NULL || !tool->active) { return; }
    tool->preview=preview;
    tool->has_preview=1;
}

void slab_edge_rebate_tool_init(SlabEdgeRebateTool *tool)
{
    if (tool == NULL) { return; }
    *tool=(SlabEdgeRebateTool){
        .width_mm=SLAB_EDGE_REBATE_TOOL_DEFAULT_WIDTH_MM,
        .depth_mm=SLAB_EDGE_REBATE_TOOL_DEFAULT_DEPTH_MM
    };
}

void slab_edge_rebate_tool_cancel(SlabEdgeRebateTool *tool)
{
    if (tool == NULL) { return; }
    int width=tool->width_mm > 0 ? tool->width_mm :
        SLAB_EDGE_REBATE_TOOL_DEFAULT_WIDTH_MM;
    int depth=tool->depth_mm > 0 ? tool->depth_mm :
        SLAB_EDGE_REBATE_TOOL_DEFAULT_DEPTH_MM;
    *tool=(SlabEdgeRebateTool){.width_mm=width,.depth_mm=depth};
}

void slab_edge_rebate_tool_activate(SlabEdgeRebateTool *tool)
{
    if (tool == NULL) { return; }
    slab_edge_rebate_tool_cancel(tool);
    tool->active=1;
}
