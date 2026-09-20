#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "command_history.h"
#include "sitehelper_command.h"

static SiteHelperCommand make_create(DomainId storey, PlanPosition position,
    DomainId target, const char *text)
{
    CreatePlanNoteCommand create={0};
    SiteHelperCommand command={0};
    assert(create_plan_note_command_create(storey,position,target,text,&create));
    assert(sitehelper_command_from_create_plan_note(&create,&command));
    create_plan_note_command_destroy(&create);
    return command;
}

static SiteHelperCommand make_edit(DomainId id, DomainId storey, PlanPosition position,
    DomainId target, const char *text)
{
    EditPlanNoteCommand edit={0};
    SiteHelperCommand command={0};
    assert(edit_plan_note_command_create(id,storey,position,target,text,&edit));
    assert(sitehelper_command_from_edit_plan_note(&edit,&command));
    edit_plan_note_command_destroy(&edit);
    return command;
}

static void test_create_edit_delete_history(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{4000,0}});
    assert(storey&&wall);

    SiteHelperCommand create=make_create(storey,(PlanPosition){500,0},wall,"Check stud");
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history,&project,&create,&result));
    DomainId note_id=result.data.annotation.annotation_id;
    assert(note_id!=DOMAIN_ID_INVALID);
    const DocumentAnnotation *note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&strcmp(note->text,"Check stud")==0&&note->target_id==wall);
    assert(sitehelper_command_history_undo(&history,&project));
    assert(sitehelper_project_find_annotation_by_id_const(&project,note_id)==NULL);
    assert(sitehelper_command_history_redo(&history,&project));
    note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&strcmp(note->text,"Check stud")==0);
    sitehelper_command_destroy(&create);

    SiteHelperCommand edit=make_edit(note_id,storey,(PlanPosition){800,120},wall,
        "Check lintel\nagainst schedule");
    assert(sitehelper_command_history_execute(&history,&project,&edit,&result));
    assert(result.data.annotation.annotation_id==note_id);
    note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&note->anchor.position.x==800&&note->anchor.position.y==120);
    assert(strcmp(note->text,"Check lintel\nagainst schedule")==0);
    assert(sitehelper_command_history_undo(&history,&project));
    note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&note->anchor.position.x==500&&strcmp(note->text,"Check stud")==0);
    assert(sitehelper_command_history_redo(&history,&project));
    note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&strcmp(note->text,"Check lintel\nagainst schedule")==0);
    sitehelper_command_destroy(&edit);

    DeletePlanNoteCommand deletion;
    SiteHelperCommand remove={0};
    assert(delete_plan_note_command_create(note_id,&deletion));
    assert(sitehelper_command_from_delete_plan_note(&deletion,&remove));
    assert(sitehelper_command_history_execute(&history,&project,&remove,&result));
    assert(sitehelper_project_find_annotation_by_id_const(&project,note_id)==NULL);
    assert(sitehelper_command_history_undo(&history,&project));
    note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&strcmp(note->text,"Check lintel\nagainst schedule")==0);
    assert(sitehelper_command_history_redo(&history,&project));
    assert(sitehelper_project_find_annotation_by_id_const(&project,note_id)==NULL);
    sitehelper_command_destroy(&remove);

    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_edit_preserves_weak_target_on_text_change(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{1000,0}});
    DomainId note_id=sitehelper_project_add_plan_note(&project,storey,
        (PlanPosition){100,20},wall,"Before");
    assert(note_id&&sitehelper_project_remove_wall_by_id(&project,wall));
    assert(sitehelper_project_validate(&project).code==SITEHELPER_PROJECT_VALID);

    SiteHelperCommand edit=make_edit(note_id,storey,(PlanPosition){100,20},wall,"After");
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history,&project,&edit,&result));
    const DocumentAnnotation *note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&note->target_id==wall&&strcmp(note->text,"After")==0);
    assert(sitehelper_command_history_undo(&history,&project));
    note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&note->target_id==wall&&strcmp(note->text,"Before")==0);
    sitehelper_command_destroy(&edit);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_create_edit_delete_history();
    test_edit_preserves_weak_target_on_text_change();
    puts("All plan note command tests passed.");
    return 0;
}
