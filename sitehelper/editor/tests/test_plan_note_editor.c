#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_editor.h"

static void test_overlay_selection_and_reconcile(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{3000,0}});
    DomainId note=sitehelper_project_add_plan_note(&project,storey,
        (PlanPosition){500,0},wall,"Overlay");
    assert(storey&&wall&&note);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){500,0},&action));
    assert(action.kind==EDITOR_ACTION_NONE);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_NOTE,note));
    assert(editor.current_wall_id==DOMAIN_ID_INVALID);

    assert(sitehelper_project_remove_annotation_by_id(&project,note));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor_selection_is_empty(&editor.selection));
    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_note_actions_and_delete_selection(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));

    EditorAction action={0};
    assert(sitehelper_editor_create_plan_note_action(&editor,(PlanPosition){10,20},0,
        "New note",&action));
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId note_id=result.data.annotation.annotation_id;
    sitehelper_editor_complete_action(&editor,&action,&result);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_NOTE,note_id));
    editor_action_destroy(&action);

    assert(sitehelper_editor_create_edit_plan_note_action(&editor,(PlanPosition){30,40},0,
        "Edited",&action));
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    const DocumentAnnotation *note=sitehelper_project_find_annotation_by_id_const(&project,note_id);
    assert(note&&note->anchor.position.x==30&&strcmp(note->text,"Edited")==0);
    editor_action_destroy(&action);

    assert(sitehelper_editor_create_delete_selection_action(&editor,&action));
    assert(action.command.type==SITEHELPER_COMMAND_DELETE_PLAN_NOTE);
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    assert(editor_selection_is_empty(&editor.selection));
    editor_action_destroy(&action);

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_note_tool_authoring_target(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId note=sitehelper_project_add_plan_note(&project,storey,
        (PlanPosition){500,500},DOMAIN_ID_INVALID,"Existing");
    assert(storey&&note);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_tool_available(EDITOR_VIEW_PLAN,EDITOR_TOOL_NOTE));
    assert(!sitehelper_editor_tool_available(EDITOR_VIEW_WALL_ELEVATION,EDITOR_TOOL_NOTE));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_NOTE));

    DomainId annotation_id=99;
    PlanPosition position={0};
    assert(sitehelper_editor_prepare_plan_note_authoring(&editor,&project,
        (Vec2){155,245},&annotation_id,&position));
    assert(annotation_id==DOMAIN_ID_INVALID&&position.x==200&&position.y==200);
    assert(editor_selection_is_empty(&editor.selection));

    assert(sitehelper_editor_prepare_plan_note_authoring(&editor,&project,
        (Vec2){510,500},&annotation_id,&position));
    assert(annotation_id==note&&position.x==500&&position.y==500);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_NOTE,note));

    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    assert(editor.active_tool==EDITOR_TOOL_SELECT);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_overlay_selection_and_reconcile();
    test_note_actions_and_delete_selection();
    test_note_tool_authoring_target();
    puts("All plan note editor tests passed.");
    return 0;
}
