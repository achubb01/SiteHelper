#ifndef PLAN_CALLOUT_COMMAND_H
#define PLAN_CALLOUT_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    PlanPosition target;
    PlanPosition label_anchor;
    char *text;
} CreatePlanCalloutCommand;

typedef struct {
    DomainId callout_id;
    DomainId storey_id;
    PlanPosition target;
    PlanPosition label_anchor;
    char *text;
} EditPlanCalloutCommand;

typedef struct { DomainId callout_id; } DeletePlanCalloutCommand;

int create_plan_callout_command_create(DomainId storey_id, PlanPosition target,
    PlanPosition label_anchor, const char *text, CreatePlanCalloutCommand *command);
int create_plan_callout_command_clone(const CreatePlanCalloutCommand *source,
    CreatePlanCalloutCommand *output);
void create_plan_callout_command_destroy(CreatePlanCalloutCommand *command);
int create_plan_callout_command_execute(SiteHelperProject *project,
    const CreatePlanCalloutCommand *command, DomainId *callout_id);
int create_plan_callout_command_redo(SiteHelperProject *project,
    const CreatePlanCalloutCommand *command, DomainId callout_id);
int create_plan_callout_command_undo(SiteHelperProject *project,
    const CreatePlanCalloutCommand *command, DomainId callout_id);

int edit_plan_callout_command_create(DomainId callout_id, DomainId storey_id,
    PlanPosition target, PlanPosition label_anchor, const char *text,
    EditPlanCalloutCommand *command);
int edit_plan_callout_command_clone(const EditPlanCalloutCommand *source,
    EditPlanCalloutCommand *output);
void edit_plan_callout_command_destroy(EditPlanCalloutCommand *command);
int edit_plan_callout_command_execute(SiteHelperProject *project,
    const EditPlanCalloutCommand *command);

int delete_plan_callout_command_create(DomainId callout_id, DeletePlanCalloutCommand *command);
int delete_plan_callout_command_execute(SiteHelperProject *project,
    const DeletePlanCalloutCommand *command);

#endif
