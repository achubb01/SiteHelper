#ifndef SLAB_GEOMETRY_TOOL_H
#define SLAB_GEOMETRY_TOOL_H

#include <stddef.h>

#include "domain_id.h"
#include "geometry.h"
#include "position.h"

typedef enum {
    EDITOR_SLAB_GEOMETRY_NONE = 0,
    EDITOR_SLAB_GEOMETRY_OUTLINE,
    EDITOR_SLAB_GEOMETRY_PENETRATION,
    EDITOR_SLAB_GEOMETRY_REGION
} EditorSlabGeometryKind;

typedef struct {
    int active;
    int has_vertex;
    int has_preview;
    DomainId storey_id;
    DomainId slab_id;
    EditorSlabGeometryKind kind;
    size_t feature_index;
    size_t vertex_index;
    PlanPosition original_position;
    PlanPoint preview;
} SlabGeometryTool;

void slab_geometry_tool_init(SlabGeometryTool *tool);
void slab_geometry_tool_activate(SlabGeometryTool *tool);
void slab_geometry_tool_cancel(SlabGeometryTool *tool);
int slab_geometry_tool_begin(SlabGeometryTool *tool, DomainId storey_id,
    DomainId slab_id, EditorSlabGeometryKind kind, size_t feature_index,
    size_t vertex_index, PlanPosition original_position);
void slab_geometry_tool_update(SlabGeometryTool *tool, PlanPoint preview);

#endif
