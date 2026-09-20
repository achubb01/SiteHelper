#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "command_history.h"
#include "sitehelper_command.h"

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    const PlanPosition vertices[]={{0,0},{100,0},{100,100},{0,100}};
    DomainId cloud=sitehelper_project_add_plan_revision_cloud(&project,storey,vertices,4); assert(cloud);

    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    SiteHelperCommand command={0}; SiteHelperCommandResult result;

    CreateDocumentRevisionCommand create={0};
    assert(create_document_revision_command_create("A","First change",&create));
    assert(sitehelper_command_from_create_document_revision(&create,&command));
    create_document_revision_command_destroy(&create);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    DomainId revision=result.data.revision.revision_id; assert(revision);
    sitehelper_command_destroy(&command);

    SetPlanRevisionCloudRevisionCommand assignment;
    assert(set_plan_revision_cloud_revision_command_create(cloud,revision,&assignment));
    assert(sitehelper_command_from_set_plan_revision_cloud_revision(&assignment,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    assert(result.data.revision_cloud.revision_cloud_id==cloud);
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,cloud)->revision_id==revision);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,cloud)->revision_id==DOMAIN_ID_INVALID);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,cloud)->revision_id==revision);

    EditDocumentRevisionCommand edit={0};
    assert(edit_document_revision_command_create(revision,"B","Updated change",&edit));
    assert(sitehelper_command_from_edit_document_revision(&edit,&command));
    edit_document_revision_command_destroy(&edit);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    assert(strcmp(sitehelper_project_find_revision_by_id_const(&project,revision)->identifier,"B")==0);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(strcmp(sitehelper_project_find_revision_by_id_const(&project,revision)->identifier,"A")==0);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(strcmp(sitehelper_project_find_revision_by_id_const(&project,revision)->identifier,"B")==0);

    DeleteDocumentRevisionCommand deletion;
    assert(delete_document_revision_command_create(revision,&deletion));
    assert(sitehelper_command_from_delete_document_revision(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    assert(sitehelper_project_find_revision_by_id_const(&project,revision)==NULL);
    assert(sitehelper_project_validate(&project).code==SITEHELPER_PROJECT_VALID);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_project_find_revision_by_id_const(&project,revision));
    assert(strcmp(sitehelper_project_find_revision_by_id_const(&project,revision)->identifier,"B")==0);
    assert(sitehelper_project_find_revision_cloud_by_id_const(&project,cloud)->revision_id==revision);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(sitehelper_project_find_revision_by_id_const(&project,revision)==NULL);

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
    puts("All document revision command tests passed.");
    return 0;
}
