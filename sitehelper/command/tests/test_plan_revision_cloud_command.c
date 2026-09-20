#include <assert.h>
#include <stdio.h>
#include "command_history.h"
#include "sitehelper_command.h"

static const PlanPosition first_shape[]={{0,0},{1000,0},{1000,800},{0,800}};
static const PlanPosition second_shape[]={{100,50},{900,100},{850,700},{200,650}};

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    SiteHelperCommand command={0}; SiteHelperCommandResult result;
    CreatePlanRevisionCloudCommand create={0};
    assert(create_plan_revision_cloud_command_create(storey,first_shape,4,&create));
    assert(sitehelper_command_from_create_plan_revision_cloud(&create,&command));
    create_plan_revision_cloud_command_destroy(&create);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    DomainId id=result.data.revision_cloud.revision_cloud_id; assert(id); sitehelper_command_destroy(&command);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(!sitehelper_project_find_revision_cloud_by_id_const(&project,id));
    assert(sitehelper_command_history_redo(&history,&project));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id));
    DomainId revision=sitehelper_project_add_revision(&project,"A","Grouped change"); assert(revision);
    assert(sitehelper_project_set_revision_cloud_revision(&project,id,revision));

    EditPlanRevisionCloudCommand edit={0};
    assert(edit_plan_revision_cloud_command_create(id,storey,second_shape,4,&edit));
    assert(sitehelper_command_from_edit_plan_revision_cloud(&edit,&command));
    edit_plan_revision_cloud_command_destroy(&edit);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result)); sitehelper_command_destroy(&command);
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->vertices[0].x==100);
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->revision_id==revision);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->vertices[0].x==0);
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->revision_id==revision);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->vertices[0].x==100);
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->revision_id==revision);

    DeletePlanRevisionCloudCommand deletion;
    assert(delete_plan_revision_cloud_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_plan_revision_cloud(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result)); sitehelper_command_destroy(&command);
    assert(!sitehelper_project_find_revision_cloud_by_id_const(&project,id));
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,id)->revision_id==revision);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(!sitehelper_project_find_revision_cloud_by_id_const(&project,id));
    sitehelper_command_history_destroy(&history); sitehelper_project_destroy(&project);
    puts("All plan revision cloud command tests passed.");
    return 0;
}
