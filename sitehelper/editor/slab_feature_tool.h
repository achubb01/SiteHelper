#ifndef SLAB_FEATURE_TOOL_H
#define SLAB_FEATURE_TOOL_H

#include <stddef.h>

#include "domain_id.h"
#include "geometry.h"
#include "position.h"

enum {
    SLAB_EDGE_REBATE_TOOL_DEFAULT_WIDTH_MM = 100,
    SLAB_EDGE_REBATE_TOOL_DEFAULT_DEPTH_MM = 20
};

/* Shared editor-only polygon storage for penetration and region interaction.
 * It has no domain meaning and never enters the Project before a command runs. */
typedef struct {
    PlanPosition *vertices;
    size_t vertex_count;
    size_t vertex_capacity;
    DomainId storey_id;
    DomainId slab_id;
    PlanPoint preview;
    int has_preview;
    int active;
    int top_level_offset_mm;
    int thickness_mm;
} SlabPolygonFeatureTool;

typedef struct {
    DomainId storey_id;
    DomainId slab_id;
    size_t edge_index;
    int start_u_mm;
    int hover_u_mm;
    PlanPoint start_point;
    PlanPoint hover_point;
    int has_start;
    int has_hover;
    int active;
    int width_mm;
    int depth_mm;
} SlabEdgeRebateTool;

void slab_polygon_feature_tool_init(SlabPolygonFeatureTool *tool);
void slab_polygon_feature_tool_destroy(SlabPolygonFeatureTool *tool);
void slab_polygon_feature_tool_activate(SlabPolygonFeatureTool *tool);
void slab_polygon_feature_tool_cancel(SlabPolygonFeatureTool *tool);
int slab_polygon_feature_tool_begin(SlabPolygonFeatureTool *tool,
    DomainId storey_id, DomainId slab_id, PlanPosition vertex,
    int top_level_offset_mm, int thickness_mm);
int slab_polygon_feature_tool_append(SlabPolygonFeatureTool *tool,
    PlanPosition vertex);
void slab_polygon_feature_tool_update(SlabPolygonFeatureTool *tool,
    PlanPoint preview);

void slab_edge_rebate_tool_init(SlabEdgeRebateTool *tool);
void slab_edge_rebate_tool_activate(SlabEdgeRebateTool *tool);
void slab_edge_rebate_tool_cancel(SlabEdgeRebateTool *tool);

#endif
