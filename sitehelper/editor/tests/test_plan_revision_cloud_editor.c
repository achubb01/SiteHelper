#include <assert.h>
#include <stdio.h>
#include "sitehelper_editor.h"

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    assert(sitehelper_project_add_wall(&project,storey,(WallPlanSegment){{0,0},{1000,0}}));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_REVISION_CLOUD));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,0},&action));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,600},&action));
    assert(editor.revision_cloud_tool.vertex_count==3);
    assert(sitehelper_editor_create_active_polygon_action(&editor,&action));
    assert(action.command.type==SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD);
    SiteHelperCommandResult result; assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId id=result.data.revision_cloud.revision_cloud_id; assert(id);
    sitehelper_editor_complete_action(&editor,&action,&result); editor_action_destroy(&action);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_REVISION_CLOUD,id));
    assert(editor.revision_cloud_tool.vertex_count==0);

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SELECT));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){500,0},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_REVISION_CLOUD,id));
    assert(editor.current_wall_id==DOMAIN_ID_INVALID); editor_action_destroy(&action);
    assert(sitehelper_editor_create_delete_selection_action(&editor,&action));
    assert(action.command.type==SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD);
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result); editor_action_destroy(&action);
    assert(editor.selection.kind==EDITOR_SELECTION_NONE);
    sitehelper_editor_destroy(&editor); sitehelper_project_destroy(&project);
    puts("All plan revision cloud editor tests passed.");
    return 0;
}
