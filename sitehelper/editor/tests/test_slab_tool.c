#include <assert.h>
#include <limits.h>
#include <math.h>
#include "sitehelper_editor.h"
#include "command_history.h"
#include "app_input.h"
#include "plan_position_conversion.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_realloc(void*,size_t);
void *__wrap_realloc(void*p,size_t n){
    return fail_after!=SIZE_MAX&&fail_after--==0?NULL:__real_realloc(p,n);
}
#endif

static EditorAction click(SiteHelperEditor *editor,const SiteHelperProject *project,int x,int y)
{
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(editor,project,(Vec2){x,y},&action));
    return action;
}

static void execute(SiteHelperEditor *editor,SiteHelperProject *project,
    SiteHelperCommandHistory *history,EditorAction *action,DomainId *id)
{
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(history,project,&action->command,&result));
    sitehelper_editor_complete_action(editor,action,&result);
    sitehelper_editor_reconcile(editor,project);
    *id=result.data.slab.slab_id; editor_action_destroy(action);
}

static void test_sketch_and_commit(void)
{
    SiteHelperProject p; SiteHelperEditor e; SiteHelperCommandHistory h;
    sitehelper_project_init(&p);DomainId storey=sitehelper_project_add_storey(&p,0);
    sitehelper_editor_init(&e);sitehelper_command_history_init(&h);
    assert(sitehelper_editor_set_current_storey(&e,&p,storey));
    assert(sitehelper_editor_set_active_tool(&e,EDITOR_TOOL_SLAB));
    DomainId next=p.domain_ids.next;
    EditorAction a=click(&e,&p,0,0); assert(a.kind==EDITOR_ACTION_NONE&&e.slab_tool.vertex_count==1);
    a=click(&e,&p,0,0); assert(e.slab_tool.vertex_count==1); /* consecutive duplicate */
    a=click(&e,&p,5000,0); assert(e.slab_tool.vertex_count==2);
    a=click(&e,&p,5000,4000); assert(e.slab_tool.vertex_count==3);
    a=click(&e,&p,0,4000); assert(e.slab_tool.vertex_count==4);
    assert(p.storeys[0].slabs.count==0&&p.domain_ids.next==next&&h.count==0);
    sitehelper_editor_pointer_move_in_project(&e,&p,(Vec2){10,10});
    const PlanPosition *vertices;size_t count;PlanPoint preview;int has;
    assert(sitehelper_editor_get_slab_preview(&e,&vertices,&count,&preview,&has));
    assert(count==4&&has&&preview.x==0&&preview.y==0); /* grid resolved */
    a=click(&e,&p,0,0); assert(a.kind==EDITOR_ACTION_COMMAND);
    assert(a.command.data.create_slab.vertex_count==4);
    DomainId id;execute(&e,&p,&h,&a,&id);
    assert(id==next&&p.storeys[0].slabs.count==1&&h.count==1&&e.slab_tool.vertex_count==0);
    const Slab *slab=sitehelper_project_find_slab_by_id_const(&p,id);
    assert(slab&&slab->definition.outline.vertex_count==4&&
        slab->definition.thickness_mm==100&&slab->definition.top_level_offset_mm==0);
    assert(e.selection.kind==EDITOR_SELECTION_SLAB&&e.selection.slab_id==id);
    assert(sitehelper_command_history_undo(&h,&p));sitehelper_editor_reconcile(&e,&p);
    assert(e.selection.kind==EDITOR_SELECTION_NONE&&!sitehelper_project_find_slab_by_id(&p,id));
    assert(sitehelper_command_history_redo(&h,&p));assert(sitehelper_project_find_slab_by_id(&p,id));
    sitehelper_editor_destroy(&e);sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
}

static void test_plan_coordinate_conversion(void)
{
    PlanPosition output={77,88};
    assert(plan_position_from_point((PlanPoint){12.875, -12.875},&output));
    assert(output.x==12&&output.y==-12); /* established Wall Tool truncation */
    assert(plan_position_from_point((PlanPoint){123.0,-456.0},&output));
    assert(output.x==123&&output.y==-456);
    assert(plan_position_from_point((PlanPoint){INT_MIN,INT_MAX},&output));
    assert(output.x==INT_MIN&&output.y==INT_MAX);
    output=(PlanPosition){77,88};
    assert(!plan_position_from_point((PlanPoint){nextafter((double)INT_MAX,INFINITY),0},&output));
    assert(output.x==77&&output.y==88);
    assert(!plan_position_from_point((PlanPoint){0,nextafter((double)INT_MIN,-INFINITY)},&output));
    assert(output.x==77&&output.y==88);
    assert(!plan_position_from_point((PlanPoint){NAN,0},&output));
}

static void test_invalid_and_cancellation(void)
{
    SiteHelperProject p;SiteHelperEditor e;sitehelper_project_init(&p);
    DomainId first=sitehelper_project_add_storey(&p,0),second=sitehelper_project_add_storey(&p,3000);
    sitehelper_editor_init(&e);assert(sitehelper_editor_set_current_storey(&e,&p,first));
    assert(sitehelper_editor_set_active_tool(&e,EDITOR_TOOL_SLAB));
    (void)click(&e,&p,0,0);(void)click(&e,&p,4000,4000);
    (void)click(&e,&p,0,4000);(void)click(&e,&p,4000,0); /* bow-tie */
    EditorAction action={0};DomainId next=p.domain_ids.next;
    assert(!sitehelper_editor_create_slab_action(&e,&action));
    assert(e.slab_tool.vertex_count==4&&p.storeys[0].slabs.count==0&&p.domain_ids.next==next);
    assert(sitehelper_editor_cancel_tool_interaction(&e)&&e.slab_tool.vertex_count==0);
    (void)click(&e,&p,0,0);assert(sitehelper_editor_set_active_tool(&e,EDITOR_TOOL_SELECT));
    assert(e.slab_tool.vertex_count==0);
    assert(sitehelper_editor_set_active_tool(&e,EDITOR_TOOL_SLAB));(void)click(&e,&p,0,0);
    assert(sitehelper_editor_set_current_storey(&e,&p,second)&&e.slab_tool.vertex_count==0);
    (void)click(&e,&p,0,0);assert(sitehelper_editor_set_active_view(&e,EDITOR_VIEW_WALL_ELEVATION));
    assert(e.slab_tool.vertex_count==0&&e.active_tool==EDITOR_TOOL_SELECT);
    assert(sitehelper_editor_set_active_view(&e,EDITOR_VIEW_PLAN));assert(sitehelper_editor_set_active_tool(&e,EDITOR_TOOL_SLAB));
    (void)click(&e,&p,0,0);sitehelper_editor_project_replaced(&e,&p);assert(e.slab_tool.vertex_count==0);
    sitehelper_editor_destroy(&e);sitehelper_project_destroy(&p);
}

static void test_enter_and_delete_routes(void)
{
    SiteHelperProject p;SiteHelperEditor e;SiteHelperCommandHistory h;AppInput input={0};
    sitehelper_project_init(&p);DomainId storey=sitehelper_project_add_storey(&p,0);
    sitehelper_editor_init(&e);sitehelper_command_history_init(&h);
    assert(sitehelper_editor_set_current_storey(&e,&p,storey));assert(sitehelper_editor_set_active_tool(&e,EDITOR_TOOL_SLAB));
    (void)click(&e,&p,0,0);(void)click(&e,&p,5000,0);(void)click(&e,&p,5000,4000);(void)click(&e,&p,0,4000);
    PlatformEvent enter={.type=PLATFORM_EVENT_KEY_DOWN,.data.key_down={.key=PLATFORM_KEY_ENTER}};
    EditorAction action={0};assert(app_input_route(&input,&e,&enter,&action)==APP_INPUT_COMMAND);
    DomainId id;execute(&e,&p,&h,&action,&id);
    PlatformEvent del={.type=PLATFORM_EVENT_KEY_DOWN,.data.key_down={.key=PLATFORM_KEY_DELETE}};
    assert(app_input_route(&input,&e,&del,&action)==APP_INPUT_COMMAND);
    SiteHelperCommandResult result;assert(sitehelper_command_history_execute(&h,&p,&action.command,&result));
    editor_action_destroy(&action);sitehelper_editor_reconcile(&e,&p);
    assert(!sitehelper_project_find_slab_by_id(&p,id)&&e.selection.kind==EDITOR_SELECTION_NONE);
    assert(sitehelper_command_history_undo(&h,&p)&&sitehelper_project_find_slab_by_id(&p,id));
    editor_selection_set_slab_feature(&e.selection,EDITOR_SELECTION_SCOPE_PLAN,id,
        EDITOR_SELECTION_SLAB_REGION,0);
    assert(app_input_route(&input,&e,&del,&action)==APP_INPUT_UNHANDLED);
    assert(sitehelper_project_find_slab_by_id(&p,id));
    sitehelper_editor_destroy(&e);sitehelper_command_history_destroy(&h);sitehelper_project_destroy(&p);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_vertex_allocation_failure(void)
{
    SlabTool tool;slab_tool_init(&tool);slab_tool_activate(&tool);
    fail_after=0;assert(!slab_tool_append(&tool,1,(PlanPosition){1,2}));
    assert(tool.vertex_count==0&&tool.vertices==NULL);
    fail_after=SIZE_MAX;assert(slab_tool_append(&tool,1,(PlanPosition){1,2}));
    slab_tool_destroy(&tool);
}
#endif

int main(void){test_plan_coordinate_conversion();test_sketch_and_commit();test_invalid_and_cancellation();test_enter_and_delete_routes();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
test_vertex_allocation_failure();
#endif
return 0;}
