#include "plan_dimension_command.h"

static int refs_equal(DocumentDimensionReference a, DocumentDimensionReference b)
{
    return a.kind == b.kind && a.target_id == b.target_id &&
        a.position.x == b.position.x && a.position.y == b.position.y;
}

static int matches_create(const DocumentPlanDimension *dimension,
    const CreatePlanDimensionCommand *command, DomainId id)
{
    return dimension != NULL && command != NULL && dimension->id == id &&
        dimension->storey_id == command->storey_id &&
        refs_equal(dimension->first, command->first) &&
        refs_equal(dimension->second, command->second) &&
        dimension->offset_mm == command->offset_mm;
}

int create_plan_dimension_command_create(DomainId storey_id,
    DocumentDimensionReference first, DocumentDimensionReference second,
    int offset_mm, CreatePlanDimensionCommand *command)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID ||
        !document_dimension_reference_is_locally_valid(&first) ||
        !document_dimension_reference_is_locally_valid(&second)) { return 0; }
    *command = (CreatePlanDimensionCommand){storey_id, first, second, offset_mm};
    return 1;
}

int create_plan_dimension_command_execute(SiteHelperProject *project,
    const CreatePlanDimensionCommand *command, DomainId *dimension_id)
{
    if (project == NULL || command == NULL || dimension_id == NULL) { return 0; }
    *dimension_id = sitehelper_project_add_plan_dimension(project, command->storey_id,
        command->first, command->second, command->offset_mm);
    return *dimension_id != DOMAIN_ID_INVALID;
}

int create_plan_dimension_command_redo(SiteHelperProject *project,
    const CreatePlanDimensionCommand *command, DomainId dimension_id)
{
    if (project == NULL || command == NULL || dimension_id == DOMAIN_ID_INVALID) { return 0; }
    DocumentPlanDimension dimension = {dimension_id, command->storey_id,
        command->first, command->second, command->offset_mm};
    return sitehelper_project_insert_dimension(project, &dimension);
}

int create_plan_dimension_command_undo(SiteHelperProject *project,
    const CreatePlanDimensionCommand *command, DomainId dimension_id)
{
    return matches_create(sitehelper_project_find_dimension_by_id_const(project, dimension_id),
        command, dimension_id) && sitehelper_project_remove_dimension_by_id(project, dimension_id);
}

int edit_plan_dimension_command_create(DomainId dimension_id, DomainId storey_id,
    DocumentDimensionReference first, DocumentDimensionReference second,
    int offset_mm, EditPlanDimensionCommand *command)
{
    if (command == NULL || dimension_id == DOMAIN_ID_INVALID ||
        storey_id == DOMAIN_ID_INVALID ||
        !document_dimension_reference_is_locally_valid(&first) ||
        !document_dimension_reference_is_locally_valid(&second)) { return 0; }
    *command = (EditPlanDimensionCommand){dimension_id, storey_id, first, second, offset_mm};
    return 1;
}

int edit_plan_dimension_command_execute(SiteHelperProject *project,
    const EditPlanDimensionCommand *command)
{
    if (project == NULL || command == NULL) { return 0; }
    return sitehelper_project_update_plan_dimension(project, command->dimension_id,
        command->storey_id, command->first, command->second, command->offset_mm);
}

int delete_plan_dimension_command_create(DomainId dimension_id,
    DeletePlanDimensionCommand *command)
{
    if (command == NULL || dimension_id == DOMAIN_ID_INVALID) { return 0; }
    *command = (DeletePlanDimensionCommand){dimension_id};
    return 1;
}

int delete_plan_dimension_command_execute(SiteHelperProject *project,
    const DeletePlanDimensionCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_dimension_by_id(project, command->dimension_id);
}
