#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"

static void test_symbol_tool_authoring_selection_edit_delete(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{1000,0}}); assert(wall);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_tool_available(EDITOR_VIEW_PLAN,EDITOR_TOOL_SYMBOL));
    assert(!sitehelper_editor_tool_available(EDITOR_VIEW_WALL_ELEVATION,EDITOR_TOOL_SYMBOL));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SYMBOL));

    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1040,20},&action));
    assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(action.command.type==SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL);
    assert(action.command.data.create_plan_symbol.anchor.x==1000);
    assert(action.command.data.create_plan_symbol.anchor.y==0);
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId symbol_id=result.data.symbol.symbol_id; assert(symbol_id);
    sitehelper_editor_complete_action(&editor,&action,&result);
    editor_action_destroy(&action);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_SYMBOL,symbol_id));

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SELECT));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,0},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_SYMBOL,symbol_id));
    assert(editor.current_wall_id==DOMAIN_ID_INVALID);
    editor_action_destroy(&action);

    assert(sitehelper_editor_create_edit_plan_symbol_action(&editor,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){1200,300},
        (DocumentPlanDirection){0,0},&action));
    assert(action.command.type==SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL);
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    editor_action_destroy(&action);
    const DocumentPlanSymbol *symbol=sitehelper_project_find_symbol_by_id_const(&project,symbol_id);
    assert(symbol&&symbol->anchor.x==1200&&symbol->anchor.y==300);

    assert(sitehelper_editor_create_delete_selection_action(&editor,&action));
    assert(action.command.type==SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL);
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    editor_action_destroy(&action);
    assert(editor.selection.kind==EDITOR_SELECTION_NONE);
    assert(!sitehelper_project_find_symbol_by_id_const(&project,symbol_id));

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_symbol_selection_storey_scope_and_reconcile(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0);
    DomainId b=sitehelper_project_add_storey(&project,3000);
    DomainId symbol=sitehelper_project_add_plan_symbol(&project,a,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){50,50},(DocumentPlanDirection){0,0});
    assert(a&&b&&symbol);
    assert(sitehelper_project_add_plan_symbol(&project,b,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){50,50},(DocumentPlanDirection){0,0}));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){50,50},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_SYMBOL,symbol));
    editor_action_destroy(&action);
    assert(sitehelper_project_remove_symbol_by_id(&project,symbol));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor.selection.kind==EDITOR_SELECTION_NONE);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_view_direction_tool_authoring(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    assert(sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{1000,0}}));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_tool_available(EDITOR_VIEW_PLAN,EDITOR_TOOL_VIEW_DIRECTION));
    assert(!sitehelper_editor_tool_available(EDITOR_VIEW_WALL_ELEVATION,
        EDITOR_TOOL_VIEW_DIRECTION));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_VIEW_DIRECTION));

    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1025,10},&action));
    assert(action.kind==EDITOR_ACTION_NONE);
    assert(editor.direction_symbol_tool.stage==PLAN_DIRECTION_SYMBOL_TOOL_PICK_DIRECTION);
    assert(editor.direction_symbol_tool.anchor.x==1000&&editor.direction_symbol_tool.anchor.y==0);

    sitehelper_editor_pointer_move_in_project(&editor,&project,(Vec2){1500,500});
    PlanPosition anchor; PlanPoint direction_point; int ready=0;
    assert(sitehelper_editor_get_view_direction_preview(&editor,&anchor,&direction_point,&ready));
    assert(anchor.x==1000&&anchor.y==0&&ready);

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1500,500},&action));
    assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(action.command.type==SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL);
    assert(action.command.data.create_plan_symbol.kind==DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION);
    assert(action.command.data.create_plan_symbol.direction.dx==1);
    assert(action.command.data.create_plan_symbol.direction.dy==1);
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId id=result.data.symbol.symbol_id; assert(id);
    sitehelper_editor_complete_action(&editor,&action,&result);
    editor_action_destroy(&action);
    const DocumentPlanSymbol *symbol=sitehelper_project_find_symbol_by_id_const(&project,id);
    assert(symbol&&symbol->kind==DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION&&
        symbol->anchor.x==1000&&symbol->anchor.y==0&&
        symbol->direction.dx==1&&symbol->direction.dy==1);
    assert(editor.direction_symbol_tool.stage==PLAN_DIRECTION_SYMBOL_TOOL_PICK_ANCHOR);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_SYMBOL,id));

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SELECT));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,0},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_SYMBOL,id));
    editor_action_destroy(&action);

    sitehelper_editor_destroy(&editor); sitehelper_project_destroy(&project);
}

int main(void)
{
    test_symbol_tool_authoring_selection_edit_delete();
    test_symbol_selection_storey_scope_and_reconcile();
    test_view_direction_tool_authoring();
    puts("All plan symbol editor tests passed.");
    return 0;
}
