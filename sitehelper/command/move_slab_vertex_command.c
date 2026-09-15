#include "move_slab_vertex_command.h"
#include "slab.h"

int move_slab_vertex_command_create(DomainId slab_id, MoveSlabVertexTarget target,
    size_t feature_index, size_t vertex_index, PlanPosition new_position,
    MoveSlabVertexCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL ||
        target < MOVE_SLAB_VERTEX_OUTLINE || target >= MOVE_SLAB_VERTEX_TARGET_COUNT ||
        vertex_index == SIZE_MAX ||
        (target == MOVE_SLAB_VERTEX_OUTLINE ? feature_index != SIZE_MAX : feature_index == SIZE_MAX)) {
        return 0;
    }
    *output=(MoveSlabVertexCommand){slab_id,target,feature_index,vertex_index,new_position};
    return 1;
}

int move_slab_vertex_command_execute(SiteHelperProject *project,
    const MoveSlabVertexCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    if (slab == NULL) { return 0; }
    SlabCode code;
    switch (command->target) {
        case MOVE_SLAB_VERTEX_OUTLINE:
            if (command->feature_index != SIZE_MAX) { return 0; }
            code=slab_set_outline_vertex(slab,command->vertex_index,command->new_position);
            break;
        case MOVE_SLAB_VERTEX_PENETRATION:
            if (command->feature_index == SIZE_MAX) { return 0; }
            code=slab_set_penetration_vertex(slab,command->feature_index,
                command->vertex_index,command->new_position);
            break;
        case MOVE_SLAB_VERTEX_REGION:
            if (command->feature_index == SIZE_MAX) { return 0; }
            code=slab_set_region_vertex(slab,command->feature_index,
                command->vertex_index,command->new_position);
            break;
        case MOVE_SLAB_VERTEX_TARGET_COUNT:
        default:
            return 0;
    }
    return code == SLAB_SUCCESS;
}
