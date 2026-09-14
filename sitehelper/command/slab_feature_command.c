#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "slab_feature_command.h"
#include "slab.h"

static int copy_vertices(const PlanPosition *vertices, size_t count,
    PlanPosition **output)
{
    if (vertices == NULL || count < 3 || count > SIZE_MAX / sizeof *vertices) {
        return 0;
    }
    PlanPosition *copy=malloc(count * sizeof *copy);
    if (copy == NULL) { return 0; }
    memcpy(copy,vertices,count * sizeof *copy);
    *output=copy;
    return 1;
}

int add_slab_penetration_command_create(DomainId slab_id,
    const PlanPosition *vertices, size_t count, AddSlabPenetrationCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    SlabPenetration borrowed={.outline={(PlanPosition *)vertices,count,count}};
    if (slab_penetration_validate(&borrowed) != SLAB_SUCCESS) { return 0; }
    AddSlabPenetrationCommand candidate={.slab_id=slab_id,.vertex_count=count};
    if (!copy_vertices(vertices,count,&candidate.vertices)) { return 0; }
    add_slab_penetration_command_destroy(output);
    *output=candidate;
    return 1;
}

int add_slab_penetration_command_clone(const AddSlabPenetrationCommand *source,
    AddSlabPenetrationCommand *output)
{
    return source != NULL && add_slab_penetration_command_create(source->slab_id,
        source->vertices,source->vertex_count,output);
}

void add_slab_penetration_command_destroy(AddSlabPenetrationCommand *command)
{
    if (command == NULL) { return; }
    free(command->vertices);
    *command=(AddSlabPenetrationCommand){0};
}

int add_slab_region_command_create(DomainId slab_id,
    const PlanPosition *vertices, size_t count, int top_level_offset_mm,
    int thickness_mm, AddSlabRegionCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    SlabRegion borrowed={.outline={(PlanPosition *)vertices,count,count},
        .top_level_offset_mm=top_level_offset_mm,.thickness_mm=thickness_mm};
    if (slab_region_validate(&borrowed) != SLAB_SUCCESS) { return 0; }
    AddSlabRegionCommand candidate={.slab_id=slab_id,.vertex_count=count,
        .top_level_offset_mm=top_level_offset_mm,.thickness_mm=thickness_mm};
    if (!copy_vertices(vertices,count,&candidate.vertices)) { return 0; }
    add_slab_region_command_destroy(output);
    *output=candidate;
    return 1;
}

int add_slab_region_command_clone(const AddSlabRegionCommand *source,
    AddSlabRegionCommand *output)
{
    return source != NULL && add_slab_region_command_create(source->slab_id,
        source->vertices,source->vertex_count,source->top_level_offset_mm,
        source->thickness_mm,output);
}

void add_slab_region_command_destroy(AddSlabRegionCommand *command)
{
    if (command == NULL) { return; }
    free(command->vertices);
    *command=(AddSlabRegionCommand){0};
}

int add_slab_edge_rebate_command_create(DomainId slab_id, size_t edge_index,
    int start_offset_mm, int end_offset_mm, int width_mm, int depth_mm,
    AddSlabEdgeRebateCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    *output=(AddSlabEdgeRebateCommand){slab_id,edge_index,start_offset_mm,
        end_offset_mm,width_mm,depth_mm};
    return 1;
}

int delete_slab_penetration_command_create(DomainId slab_id,
    size_t index, DeleteSlabPenetrationCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    *output=(DeleteSlabPenetrationCommand){slab_id,index}; return 1;
}

int delete_slab_region_command_create(DomainId slab_id,
    size_t index, DeleteSlabRegionCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    *output=(DeleteSlabRegionCommand){slab_id,index}; return 1;
}

int delete_slab_edge_rebate_command_create(DomainId slab_id,
    size_t index, DeleteSlabEdgeRebateCommand *output)
{
    if (slab_id == DOMAIN_ID_INVALID || output == NULL) { return 0; }
    *output=(DeleteSlabEdgeRebateCommand){slab_id,index}; return 1;
}

int add_slab_penetration_command_execute(SiteHelperProject *project,
    const AddSlabPenetrationCommand *command, size_t *index)
{
    if (project == NULL || command == NULL || index == NULL) { return 0; }
    Slab *slab=sitehelper_project_find_slab_by_id(project,command->slab_id);
    if (slab == NULL) { return 0; }
    size_t candidate=slab->definition.penetrations.count;
    if (slab_add_penetration(slab,command->vertices,command->vertex_count) != SLAB_SUCCESS) {
        return 0;
    }
    *index=candidate; return 1;
}

int add_slab_region_command_execute(SiteHelperProject *project,
    const AddSlabRegionCommand *command, size_t *index)
{
    if (project == NULL || command == NULL || index == NULL) { return 0; }
    Slab *slab=sitehelper_project_find_slab_by_id(project,command->slab_id);
    if (slab == NULL) { return 0; }
    size_t candidate=slab->definition.regions.count;
    if (slab_add_region(slab,command->vertices,command->vertex_count,
        command->top_level_offset_mm,command->thickness_mm) != SLAB_SUCCESS) { return 0; }
    *index=candidate; return 1;
}

int add_slab_edge_rebate_command_execute(SiteHelperProject *project,
    const AddSlabEdgeRebateCommand *command, size_t *index)
{
    if (project == NULL || command == NULL || index == NULL) { return 0; }
    Slab *slab=sitehelper_project_find_slab_by_id(project,command->slab_id);
    if (slab == NULL) { return 0; }
    size_t candidate=slab->definition.edge_rebates.count;
    if (slab_add_edge_rebate(slab,command->edge_index,command->start_offset_mm,
        command->end_offset_mm,command->width_mm,command->depth_mm) != SLAB_SUCCESS) {
        return 0;
    }
    *index=candidate; return 1;
}

int delete_slab_penetration_command_execute(SiteHelperProject *project,
    const DeleteSlabPenetrationCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    return slab != NULL && slab_remove_penetration(slab,command->feature_index) == SLAB_SUCCESS;
}

int delete_slab_region_command_execute(SiteHelperProject *project,
    const DeleteSlabRegionCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    return slab != NULL && slab_remove_region(slab,command->feature_index) == SLAB_SUCCESS;
}

int delete_slab_edge_rebate_command_execute(SiteHelperProject *project,
    const DeleteSlabEdgeRebateCommand *command)
{
    Slab *slab=project == NULL || command == NULL ? NULL :
        sitehelper_project_find_slab_by_id(project,command->slab_id);
    return slab != NULL && slab_remove_edge_rebate(slab,command->feature_index) == SLAB_SUCCESS;
}
