#include "delete_roof_command.h"
#include "delete_roof_command_internal.h"
#include "roof.h"

int delete_roof_command_create(DomainId roof_id, DeleteRoofCommand *command)
{
    if (command == NULL || roof_id == DOMAIN_ID_INVALID) { return 0; }
    *command = (DeleteRoofCommand){.roof_id = roof_id};
    return 1;
}

int delete_roof_command_execute(SiteHelperProject *project, const DeleteRoofCommand *command)
{
    return project != NULL && command != NULL && command->roof_id != DOMAIN_ID_INVALID &&
        sitehelper_project_remove_roof_by_id(project, command->roof_id);
}

void deleted_roof_snapshot_destroy(DeletedRoofSnapshot *snapshot)
{
    if (snapshot == NULL) { return; }
    roof_destroy(&snapshot->roof);
    *snapshot = (DeletedRoofSnapshot){0};
}

int deleted_roof_snapshot_capture(const SiteHelperProject *project,
    const DeleteRoofCommand *command, DeletedRoofSnapshot *snapshot)
{
    if (project == NULL || command == NULL || snapshot == NULL) { return 0; }
    const Storey *storey = sitehelper_project_find_owning_storey_const(project, command->roof_id);
    if (storey == NULL) { return 0; }
    size_t index = storey->roofs.count;
    for (size_t i = 0; i < storey->roofs.count; i++) {
        if (storey->roofs.items[i].id == command->roof_id) { index = i; break; }
    }
    if (index == storey->roofs.count) { return 0; }
    DeletedRoofSnapshot candidate = {.storey_id = storey->id, .index = index};
    if (roof_clone(&storey->roofs.items[index], &candidate.roof) != ROOF_SUCCESS) {
        return 0;
    }
    *snapshot = candidate;
    return 1;
}

int deleted_roof_snapshot_restore(SiteHelperProject *project,
    const DeletedRoofSnapshot *snapshot)
{
    if (project == NULL || snapshot == NULL || snapshot->roof.id == DOMAIN_ID_INVALID ||
        project->domain_ids.next == DOMAIN_ID_INVALID || snapshot->roof.id >= project->domain_ids.next ||
        sitehelper_project_contains_domain_id(project, snapshot->roof.id)) { return 0; }
    for (size_t i = 0; i < snapshot->roof.definition.portion_count; i++) {
        DomainId id = snapshot->roof.definition.portions[i].id;
        if (id == DOMAIN_ID_INVALID || id >= project->domain_ids.next ||
            sitehelper_project_contains_domain_id(project, id)) { return 0; }
    }
    return sitehelper_project_insert_roof_at(project, snapshot->storey_id,
        &snapshot->roof, snapshot->index);
}
