#ifndef SLAB_TOOL_H
#define SLAB_TOOL_H

#include "position.h"
#include "geometry.h"
#include "domain_id.h"

enum { SLAB_TOOL_DEFAULT_THICKNESS_MM = 100, SLAB_TOOL_DEFAULT_TOP_LEVEL_OFFSET_MM = 0 };

typedef struct {
    PlanPosition *vertices;
    size_t vertex_count, vertex_capacity;
    DomainId storey_id;
    PlanPoint preview;
    int has_preview;
    int active;
    int thickness_mm;
    int top_level_offset_mm;
} SlabTool;

void slab_tool_init(SlabTool *tool);
void slab_tool_destroy(SlabTool *tool);
void slab_tool_activate(SlabTool *tool);
void slab_tool_cancel(SlabTool *tool);
int slab_tool_append(SlabTool *tool, DomainId storey_id, PlanPosition vertex);
void slab_tool_update(SlabTool *tool, PlanPoint preview);

#endif
