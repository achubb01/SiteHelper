#include "slab_geometry_tool.h"

void slab_geometry_tool_init(SlabGeometryTool *tool)
{
    if (tool != NULL) { *tool=(SlabGeometryTool){0}; }
}

void slab_geometry_tool_cancel(SlabGeometryTool *tool)
{
    if (tool == NULL) { return; }
    *tool=(SlabGeometryTool){0};
}

void slab_geometry_tool_activate(SlabGeometryTool *tool)
{
    if (tool == NULL) { return; }
    slab_geometry_tool_cancel(tool);
    tool->active=1;
}

int slab_geometry_tool_begin(SlabGeometryTool *tool, DomainId storey_id,
    DomainId slab_id, EditorSlabGeometryKind kind, size_t feature_index,
    size_t vertex_index, PlanPosition original_position)
{
    if (tool == NULL || !tool->active || tool->has_vertex ||
        storey_id == DOMAIN_ID_INVALID || slab_id == DOMAIN_ID_INVALID ||
        kind <= EDITOR_SLAB_GEOMETRY_NONE || kind > EDITOR_SLAB_GEOMETRY_REGION ||
        vertex_index == SIZE_MAX ||
        (kind == EDITOR_SLAB_GEOMETRY_OUTLINE ? feature_index != SIZE_MAX : feature_index == SIZE_MAX)) {
        return 0;
    }
    tool->storey_id=storey_id;
    tool->slab_id=slab_id;
    tool->kind=kind;
    tool->feature_index=feature_index;
    tool->vertex_index=vertex_index;
    tool->original_position=original_position;
    tool->preview=(PlanPoint){original_position.x,original_position.y};
    tool->has_preview=1;
    tool->has_vertex=1;
    return 1;
}

void slab_geometry_tool_update(SlabGeometryTool *tool, PlanPoint preview)
{
    if (tool == NULL || !tool->active || !tool->has_vertex) { return; }
    tool->preview=preview;
    tool->has_preview=1;
}
