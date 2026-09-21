#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_editor.h"

static void test_authoring_selection_edit_delete(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId wall=sitehelper_project_add_wall(&project,storey,(WallPlanSegment){{0,0},{1000,0}});
    assert(wall);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(editor_view_supports_tool(EDITOR_VIEW_PLAN,EDITOR_TOOL_CALLOUT));
    assert(!editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION,EDITOR_TOOL_CALLOUT));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_CALLOUT));
    EditorAction action={0};

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){20,15},&action));
    assert(editor.callout_tool.stage==PLAN_CALLOUT_TOOL_PICK_LABEL);
    sitehelper_editor_pointer_move_in_project(&editor,&project,(Vec2){640,360});
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){640,360},&action));
    PlanPosition target,label;
    assert(sitehelper_editor_get_plan_callout_ready(&editor,&target,&label));
    assert(!(target.x==label.x&&target.y==label.y));

    assert(sitehelper_editor_create_plan_callout_action(&editor,target,label,"Wall note",&action));
    assert(action.kind==EDITOR_ACTION_COMMAND&&action.command.type==SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT);
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId id=result.data.callout.callout_id; assert(id);
    sitehelper_editor_complete_action(&editor,&action,&result); editor_action_destroy(&action);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_CALLOUT,id));
    assert(editor.callout_tool.stage==PLAN_CALLOUT_TOOL_PICK_TARGET);

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SELECT));
    const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(&project,id); assert(callout);
    PlanPoint midpoint={(callout->target.x+callout->label_anchor.x)/2.0,
        (callout->target.y+callout->label_anchor.y)/2.0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){midpoint.x,midpoint.y},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_CALLOUT,id));
    assert(editor.current_wall_id==DOMAIN_ID_INVALID); editor_action_destroy(&action);

    assert(sitehelper_editor_create_edit_plan_callout_action(&editor,callout->target,
        callout->label_anchor,"Edited text",&action));
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result); editor_action_destroy(&action);
    callout=sitehelper_project_find_callout_by_id_const(&project,id);
    assert(callout&&strcmp(callout->text,"Edited text")==0);

    assert(sitehelper_editor_create_delete_selection_action(&editor,&action));
    assert(action.command.type==SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT);
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result); editor_action_destroy(&action);
    assert(editor.selection.kind==EDITOR_SELECTION_NONE&&!sitehelper_project_find_callout_by_id_const(&project,id));
    sitehelper_editor_destroy(&editor); sitehelper_project_destroy(&project);
}

static void test_storey_scope_and_reconcile(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0),b=sitehelper_project_add_storey(&project,3000);
    DomainId first=sitehelper_project_add_plan_callout(&project,a,(PlanPosition){0,0},
        (PlanPosition){1000,0},"A");
    DomainId second=sitehelper_project_add_plan_callout(&project,b,(PlanPosition){0,0},
        (PlanPosition){1000,0},"B");
    assert(a&&b&&first&&second);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){500,0},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_CALLOUT,first));
    editor_action_destroy(&action);
    assert(sitehelper_project_remove_callout_by_id(&project,first));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor.selection.kind==EDITOR_SELECTION_NONE);
    sitehelper_editor_destroy(&editor); sitehelper_project_destroy(&project);
}

int main(void)
{
    test_authoring_selection_edit_delete();
    test_storey_scope_and_reconcile();
    puts("All plan callout editor tests passed.");
    return 0;
}
