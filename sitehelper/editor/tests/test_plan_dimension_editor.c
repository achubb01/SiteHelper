#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"
#include "command_history.h"
#include "sitehelper_command.h"

static DocumentDimensionReference fixed(int x,int y)
{
    return (DocumentDimensionReference){.kind=DOCUMENT_DIMENSION_FIXED_POINT,.position={x,y}};
}

static void test_actions_selection_and_reconcile(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));

    EditorAction action={0}; SiteHelperCommandResult result;
    assert(sitehelper_editor_create_plan_dimension_action(&editor,fixed(0,0),fixed(2000,0),150,&action));
    assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId id=result.data.dimension.dimension_id;
    sitehelper_editor_complete_action(&editor,&action,&result);
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_DIMENSION,id));
    editor_action_destroy(&action);

    assert(sitehelper_editor_create_edit_plan_dimension_action(&editor,fixed(0,0),fixed(2500,0),250,&action));
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    const DocumentPlanDimension *d=sitehelper_project_find_dimension_by_id_const(&project,id);
    assert(d&&d->offset_mm==250&&d->second.position.x==2500);
    editor_action_destroy(&action);

    assert(sitehelper_editor_create_delete_selection_action(&editor,&action));
    assert(action.command.type==SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION);
    assert(sitehelper_command_execute(&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    assert(editor_selection_is_empty(&editor.selection));
    editor_action_destroy(&action);

    DomainId id2=sitehelper_project_add_plan_dimension(&project,storey,fixed(0,0),fixed(1000,0),50);
    assert(id2);
    editor_selection_set_dimension(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,id2);
    assert(sitehelper_project_remove_dimension_by_id(&project,id2));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor_selection_is_empty(&editor.selection));

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}


static void test_plan_select_hits_visible_dimension_before_wall(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,200},{2000,200}}); assert(wall);
    DomainId dimension=sitehelper_project_add_plan_dimension(&project,storey,
        fixed(0,0),fixed(2000,0),200); assert(dimension);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,200},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_DIMENSION,dimension));
    editor_action_destroy(&action);

    assert(sitehelper_project_remove_dimension_by_id(&project,dimension));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,200},&action));
    assert(editor.selection.kind==EDITOR_SELECTION_WALL);
    assert(editor.selection.wall_id==wall);
    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}


static void test_dimension_authoring_creates_associative_wall_dimension(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{2000,0}}); assert(wall);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(editor_view_supports_tool(EDITOR_VIEW_PLAN,EDITOR_TOOL_DIMENSION));
    assert(!editor_view_supports_tool(EDITOR_VIEW_WALL_ELEVATION,EDITOR_TOOL_DIMENSION));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));

    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(action.kind==EDITOR_ACTION_NONE);
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_SECOND);
    assert(editor.dimension_tool.first.kind==DOCUMENT_DIMENSION_WALL_START);
    assert(editor.dimension_tool.first.target_id==wall);

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){2000,0},&action));
    assert(action.kind==EDITOR_ACTION_NONE);
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PLACE_OFFSET);
    assert(editor.dimension_tool.second.kind==DOCUMENT_DIMENSION_WALL_END);
    assert(editor.dimension_tool.second.target_id==wall);

    sitehelper_editor_pointer_move_in_project(&editor,&project,(Vec2){1000,300});
    DocumentPlanDimensionGeometry geometry; int distance,ready;
    assert(sitehelper_editor_get_plan_dimension_preview(&editor,&geometry,&distance,&ready));
    assert(distance==2000&&ready&&geometry.line_first.y==300&&geometry.line_second.y==300);

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,300},&action));
    assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(action.command.type==SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION);
    assert(action.command.data.create_plan_dimension.offset_mm==300);
    assert(action.command.data.create_plan_dimension.first.kind==DOCUMENT_DIMENSION_WALL_START);
    assert(action.command.data.create_plan_dimension.second.kind==DOCUMENT_DIMENSION_WALL_END);
    SiteHelperCommandResult result;
    assert(sitehelper_command_execute(&project,&action.command,&result));
    DomainId id=result.data.dimension.dimension_id; assert(id);
    sitehelper_editor_complete_action(&editor,&action,&result);
    editor_action_destroy(&action);
    const DocumentPlanDimension *d=sitehelper_project_find_dimension_by_id_const(&project,id);
    assert(d&&d->offset_mm==300&&d->first.target_id==wall&&d->second.target_id==wall);
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);
    assert(editor.dimension_tool.active);

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_dimension_authoring_ambiguous_endpoint_is_fixed_and_cancel(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    assert(sitehelper_project_add_wall(&project,storey,(WallPlanSegment){{0,0},{1000,0}}));
    assert(sitehelper_project_add_wall(&project,storey,(WallPlanSegment){{0,0},{0,1000}}));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(editor.dimension_tool.first.kind==DOCUMENT_DIMENSION_FIXED_POINT);
    assert(editor.dimension_tool.first.position.x==0&&editor.dimension_tool.first.position.y==0);
    assert(sitehelper_editor_cancel_tool_interaction(&editor));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);
    assert(!sitehelper_editor_cancel_tool_interaction(&editor));
    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_dimension_authoring_preserves_unique_endpoint_at_t_junction(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId terminating=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{1000,0}}); assert(terminating);
    assert(sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{1000,-1000},{1000,1000}}));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,0},&action));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_SECOND);
    assert(editor.dimension_tool.first.kind==DOCUMENT_DIMENSION_WALL_END);
    assert(editor.dimension_tool.first.target_id==terminating);
    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_dimension_authoring_true_crossing_remains_fixed(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    assert(sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{-1000,0},{1000,0}}));
    assert(sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,-1000},{0,1000}}));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(editor.dimension_tool.first.kind==DOCUMENT_DIMENSION_FIXED_POINT);
    assert(editor.dimension_tool.first.position.x==0&&editor.dimension_tool.first.position.y==0);
    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_dimension_tool_context_changes_cancel_transient_authoring(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0);
    DomainId b=sitehelper_project_add_storey(&project,3000);
    assert(a&&b);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_SECOND);

    assert(sitehelper_editor_set_current_storey(&editor,&project,b));
    assert(editor.active_tool==EDITOR_TOOL_DIMENSION&&editor.dimension_tool.active);
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,0},&action));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PLACE_OFFSET);
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SELECT));
    assert(!editor.dimension_tool.active&&editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){0,0},&action));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,0},&action));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PLACE_OFFSET);
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    assert(editor.active_tool==EDITOR_TOOL_SELECT);
    assert(!editor.dimension_tool.active&&editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);

    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_history_reconcile_cancels_in_progress_dimension(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    CreatePlanDimensionCommand create;
    assert(create_plan_dimension_command_create(storey,fixed(0,0),fixed(1000,0),100,&create));
    SiteHelperCommand command; SiteHelperCommandResult result;
    assert(sitehelper_command_from_create_plan_dimension(&create,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);

    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){2000,0},&action));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_SECOND);
    assert(sitehelper_command_history_undo(&history,&project));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor.active_tool==EDITOR_TOOL_DIMENSION&&editor.dimension_tool.active);
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){2000,0},&action));
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_SECOND);
    assert(sitehelper_command_history_redo(&history,&project));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor.dimension_tool.stage==PLAN_DIMENSION_TOOL_PICK_FIRST);

    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
}

static void test_associative_dimension_disappears_and_recovers_with_wall_history(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId wall=sitehelper_project_add_wall(&project,storey,
        (WallPlanSegment){{0,0},{2000,0}}); assert(wall);
    DocumentDimensionReference start={.kind=DOCUMENT_DIMENSION_WALL_START,.target_id=wall};
    DocumentDimensionReference end={.kind=DOCUMENT_DIMENSION_WALL_END,.target_id=wall};
    DomainId dimension=sitehelper_project_add_plan_dimension(&project,storey,start,end,200);
    assert(dimension);

    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,200},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_DIMENSION,dimension));

    SiteHelperCommandHistory history; sitehelper_command_history_init(&history);
    DeleteWallCommand deletion; SiteHelperCommand command; SiteHelperCommandResult result;
    assert(delete_wall_command_create(wall,&deletion));
    assert(sitehelper_command_from_delete_wall(&deletion,&command));
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);
    sitehelper_editor_reconcile(&editor,&project);
    PlanPosition a,b; int distance;
    assert(!sitehelper_project_resolve_plan_dimension(&project,dimension,&a,&b,&distance));
    sitehelper_editor_clear_selection(&editor);
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,200},&action));
    assert(editor_selection_is_empty(&editor.selection));

    assert(sitehelper_command_history_undo(&history,&project));
    sitehelper_editor_reconcile(&editor,&project);
    assert(sitehelper_project_resolve_plan_dimension(&project,dimension,&a,&b,&distance));
    assert(distance==2000);
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,200},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_DIMENSION,dimension));

    editor_action_destroy(&action);
    sitehelper_command_history_destroy(&history);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_dimension_overlap_picking_prefers_nearest_then_later_authored(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0); assert(storey);
    DomainId first=sitehelper_project_add_plan_dimension(&project,storey,
        fixed(0,0),fixed(2000,0),100);
    DomainId second=sitehelper_project_add_plan_dimension(&project,storey,
        fixed(0,0),fixed(2000,0),100);
    DomainId later_but_farther=sitehelper_project_add_plan_dimension(&project,storey,
        fixed(0,0),fixed(2000,0),140);
    assert(first&&second&&later_but_farther);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,100},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_DIMENSION,second));
    assert(sitehelper_project_remove_dimension_by_id(&project,second));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){1000,100},&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_DIMENSION,first));
    editor_action_destroy(&action);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_actions_selection_and_reconcile();
    test_plan_select_hits_visible_dimension_before_wall();
    test_dimension_authoring_creates_associative_wall_dimension();
    test_dimension_authoring_ambiguous_endpoint_is_fixed_and_cancel();
    test_dimension_authoring_preserves_unique_endpoint_at_t_junction();
    test_dimension_authoring_true_crossing_remains_fixed();
    test_dimension_tool_context_changes_cancel_transient_authoring();
    test_history_reconcile_cancels_in_progress_dimension();
    test_associative_dimension_disappears_and_recovers_with_wall_history();
    test_dimension_overlap_picking_prefers_nearest_then_later_authored();
    puts("All plan dimension editor tests passed.");
    return 0;
}
