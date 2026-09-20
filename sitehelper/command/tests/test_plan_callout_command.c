#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "command_history.h"
#include "sitehelper_command.h"

static void test_create_edit_delete_history(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    SiteHelperCommand command={0}; SiteHelperCommandResult result;

    CreatePlanCalloutCommand create={0};
    assert(create_plan_callout_command_create(storey,(PlanPosition){100,200},
        (PlanPosition){700,500},"Check flashing",&create));
    assert(sitehelper_command_from_create_plan_callout(&create,&command));
    create_plan_callout_command_destroy(&create);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    DomainId id=result.data.callout.callout_id; assert(id);
    sitehelper_command_destroy(&command);
    const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&strcmp(callout->text,"Check flashing")==0);

    assert(sitehelper_command_history_undo(&history,&project));
    assert(!sitehelper_project_find_callout_by_id_const(&project,id));
    assert(sitehelper_command_history_redo(&history,&project));
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->id==id);

    EditPlanCalloutCommand edit={0};
    assert(edit_plan_callout_command_create(id,storey,(PlanPosition){150,250},
        (PlanPosition){900,800},"Updated\ntext",&edit));
    assert(sitehelper_command_from_edit_plan_callout(&edit,&command));
    edit_plan_callout_command_destroy(&edit);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->target.x==150&&strcmp(callout->text,"Updated\ntext")==0);
    assert(sitehelper_command_history_undo(&history,&project));
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->target.x==100&&strcmp(callout->text,"Check flashing")==0);
    assert(sitehelper_command_history_redo(&history,&project));
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->label_anchor.x==900&&strcmp(callout->text,"Updated\ntext")==0);

    DeletePlanCalloutCommand deletion;
    assert(delete_plan_callout_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_plan_callout(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    assert(!sitehelper_project_find_callout_by_id_const(&project,id));
    assert(sitehelper_command_history_undo(&history,&project));
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&callout->id==id&&strcmp(callout->text,"Updated\ntext")==0);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(!sitehelper_project_find_callout_by_id_const(&project,id));

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_create_edit_delete_history();
    puts("All plan callout command tests passed.");
    return 0;
}
