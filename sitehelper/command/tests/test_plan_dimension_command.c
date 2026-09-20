#include <assert.h>
#include <stdio.h>

#include "command_history.h"
#include "sitehelper_command.h"

static DocumentDimensionReference fixed(int x,int y)
{
    return (DocumentDimensionReference){.kind=DOCUMENT_DIMENSION_FIXED_POINT,.position={x,y}};
}

static void test_create_edit_delete_history(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);

    CreatePlanDimensionCommand create;
    assert(create_plan_dimension_command_create(storey,fixed(0,0),fixed(3000,0),200,&create));
    SiteHelperCommand command;
    assert(sitehelper_command_from_create_plan_dimension(&create,&command));
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    DomainId id=result.data.dimension.dimension_id; assert(id);
    assert(sitehelper_project_find_dimension_by_id_const(&project,id));
    sitehelper_command_destroy(&command);

    assert(sitehelper_command_history_undo(&history,&project));
    assert(!sitehelper_project_find_dimension_by_id_const(&project,id));
    assert(sitehelper_command_history_redo(&history,&project));
    assert(sitehelper_project_find_dimension_by_id_const(&project,id));

    EditPlanDimensionCommand edit;
    assert(edit_plan_dimension_command_create(id,storey,fixed(100,0),fixed(4100,0),-350,&edit));
    assert(sitehelper_command_from_edit_plan_dimension(&edit,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    const DocumentPlanDimension *d=sitehelper_project_find_dimension_by_id_const(&project,id);
    assert(d&&d->offset_mm==-350&&d->first.position.x==100&&d->second.position.x==4100);
    sitehelper_command_destroy(&command);
    assert(sitehelper_command_history_undo(&history,&project));
    d=sitehelper_project_find_dimension_by_id_const(&project,id);
    assert(d&&d->offset_mm==200&&d->first.position.x==0&&d->second.position.x==3000);
    assert(sitehelper_command_history_redo(&history,&project));
    d=sitehelper_project_find_dimension_by_id_const(&project,id);
    assert(d&&d->offset_mm==-350);

    DeletePlanDimensionCommand deletion;
    assert(delete_plan_dimension_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_plan_dimension(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    assert(!sitehelper_project_find_dimension_by_id_const(&project,id));
    sitehelper_command_destroy(&command);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_project_find_dimension_by_id_const(&project,id));
    assert(sitehelper_command_history_redo(&history,&project));
    assert(!sitehelper_project_find_dimension_by_id_const(&project,id));

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}


static DocumentDimensionReference wall_ref(DocumentDimensionReferenceKind kind, DomainId id)
{
    return (DocumentDimensionReference){.kind=kind,.target_id=id};
}

static void test_weak_reference_restoration(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{2400,0}});
    DomainId id=sitehelper_project_add_plan_dimension(&project,storey,
        wall_ref(DOCUMENT_DIMENSION_WALL_START,wall),
        wall_ref(DOCUMENT_DIMENSION_WALL_END,wall),100);
    assert(storey&&wall&&id);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    DeletePlanDimensionCommand deletion; SiteHelperCommand command; SiteHelperCommandResult result;
    assert(delete_plan_dimension_command_create(id,&deletion));
    assert(sitehelper_command_from_delete_plan_dimension(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    assert(sitehelper_project_remove_wall_by_id(&project,wall));
    assert(sitehelper_command_history_undo(&history,&project));
    const DocumentPlanDimension *restored=sitehelper_project_find_dimension_by_id_const(&project,id);
    assert(restored&&restored->first.target_id==wall&&restored->second.target_id==wall);
    PlanPosition a,b; int distance;
    assert(!sitehelper_project_resolve_plan_dimension(&project,id,&a,&b,&distance));
    assert(sitehelper_project_validate(&project).code==SITEHELPER_PROJECT_VALID);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_create_edit_delete_history();
    test_weak_reference_restoration();
    puts("All plan dimension command tests passed.");
    return 0;
}
