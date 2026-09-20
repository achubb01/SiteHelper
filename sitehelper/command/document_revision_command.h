#ifndef DOCUMENT_REVISION_COMMAND_H
#define DOCUMENT_REVISION_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    char *identifier;
    char *description;
} CreateDocumentRevisionCommand;

typedef struct {
    DomainId revision_id;
    char *identifier;
    char *description;
} EditDocumentRevisionCommand;

typedef struct { DomainId revision_id; } DeleteDocumentRevisionCommand;

typedef struct {
    DomainId revision_cloud_id;
    DomainId revision_id;
} SetPlanRevisionCloudRevisionCommand;

int create_document_revision_command_create(const char *identifier,
    const char *description, CreateDocumentRevisionCommand *command);
int create_document_revision_command_clone(const CreateDocumentRevisionCommand *source,
    CreateDocumentRevisionCommand *output);
void create_document_revision_command_destroy(CreateDocumentRevisionCommand *command);
int create_document_revision_command_execute(SiteHelperProject *project,
    const CreateDocumentRevisionCommand *command, DomainId *revision_id);
int create_document_revision_command_redo(SiteHelperProject *project,
    const CreateDocumentRevisionCommand *command, DomainId revision_id);
int create_document_revision_command_undo(SiteHelperProject *project,
    const CreateDocumentRevisionCommand *command, DomainId revision_id);

int edit_document_revision_command_create(DomainId revision_id, const char *identifier,
    const char *description, EditDocumentRevisionCommand *command);
int edit_document_revision_command_clone(const EditDocumentRevisionCommand *source,
    EditDocumentRevisionCommand *output);
void edit_document_revision_command_destroy(EditDocumentRevisionCommand *command);
int edit_document_revision_command_execute(SiteHelperProject *project,
    const EditDocumentRevisionCommand *command);

int delete_document_revision_command_create(DomainId revision_id,
    DeleteDocumentRevisionCommand *command);
int delete_document_revision_command_execute(SiteHelperProject *project,
    const DeleteDocumentRevisionCommand *command);

int set_plan_revision_cloud_revision_command_create(DomainId revision_cloud_id,
    DomainId revision_id, SetPlanRevisionCloudRevisionCommand *command);
int set_plan_revision_cloud_revision_command_execute(SiteHelperProject *project,
    const SetPlanRevisionCloudRevisionCommand *command);

#endif
