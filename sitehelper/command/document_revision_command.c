#include "document_revision_command.h"

#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *text)
{
    if (text == NULL) { return NULL; }
    size_t length = strlen(text);
    char *copy = malloc(length + 1);
    if (copy == NULL) { return NULL; }
    memcpy(copy, text, length + 1);
    return copy;
}

static int revision_matches(const DocumentRevision *revision, DomainId id,
    const char *identifier, const char *description)
{
    return revision != NULL && revision->id == id && identifier != NULL &&
        description != NULL && strcmp(revision->identifier, identifier) == 0 &&
        strcmp(revision->description, description) == 0;
}

int create_document_revision_command_create(const char *identifier,
    const char *description, CreateDocumentRevisionCommand *command)
{
    if (command == NULL || identifier == NULL || identifier[0] == '\0' ||
        description == NULL) { return 0; }
    char *identifier_copy = copy_string(identifier);
    char *description_copy = copy_string(description);
    if (identifier_copy == NULL || description_copy == NULL) {
        free(identifier_copy); free(description_copy); return 0;
    }
    create_document_revision_command_destroy(command);
    *command = (CreateDocumentRevisionCommand){identifier_copy, description_copy};
    return 1;
}

int create_document_revision_command_clone(const CreateDocumentRevisionCommand *source,
    CreateDocumentRevisionCommand *output)
{
    return source != NULL && output != NULL && source != output &&
        create_document_revision_command_create(source->identifier, source->description, output);
}

void create_document_revision_command_destroy(CreateDocumentRevisionCommand *command)
{
    if (command == NULL) { return; }
    free(command->identifier); free(command->description);
    *command = (CreateDocumentRevisionCommand){0};
}

int create_document_revision_command_execute(SiteHelperProject *project,
    const CreateDocumentRevisionCommand *command, DomainId *revision_id)
{
    if (revision_id == NULL) { return 0; }
    *revision_id = DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL) { return 0; }
    DomainId id = sitehelper_project_add_revision(project, command->identifier,
        command->description);
    if (id == DOMAIN_ID_INVALID) { return 0; }
    *revision_id = id;
    return 1;
}

int create_document_revision_command_redo(SiteHelperProject *project,
    const CreateDocumentRevisionCommand *command, DomainId revision_id)
{
    if (project == NULL || command == NULL || revision_id == DOMAIN_ID_INVALID) { return 0; }
    DocumentRevision revision = {.id=revision_id, .identifier=command->identifier,
        .description=command->description};
    return sitehelper_project_insert_revision(project, &revision);
}

int create_document_revision_command_undo(SiteHelperProject *project,
    const CreateDocumentRevisionCommand *command, DomainId revision_id)
{
    const DocumentRevision *revision = sitehelper_project_find_revision_by_id_const(
        project, revision_id);
    return command != NULL && revision_matches(revision, revision_id,
        command->identifier, command->description) &&
        sitehelper_project_remove_revision_by_id(project, revision_id);
}

int edit_document_revision_command_create(DomainId revision_id, const char *identifier,
    const char *description, EditDocumentRevisionCommand *command)
{
    if (command == NULL || revision_id == DOMAIN_ID_INVALID || identifier == NULL ||
        identifier[0] == '\0' || description == NULL) { return 0; }
    char *identifier_copy = copy_string(identifier);
    char *description_copy = copy_string(description);
    if (identifier_copy == NULL || description_copy == NULL) {
        free(identifier_copy); free(description_copy); return 0;
    }
    edit_document_revision_command_destroy(command);
    *command = (EditDocumentRevisionCommand){revision_id, identifier_copy, description_copy};
    return 1;
}

int edit_document_revision_command_clone(const EditDocumentRevisionCommand *source,
    EditDocumentRevisionCommand *output)
{
    return source != NULL && output != NULL && source != output &&
        edit_document_revision_command_create(source->revision_id, source->identifier,
            source->description, output);
}

void edit_document_revision_command_destroy(EditDocumentRevisionCommand *command)
{
    if (command == NULL) { return; }
    free(command->identifier); free(command->description);
    *command = (EditDocumentRevisionCommand){0};
}

int edit_document_revision_command_execute(SiteHelperProject *project,
    const EditDocumentRevisionCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_update_revision(project, command->revision_id,
            command->identifier, command->description);
}

int delete_document_revision_command_create(DomainId revision_id,
    DeleteDocumentRevisionCommand *command)
{
    if (command == NULL || revision_id == DOMAIN_ID_INVALID) { return 0; }
    *command = (DeleteDocumentRevisionCommand){revision_id};
    return 1;
}

int delete_document_revision_command_execute(SiteHelperProject *project,
    const DeleteDocumentRevisionCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_revision_by_id(project, command->revision_id);
}

int set_plan_revision_cloud_revision_command_create(DomainId revision_cloud_id,
    DomainId revision_id, SetPlanRevisionCloudRevisionCommand *command)
{
    if (command == NULL || revision_cloud_id == DOMAIN_ID_INVALID) { return 0; }
    *command = (SetPlanRevisionCloudRevisionCommand){revision_cloud_id, revision_id};
    return 1;
}

int set_plan_revision_cloud_revision_command_execute(SiteHelperProject *project,
    const SetPlanRevisionCloudRevisionCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_set_revision_cloud_revision(project,
            command->revision_cloud_id, command->revision_id);
}
