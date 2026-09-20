#include "plan_symbol_command.h"

static int matches_create(const DocumentPlanSymbol *symbol,
    const CreatePlanSymbolCommand *command, DomainId id)
{
    return symbol != NULL && command != NULL && symbol->id == id &&
        symbol->storey_id == command->storey_id && symbol->kind == command->kind &&
        symbol->anchor.x == command->anchor.x && symbol->anchor.y == command->anchor.y &&
        symbol->direction.dx == command->direction.dx &&
        symbol->direction.dy == command->direction.dy;
}

int create_plan_symbol_command_create(DomainId storey_id,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    CreatePlanSymbolCommand *command)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID) { return 0; }
    DocumentPlanSymbol candidate={.id=1,.storey_id=storey_id,.kind=kind,.anchor=anchor,
        .direction=direction};
    if (!document_plan_symbol_is_locally_valid(&candidate)) { return 0; }
    *command=(CreatePlanSymbolCommand){storey_id,kind,anchor,direction};
    return 1;
}

int create_plan_symbol_command_execute(SiteHelperProject *project,
    const CreatePlanSymbolCommand *command, DomainId *symbol_id)
{
    if (project == NULL || command == NULL || symbol_id == NULL) { return 0; }
    *symbol_id=sitehelper_project_add_plan_symbol(project,command->storey_id,
        command->kind,command->anchor,command->direction);
    return *symbol_id != DOMAIN_ID_INVALID;
}

int create_plan_symbol_command_redo(SiteHelperProject *project,
    const CreatePlanSymbolCommand *command, DomainId symbol_id)
{
    if (project == NULL || command == NULL || symbol_id == DOMAIN_ID_INVALID) { return 0; }
    DocumentPlanSymbol symbol={symbol_id,command->storey_id,command->kind,command->anchor,
        command->direction};
    return sitehelper_project_insert_symbol(project,&symbol);
}

int create_plan_symbol_command_undo(SiteHelperProject *project,
    const CreatePlanSymbolCommand *command, DomainId symbol_id)
{
    return matches_create(sitehelper_project_find_symbol_by_id_const(project,symbol_id),
        command,symbol_id) && sitehelper_project_remove_symbol_by_id(project,symbol_id);
}

int edit_plan_symbol_command_create(DomainId symbol_id, DomainId storey_id,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction,
    EditPlanSymbolCommand *command)
{
    if (command == NULL || symbol_id == DOMAIN_ID_INVALID || storey_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    DocumentPlanSymbol candidate={symbol_id,storey_id,kind,anchor,direction};
    if (!document_plan_symbol_is_locally_valid(&candidate)) { return 0; }
    *command=(EditPlanSymbolCommand){symbol_id,storey_id,kind,anchor,direction};
    return 1;
}

int edit_plan_symbol_command_execute(SiteHelperProject *project,
    const EditPlanSymbolCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_update_plan_symbol(project,command->symbol_id,command->storey_id,
            command->kind,command->anchor,command->direction);
}

int delete_plan_symbol_command_create(DomainId symbol_id, DeletePlanSymbolCommand *command)
{
    if (command == NULL || symbol_id == DOMAIN_ID_INVALID) { return 0; }
    *command=(DeletePlanSymbolCommand){symbol_id};
    return 1;
}

int delete_plan_symbol_command_execute(SiteHelperProject *project,
    const DeletePlanSymbolCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_symbol_by_id(project,command->symbol_id);
}
