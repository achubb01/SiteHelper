#ifndef PLAN_SYMBOL_COMMAND_H
#define PLAN_SYMBOL_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    DocumentPlanSymbolKind kind;
    PlanPosition anchor;
    DocumentPlanDirection direction;
} CreatePlanSymbolCommand;

typedef struct {
    DomainId symbol_id;
    DomainId storey_id;
    DocumentPlanSymbolKind kind;
    PlanPosition anchor;
    DocumentPlanDirection direction;
} EditPlanSymbolCommand;

typedef struct { DomainId symbol_id; } DeletePlanSymbolCommand;

int create_plan_symbol_command_create(DomainId storey_id,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    CreatePlanSymbolCommand *command);
int create_plan_symbol_command_execute(SiteHelperProject *project,
    const CreatePlanSymbolCommand *command, DomainId *symbol_id);
int create_plan_symbol_command_redo(SiteHelperProject *project,
    const CreatePlanSymbolCommand *command, DomainId symbol_id);
int create_plan_symbol_command_undo(SiteHelperProject *project,
    const CreatePlanSymbolCommand *command, DomainId symbol_id);

int edit_plan_symbol_command_create(DomainId symbol_id, DomainId storey_id,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    EditPlanSymbolCommand *command);
int edit_plan_symbol_command_execute(SiteHelperProject *project,
    const EditPlanSymbolCommand *command);

int delete_plan_symbol_command_create(DomainId symbol_id, DeletePlanSymbolCommand *command);
int delete_plan_symbol_command_execute(SiteHelperProject *project,
    const DeletePlanSymbolCommand *command);

#endif
