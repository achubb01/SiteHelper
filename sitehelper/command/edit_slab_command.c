#include "edit_slab_command.h"
#include "slab.h"

int edit_slab_command_create(DomainId slab_id, int thickness_mm,
    int top_level_offset_mm, EditSlabCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    *output=(EditSlabCommand){slab_id,thickness_mm,top_level_offset_mm};
    return 1;
}

int edit_slab_region_command_create(DomainId slab_id, size_t feature_index,
    int top_level_offset_mm, int thickness_mm, EditSlabRegionCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || feature_index == SIZE_MAX || output == NULL) { return 0; }
    *output=(EditSlabRegionCommand){slab_id,feature_index,top_level_offset_mm,thickness_mm};
    return 1;
}

int edit_slab_edge_rebate_command_create(DomainId slab_id, size_t feature_index,
    size_t edge_index, int start_offset_mm, int end_offset_mm, int width_mm,
    int depth_mm, EditSlabEdgeRebateCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || feature_index == SIZE_MAX ||
        edge_index == SIZE_MAX || output == NULL) { return 0; }
    *output=(EditSlabEdgeRebateCommand){slab_id,feature_index,edge_index,
        start_offset_mm,end_offset_mm,width_mm,depth_mm};
    return 1;
}

int edit_slab_command_execute(SiteHelperProject *project,
    const EditSlabCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    return slab != NULL && slab_set_base_properties(slab,command->thickness_mm,
        command->top_level_offset_mm) == SLAB_SUCCESS;
}

int edit_slab_region_command_execute(SiteHelperProject *project,
    const EditSlabRegionCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    return slab != NULL && slab_set_region_properties(slab,command->feature_index,
        command->top_level_offset_mm,command->thickness_mm) == SLAB_SUCCESS;
}

int edit_slab_edge_rebate_command_execute(SiteHelperProject *project,
    const EditSlabEdgeRebateCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    const SlabEdgeRebate *rebate=slab == NULL ? NULL :
        slab_edge_rebate_at(slab,command->feature_index);
    if (rebate == NULL || rebate->edge_index != command->edge_index) { return 0; }
    return slab_set_edge_rebate_properties(slab,command->feature_index,
        command->start_offset_mm,command->end_offset_mm,command->width_mm,
        command->depth_mm) == SLAB_SUCCESS;
}
