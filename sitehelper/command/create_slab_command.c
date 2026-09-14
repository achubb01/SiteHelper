#include <stdlib.h>
#include "create_slab_command.h"
#include "slab.h"

int create_slab_command_create(DomainId storey_id, const PlanPosition *vertices,
    size_t count, int thickness_mm, int offset_mm, CreateSlabCommand *output)
{
    if (output == NULL || storey_id == DOMAIN_ID_INVALID) { return 0; }
    Slab validated = {0};
    if (slab_build(1, vertices, count, thickness_mm, offset_mm, &validated) != SLAB_SUCCESS) {
        return 0;
    }
    CreateSlabCommand candidate = {.storey_id=storey_id,
        .vertices=validated.definition.outline.vertices,.vertex_count=count,
        .thickness_mm=thickness_mm,.top_level_offset_mm=offset_mm};
    validated.definition.outline = (SlabOutline){0};
    slab_destroy(&validated);
    create_slab_command_destroy(output);
    *output=candidate;
    return 1;
}

int create_slab_command_clone(const CreateSlabCommand *source, CreateSlabCommand *output)
{
    return source != NULL && create_slab_command_create(source->storey_id,
        source->vertices,source->vertex_count,source->thickness_mm,
        source->top_level_offset_mm,output);
}

void create_slab_command_destroy(CreateSlabCommand *command)
{
    if (command == NULL) { return; }
    free(command->vertices);
    *command=(CreateSlabCommand){0};
}

int create_slab_command_execute(SiteHelperProject *project,
    const CreateSlabCommand *command, DomainId *slab_id)
{
    if (slab_id == NULL) { return 0; }
    *slab_id=DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL) { return 0; }
    *slab_id=sitehelper_project_add_slab(project,command->storey_id,
        command->vertices,command->vertex_count,command->thickness_mm,
        command->top_level_offset_mm);
    return *slab_id != DOMAIN_ID_INVALID;
}

int create_slab_command_undo(SiteHelperProject *project,
    const CreateSlabCommand *command, DomainId slab_id)
{
    if (project == NULL || command == NULL || slab_id == DOMAIN_ID_INVALID) { return 0; }
    const Storey *owner=sitehelper_project_find_owning_storey_const(project,slab_id);
    return owner != NULL && owner->id == command->storey_id &&
        sitehelper_project_remove_slab_by_id(project,slab_id);
}

int create_slab_command_redo(SiteHelperProject *project,
    const CreateSlabCommand *command, DomainId slab_id)
{
    if (project == NULL || command == NULL || slab_id == DOMAIN_ID_INVALID ||
        project->domain_ids.next == DOMAIN_ID_INVALID || slab_id >= project->domain_ids.next ||
        sitehelper_project_contains_domain_id(project,slab_id)) { return 0; }
    Slab slab={0};
    if (slab_build(slab_id,command->vertices,command->vertex_count,
        command->thickness_mm,command->top_level_offset_mm,&slab) != SLAB_SUCCESS) { return 0; }
    int ok=sitehelper_project_insert_slab(project,command->storey_id,&slab);
    slab_destroy(&slab);
    return ok;
}
