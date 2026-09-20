#ifndef PLAN_DIMENSION_COMMAND_H
#define PLAN_DIMENSION_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    DocumentDimensionReference first;
    DocumentDimensionReference second;
    int offset_mm;
} CreatePlanDimensionCommand;

typedef struct {
    DomainId dimension_id;
    DomainId storey_id;
    DocumentDimensionReference first;
    DocumentDimensionReference second;
    int offset_mm;
} EditPlanDimensionCommand;

typedef struct { DomainId dimension_id; } DeletePlanDimensionCommand;

int create_plan_dimension_command_create(DomainId storey_id,
    DocumentDimensionReference first, DocumentDimensionReference second,
    int offset_mm, CreatePlanDimensionCommand *command);
int create_plan_dimension_command_execute(SiteHelperProject *project,
    const CreatePlanDimensionCommand *command, DomainId *dimension_id);
int create_plan_dimension_command_redo(SiteHelperProject *project,
    const CreatePlanDimensionCommand *command, DomainId dimension_id);
int create_plan_dimension_command_undo(SiteHelperProject *project,
    const CreatePlanDimensionCommand *command, DomainId dimension_id);

int edit_plan_dimension_command_create(DomainId dimension_id, DomainId storey_id,
    DocumentDimensionReference first, DocumentDimensionReference second,
    int offset_mm, EditPlanDimensionCommand *command);
int edit_plan_dimension_command_execute(SiteHelperProject *project,
    const EditPlanDimensionCommand *command);

int delete_plan_dimension_command_create(DomainId dimension_id,
    DeletePlanDimensionCommand *command);
int delete_plan_dimension_command_execute(SiteHelperProject *project,
    const DeletePlanDimensionCommand *command);

#endif
