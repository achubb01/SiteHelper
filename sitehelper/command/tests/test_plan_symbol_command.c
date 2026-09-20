#include <assert.h>
#include <stdio.h>

#include "command_history.h"
#include "sitehelper_command.h"

static void test_create_edit_delete_history(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    SiteHelperCommand command; SiteHelperCommandResult result;

    CreatePlanSymbolCommand create;
    assert(create_plan_symbol_command_create(storey,DOCUMENT_PLAN_SYMBOL_POINT_MARKER,
        (PlanPosition){100,200},(DocumentPlanDirection){0,0},&create));
    assert(sitehelper_command_from_create_plan_symbol(&create,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    DomainId id=result.data.symbol.symbol_id; assert(id);
    sitehelper_command_destroy(&command);
    const DocumentPlanSymbol *symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->anchor.x==100&&symbol->anchor.y==200);

    assert(sitehelper_command_history_undo(&history,&project));
    assert(!sitehelper_project_find_symbol_by_id_const(&project,id));
    assert(sitehelper_command_history_redo(&history,&project));
    symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->id==id);

    EditPlanSymbolCommand edit;
    assert(edit_plan_symbol_command_create(id,storey,DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,
        (PlanPosition){700,-50},(DocumentPlanDirection){3,4},&edit));
    assert(sitehelper_command_from_edit_plan_symbol(&edit,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->anchor.x==700&&symbol->anchor.y==-50&&
        symbol->kind==DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION&&symbol->direction.dx==3&&symbol->direction.dy==4);
    assert(sitehelper_command_history_undo(&history,&project));
    symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->anchor.x==100&&symbol->anchor.y==200);
    assert(sitehelper_command_history_redo(&history,&project));
    symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->anchor.x==700&&symbol->anchor.y==-50&&
        symbol->kind==DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION&&symbol->direction.dx==3&&symbol->direction.dy==4);

    DeletePlanSymbolCommand deletion;
    assert(delete_plan_symbol_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_plan_symbol(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    assert(!sitehelper_project_find_symbol_by_id_const(&project,id));
    assert(sitehelper_command_history_undo(&history,&project));
    symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->id==id&&symbol->anchor.x==700);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(!sitehelper_project_find_symbol_by_id_const(&project,id));

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_create_edit_delete_history();
    puts("All plan symbol command tests passed.");
    return 0;
}
