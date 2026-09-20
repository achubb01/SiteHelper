#ifndef PLAN_REVISION_CLOUD_COMMAND_H
#define PLAN_REVISION_CLOUD_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    PlanPosition *vertices;
    size_t vertex_count;
} CreatePlanRevisionCloudCommand;

typedef struct {
    DomainId revision_cloud_id;
    DomainId storey_id;
    PlanPosition *vertices;
    size_t vertex_count;
} EditPlanRevisionCloudCommand;

typedef struct { DomainId revision_cloud_id; } DeletePlanRevisionCloudCommand;

int create_plan_revision_cloud_command_create(DomainId storey_id,
    const PlanPosition *vertices, size_t vertex_count,
    CreatePlanRevisionCloudCommand *command);
int create_plan_revision_cloud_command_clone(
    const CreatePlanRevisionCloudCommand *source,
    CreatePlanRevisionCloudCommand *output);
void create_plan_revision_cloud_command_destroy(CreatePlanRevisionCloudCommand *command);
int create_plan_revision_cloud_command_execute(SiteHelperProject *project,
    const CreatePlanRevisionCloudCommand *command, DomainId *revision_cloud_id);
int create_plan_revision_cloud_command_redo(SiteHelperProject *project,
    const CreatePlanRevisionCloudCommand *command, DomainId revision_cloud_id);
int create_plan_revision_cloud_command_undo(SiteHelperProject *project,
    const CreatePlanRevisionCloudCommand *command, DomainId revision_cloud_id);

int edit_plan_revision_cloud_command_create(DomainId revision_cloud_id,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count,
    EditPlanRevisionCloudCommand *command);
int edit_plan_revision_cloud_command_clone(const EditPlanRevisionCloudCommand *source,
    EditPlanRevisionCloudCommand *output);
void edit_plan_revision_cloud_command_destroy(EditPlanRevisionCloudCommand *command);
int edit_plan_revision_cloud_command_execute(SiteHelperProject *project,
    const EditPlanRevisionCloudCommand *command);

int delete_plan_revision_cloud_command_create(DomainId revision_cloud_id,
    DeletePlanRevisionCloudCommand *command);
int delete_plan_revision_cloud_command_execute(SiteHelperProject *project,
    const DeletePlanRevisionCloudCommand *command);

#endif
