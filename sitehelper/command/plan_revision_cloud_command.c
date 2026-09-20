#include "plan_revision_cloud_command.h"

#include <stdlib.h>
#include <string.h>

static int copy_vertices(const PlanPosition *vertices, size_t vertex_count,
    PlanPosition **output)
{
    if (output == NULL) { return 0; }
    *output = NULL;
    if (vertices == NULL || vertex_count < 3 ||
        vertex_count > SIZE_MAX / sizeof *vertices) { return 0; }
    PlanPosition *copy = malloc(vertex_count * sizeof *copy);
    if (copy == NULL) { return 0; }
    memcpy(copy, vertices, vertex_count * sizeof *copy);
    *output = copy;
    return 1;
}

static int cloud_matches(const DocumentPlanRevisionCloud *cloud, DomainId id,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count)
{
    return cloud != NULL && cloud->id == id && cloud->storey_id == storey_id &&
        cloud->vertex_count == vertex_count && vertices != NULL &&
        (vertex_count == 0 || memcmp(cloud->vertices, vertices,
            vertex_count * sizeof *vertices) == 0);
}

int create_plan_revision_cloud_command_create(DomainId storey_id,
    const PlanPosition *vertices, size_t vertex_count,
    CreatePlanRevisionCloudCommand *command)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID) { return 0; }
    DocumentPlanRevisionCloud candidate = {
        .id = 1, .storey_id = storey_id,
        .vertices = (PlanPosition *)vertices, .vertex_count = vertex_count
    };
    if (!document_plan_revision_cloud_is_locally_valid(&candidate)) { return 0; }
    PlanPosition *owned = NULL;
    if (!copy_vertices(vertices, vertex_count, &owned)) { return 0; }
    create_plan_revision_cloud_command_destroy(command);
    *command = (CreatePlanRevisionCloudCommand){storey_id, owned, vertex_count};
    return 1;
}

int create_plan_revision_cloud_command_clone(
    const CreatePlanRevisionCloudCommand *source,
    CreatePlanRevisionCloudCommand *output)
{
    if (source == NULL || output == NULL || source == output) { return 0; }
    CreatePlanRevisionCloudCommand candidate = {0};
    if (!create_plan_revision_cloud_command_create(source->storey_id,
            source->vertices, source->vertex_count, &candidate)) { return 0; }
    create_plan_revision_cloud_command_destroy(output);
    *output = candidate;
    return 1;
}

void create_plan_revision_cloud_command_destroy(CreatePlanRevisionCloudCommand *command)
{
    if (command == NULL) { return; }
    free(command->vertices);
    *command = (CreatePlanRevisionCloudCommand){0};
}

int create_plan_revision_cloud_command_execute(SiteHelperProject *project,
    const CreatePlanRevisionCloudCommand *command, DomainId *revision_cloud_id)
{
    if (revision_cloud_id == NULL) { return 0; }
    *revision_cloud_id = DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL) { return 0; }
    DomainId id = sitehelper_project_add_plan_revision_cloud(project,
        command->storey_id, command->vertices, command->vertex_count);
    if (id == DOMAIN_ID_INVALID) { return 0; }
    *revision_cloud_id = id;
    return 1;
}

int create_plan_revision_cloud_command_redo(SiteHelperProject *project,
    const CreatePlanRevisionCloudCommand *command, DomainId revision_cloud_id)
{
    if (project == NULL || command == NULL || revision_cloud_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    DocumentPlanRevisionCloud cloud = {
        .id = revision_cloud_id, .storey_id = command->storey_id,
        .vertices = command->vertices, .vertex_count = command->vertex_count
    };
    return sitehelper_project_insert_revision_cloud(project, &cloud);
}

int create_plan_revision_cloud_command_undo(SiteHelperProject *project,
    const CreatePlanRevisionCloudCommand *command, DomainId revision_cloud_id)
{
    const DocumentPlanRevisionCloud *cloud =
        sitehelper_project_find_revision_cloud_by_id_const(project, revision_cloud_id);
    return command != NULL &&
        cloud_matches(cloud, revision_cloud_id, command->storey_id,
            command->vertices, command->vertex_count) &&
        sitehelper_project_remove_revision_cloud_by_id(project, revision_cloud_id);
}

int edit_plan_revision_cloud_command_create(DomainId revision_cloud_id,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count,
    EditPlanRevisionCloudCommand *command)
{
    if (command == NULL || revision_cloud_id == DOMAIN_ID_INVALID ||
        storey_id == DOMAIN_ID_INVALID) { return 0; }
    DocumentPlanRevisionCloud candidate = {
        .id = revision_cloud_id, .storey_id = storey_id,
        .vertices = (PlanPosition *)vertices, .vertex_count = vertex_count
    };
    if (!document_plan_revision_cloud_is_locally_valid(&candidate)) { return 0; }
    PlanPosition *owned = NULL;
    if (!copy_vertices(vertices, vertex_count, &owned)) { return 0; }
    edit_plan_revision_cloud_command_destroy(command);
    *command = (EditPlanRevisionCloudCommand){revision_cloud_id, storey_id,
        owned, vertex_count};
    return 1;
}

int edit_plan_revision_cloud_command_clone(const EditPlanRevisionCloudCommand *source,
    EditPlanRevisionCloudCommand *output)
{
    if (source == NULL || output == NULL || source == output) { return 0; }
    EditPlanRevisionCloudCommand candidate = {0};
    if (!edit_plan_revision_cloud_command_create(source->revision_cloud_id,
            source->storey_id, source->vertices, source->vertex_count, &candidate)) {
        return 0;
    }
    edit_plan_revision_cloud_command_destroy(output);
    *output = candidate;
    return 1;
}

void edit_plan_revision_cloud_command_destroy(EditPlanRevisionCloudCommand *command)
{
    if (command == NULL) { return; }
    free(command->vertices);
    *command = (EditPlanRevisionCloudCommand){0};
}

int edit_plan_revision_cloud_command_execute(SiteHelperProject *project,
    const EditPlanRevisionCloudCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_update_plan_revision_cloud(project,
            command->revision_cloud_id, command->storey_id,
            command->vertices, command->vertex_count);
}

int delete_plan_revision_cloud_command_create(DomainId revision_cloud_id,
    DeletePlanRevisionCloudCommand *command)
{
    if (command == NULL || revision_cloud_id == DOMAIN_ID_INVALID) { return 0; }
    *command = (DeletePlanRevisionCloudCommand){revision_cloud_id};
    return 1;
}

int delete_plan_revision_cloud_command_execute(SiteHelperProject *project,
    const DeletePlanRevisionCloudCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_revision_cloud_by_id(project,
            command->revision_cloud_id);
}
