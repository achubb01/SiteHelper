#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "app_input.h"
#include "command_history.h"
#include "sitehelper_editor.h"
#include "slab.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after=SIZE_MAX;
void *__real_malloc(size_t); void *__real_calloc(size_t,size_t);
void *__real_realloc(void*,size_t);
static int fail_now(void){return fail_after!=SIZE_MAX&&fail_after--==0;}
void *__wrap_malloc(size_t n){return fail_now()?NULL:__real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){return fail_now()?NULL:__real_calloc(n,s);}
void *__wrap_realloc(void *p,size_t n){return fail_now()?NULL:__real_realloc(p,n);}
#endif

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};

typedef struct {
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    DomainId storey_id;
    DomainId slab_id;
} Fixture;

static void fixture_init(Fixture *f)
{
    *f=(Fixture){0};sitehelper_project_init(&f->project);
    f->storey_id=sitehelper_project_add_storey(&f->project,3000);
    f->slab_id=sitehelper_project_add_slab(&f->project,f->storey_id,
        outer,4,125,-25);
    assert(f->slab_id!=DOMAIN_ID_INVALID);
    sitehelper_editor_init(&f->editor);sitehelper_command_history_init(&f->history);
    assert(sitehelper_editor_set_current_storey(&f->editor,&f->project,f->storey_id));
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_command_history_destroy(&f->history);
    sitehelper_editor_destroy(&f->editor);sitehelper_project_destroy(&f->project);
}

static EditorAction click(Fixture *f,double x,double y)
{
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&f->editor,&f->project,
        (Vec2){x,y},&action));
    return action;
}

static int execute(Fixture *f,EditorAction *action,SiteHelperCommandResult *result)
{
    int ok=sitehelper_command_history_execute(&f->history,&f->project,
        &action->command,result);
    if (ok) {
        sitehelper_editor_complete_action(&f->editor,action,result);
        sitehelper_editor_reconcile(&f->editor,&f->project);
    }
    editor_action_destroy(action);
    return ok;
}

static EditorAction polygon(Fixture *f,EditorTool kind,
    const PlanPosition *vertices,size_t count,int close_with_click)
{
    assert(sitehelper_editor_set_active_tool(&f->editor,kind));
    EditorAction action={0};
    for (size_t i=0;i<count;i++) {
        action=click(f,vertices[i].x,vertices[i].y);
        assert(action.kind==EDITOR_ACTION_NONE);
    }
    if (close_with_click) {
        action=(EditorAction){0};
        (void)sitehelper_editor_primary_action_in_project(&f->editor,&f->project,
            (Vec2){vertices[0].x,vertices[0].y},&action);
    }
    else { assert(sitehelper_editor_create_active_polygon_action(&f->editor,&action)); }
    return action;
}

static void test_penetration_creation_and_failures(void)
{
    Fixture f;fixture_init(&f);DomainId next=f.project.domain_ids.next;
    const PlanPosition hole[]={{1000,1000},{2500,1000},{2500,2000},{1000,2000}};
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_PENETRATION));
    EditorAction action=click(&f,1000,1000);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_penetration_tool.slab_id==f.slab_id);
    (void)click(&f,1000,1000);
    assert(f.editor.slab_penetration_tool.vertex_count==1);
    sitehelper_editor_pointer_move_in_project(&f.editor,&f.project,(Vec2){2500,1000});
    EditorSlabPolygonPreviewKind preview_kind;DomainId preview_slab;
    const PlanPosition *preview_vertices;size_t preview_count;PlanPoint preview;int has_preview;
    assert(sitehelper_editor_get_slab_feature_polygon_preview(&f.editor,&preview_kind,
        &preview_slab,&preview_vertices,&preview_count,&preview,&has_preview));
    assert(preview_kind==EDITOR_SLAB_POLYGON_PREVIEW_PENETRATION&&preview_slab==f.slab_id&&
        preview_count==1&&has_preview&&preview.x==2500);
    for(size_t i=1;i<4;i++){action=click(&f,hole[i].x,hole[i].y);assert(action.kind==EDITOR_ACTION_NONE);}
    action=click(&f,hole[0].x,hole[0].y);assert(action.kind==EDITOR_ACTION_COMMAND);
    SiteHelperCommandResult result;assert(execute(&f,&action,&result));
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    assert(slab->definition.penetrations.count==1&&f.history.count==1&&
        f.project.domain_ids.next==next);
    assert(f.editor.selection.kind==EDITOR_SELECTION_SLAB_PENETRATION&&
        f.editor.selection.slab_id==f.slab_id&&f.editor.selection.slab_feature_index==0);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab->definition.penetrations.count==0);
    assert(sitehelper_command_history_redo(&f.history,&f.project));
    assert(slab->definition.penetrations.count==1);

    const PlanPosition overlap[]={{2000,1500},{3000,1500},{3000,2500},{2000,2500}};
    action=polygon(&f,EDITOR_TOOL_SLAB_PENETRATION,overlap,4,0);
    assert(action.kind==EDITOR_ACTION_COMMAND&&!execute(&f,&action,&result));
    assert(f.editor.slab_penetration_tool.vertex_count==4&&
        slab->definition.penetrations.count==1&&f.history.count==1);
    assert(sitehelper_editor_cancel_tool_interaction(&f.editor));

    const PlanPosition crossing[]={{3000,3000},{5000,5000},{3000,5000},{5000,3000}};
    action=polygon(&f,EDITOR_TOOL_SLAB_PENETRATION,crossing,4,1);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_penetration_tool.vertex_count==4);
    assert(slab->definition.penetrations.count==1&&f.history.count==1&&
        f.project.domain_ids.next==next);
    assert(sitehelper_editor_cancel_tool_interaction(&f.editor));

    const PlanPosition outside[]={{6000,6000},{11000,6000},{11000,7000},{6000,7000}};
    action=polygon(&f,EDITOR_TOOL_SLAB_PENETRATION,outside,4,0);
    assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(!execute(&f,&action,&result));
    assert(f.editor.slab_penetration_tool.vertex_count==4&&f.history.count==1&&
        slab->definition.penetrations.count==1&&f.project.domain_ids.next==next);
    fixture_destroy(&f);
}

static void test_region_creation_and_boundary(void)
{
    Fixture f;fixture_init(&f);DomainId next=f.project.domain_ids.next;
    const PlanPosition region[]={{0,0},{3000,0},{3000,2000},{0,2000}};
    EditorAction action=polygon(&f,EDITOR_TOOL_SLAB_REGION,region,4,0);
    assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(action.command.data.add_slab_region.top_level_offset_mm==-25&&
        action.command.data.add_slab_region.thickness_mm==125);
    SiteHelperCommandResult result;assert(execute(&f,&action,&result));
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    assert(slab->definition.regions.count==1&&
        slab->definition.regions.items[0].top_level_offset_mm==-25&&
        slab->definition.regions.items[0].thickness_mm==125);
    assert(f.editor.selection.kind==EDITOR_SELECTION_SLAB_REGION&&
        f.editor.selection.slab_feature_index==0&&f.project.domain_ids.next==next);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(sitehelper_command_history_redo(&f.history,&f.project));

    const PlanPosition overlap[]={{2000,1000},{4000,1000},{4000,3000},{2000,3000}};
    action=polygon(&f,EDITOR_TOOL_SLAB_REGION,overlap,4,0);
    assert(!execute(&f,&action,&result));
    assert(f.editor.slab_region_tool.vertex_count==4&&slab->definition.regions.count==1&&
        f.history.count==1&&f.project.domain_ids.next==next);
    assert(sitehelper_editor_cancel_tool_interaction(&f.editor));
    const PlanPosition hole[]={{5000,3000},{6000,3000},{6000,4000},{5000,4000}};
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    const PlanPosition crosses_void[]={{4500,3500},{5500,3500},{5500,4500},{4500,4500}};
    action=polygon(&f,EDITOR_TOOL_SLAB_REGION,crosses_void,4,0);
    assert(action.kind==EDITOR_ACTION_COMMAND&&!execute(&f,&action,&result));
    assert(f.editor.slab_region_tool.vertex_count==4&&slab->definition.regions.count==1&&
        slab->definition.penetrations.count==1&&f.history.count==1);
    fixture_destroy(&f);
}

static void test_enter_commit_and_parent_lock(void)
{
    Fixture f;fixture_init(&f);
    DomainId second=sitehelper_project_add_slab(&f.project,f.storey_id,outer,4,100,0);
    assert(second!=DOMAIN_ID_INVALID);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_REGION));
    (void)click(&f,4000,3000);
    assert(f.editor.slab_region_tool.slab_id==second); /* later render order */
    (void)click(&f,5000,3000);(void)click(&f,5000,4000);(void)click(&f,4000,4000);
    assert(f.editor.slab_region_tool.slab_id==second);
    AppInput input={0};PlatformEvent enter={.type=PLATFORM_EVENT_KEY_DOWN,
        .data.key_down={.key=PLATFORM_KEY_ENTER,.modifiers=PLATFORM_MODIFIER_NONE}};
    EditorAction action={0};assert(app_input_route(&input,&f.editor,&enter,&action)==APP_INPUT_COMMAND);
    SiteHelperCommandResult result;assert(execute(&f,&action,&result));
    assert(result.data.slab_feature.slab_id==second&&
        sitehelper_project_find_slab_by_id(&f.project,second)->definition.regions.count==1);
    fixture_destroy(&f);
}

static void test_rebate_creation_and_failure(void)
{
    Fixture f;fixture_init(&f);DomainId next=f.project.domain_ids.next;
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_EDGE_REBATE));
    EditorAction action=click(&f,3000,15);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_edge_rebate_tool.has_start&&
        f.editor.slab_edge_rebate_tool.edge_index==0&&
        f.editor.slab_edge_rebate_tool.start_u_mm==3000);
    sitehelper_editor_pointer_move_in_project(&f.editor,&f.project,(Vec2){1000,10});
    DomainId preview_slab;size_t preview_edge;PlanPoint start,end;int has_end;
    assert(sitehelper_editor_get_slab_rebate_preview(&f.editor,&preview_slab,
        &preview_edge,&start,&end,&has_end));
    assert(preview_slab==f.slab_id&&preview_edge==0&&has_end&&end.x==1000&&end.y==0);
    action=click(&f,1000,10);assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(action.command.data.add_slab_edge_rebate.start_offset_mm==1000&&
        action.command.data.add_slab_edge_rebate.end_offset_mm==3000&&
        action.command.data.add_slab_edge_rebate.width_mm==100&&
        action.command.data.add_slab_edge_rebate.depth_mm==20);
    SiteHelperCommandResult result;assert(execute(&f,&action,&result));
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    assert(slab->definition.edge_rebates.count==1&&f.history.count==1&&
        f.project.domain_ids.next==next&&
        f.editor.selection.kind==EDITOR_SELECTION_SLAB_EDGE_REBATE);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(sitehelper_command_history_redo(&f.history,&f.project));

    (void)click(&f,2000,5);action=click(&f,2000,5);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_edge_rebate_tool.has_start);
    action=click(&f,10000,1000); /* a different edge */
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_edge_rebate_tool.has_start);
    action=click(&f,4000,5);assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(!execute(&f,&action,&result)); /* overlaps existing interval */
    assert(slab->definition.edge_rebates.count==1&&f.history.count==1&&
        f.editor.slab_edge_rebate_tool.has_start&&f.project.domain_ids.next==next);
    assert(sitehelper_editor_cancel_tool_interaction(&f.editor));

    /* The implicit closing edge remains eligible and its U runs vertex 3 -> 0. */
    (void)click(&f,5,7000);action=click(&f,5,5000);
    assert(action.kind==EDITOR_ACTION_COMMAND&&
        action.command.data.add_slab_edge_rebate.edge_index==3&&
        action.command.data.add_slab_edge_rebate.start_offset_mm==1000&&
        action.command.data.add_slab_edge_rebate.end_offset_mm==3000);
    assert(execute(&f,&action,&result));
    fixture_destroy(&f);
}

static void test_rebate_diagonal_tool(void)
{
    const PlanPosition triangle[]={{0,0},{300,400},{600,0}};
    SiteHelperProject project;sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId slab_id=sitehelper_project_add_slab(&project,storey,triangle,3,100,0);
    SiteHelperEditor editor;sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_SLAB_EDGE_REBATE));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){240,320},&action));
    assert(editor.slab_edge_rebate_tool.slab_id==slab_id&&
        editor.slab_edge_rebate_tool.edge_index==0&&editor.slab_edge_rebate_tool.start_u_mm==400);
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){60,80},&action));
    assert(action.kind==EDITOR_ACTION_COMMAND&&
        action.command.data.add_slab_edge_rebate.start_offset_mm==100&&
        action.command.data.add_slab_edge_rebate.end_offset_mm==400);
    editor_action_destroy(&action);sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_transient_invalidation_and_parent_loss(void)
{
    Fixture f;fixture_init(&f);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_PENETRATION));
    (void)click(&f,1000,1000);assert(f.editor.slab_penetration_tool.vertex_count==1);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_REGION));
    assert(f.editor.slab_penetration_tool.vertex_count==0);
    (void)click(&f,1000,1000);assert(sitehelper_editor_set_active_view(&f.editor,
        EDITOR_VIEW_WALL_ELEVATION));assert(f.editor.slab_region_tool.vertex_count==0&&
        f.editor.active_tool==EDITOR_TOOL_SELECT);
    assert(sitehelper_editor_set_active_view(&f.editor,EDITOR_VIEW_PLAN));
    DomainId other=sitehelper_project_add_storey(&f.project,6000);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_PENETRATION));
    (void)click(&f,1000,1000);assert(f.editor.slab_penetration_tool.vertex_count==1);
    assert(sitehelper_editor_set_current_storey(&f.editor,&f.project,other));
    assert(f.editor.slab_penetration_tool.vertex_count==0);
    assert(sitehelper_editor_set_current_storey(&f.editor,&f.project,f.storey_id));
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_EDGE_REBATE));
    (void)click(&f,1000,0);assert(f.editor.slab_edge_rebate_tool.has_start);
    sitehelper_editor_project_replaced(&f.editor,&f.project);
    assert(!f.editor.slab_edge_rebate_tool.has_start);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_PENETRATION));
    (void)click(&f,1000,1000);assert(sitehelper_project_remove_slab_by_id(&f.project,f.slab_id));
    (void)click(&f,2000,1000); /* click path detects the lost parent */
    assert(f.editor.slab_penetration_tool.vertex_count==0);
    DomainId replacement=sitehelper_project_add_slab(&f.project,f.storey_id,outer,4,100,0);
    assert(replacement!=DOMAIN_ID_INVALID);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_EDGE_REBATE));
    (void)click(&f,1000,0);assert(f.editor.slab_edge_rebate_tool.has_start);
    assert(sitehelper_project_remove_slab_by_id(&f.project,replacement));
    (void)click(&f,2000,0);assert(!f.editor.slab_edge_rebate_tool.has_start);
    fixture_destroy(&f);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_polygon_allocation_failures(void)
{
    Fixture f;fixture_init(&f);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_PENETRATION));
    fail_after=0;EditorAction action={0};
    assert(!sitehelper_editor_primary_action_in_project(&f.editor,&f.project,
        (Vec2){1000,1000},&action));
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_penetration_tool.vertex_count==0);
    fail_after=SIZE_MAX;(void)click(&f,1000,1000);(void)click(&f,2000,1000);
    (void)click(&f,2000,2000);(void)click(&f,1000,2000);
    PlanPosition *saved=f.editor.slab_penetration_tool.vertices;
    for(size_t failure=0;failure<2;failure++) {
        fail_after=failure;assert(!sitehelper_editor_create_active_polygon_action(&f.editor,&action));
        assert(f.editor.slab_penetration_tool.vertices==saved&&
            f.editor.slab_penetration_tool.vertex_count==4&&f.history.count==0);
    }
    fail_after=SIZE_MAX;assert(sitehelper_editor_create_active_polygon_action(&f.editor,&action));
    editor_action_destroy(&action);

    /* History clones the owning command, reserves its entry, then the slab
     * domain allocates the committed polygon. Sweep all three failure points. */
    for(size_t failure=0;failure<3;failure++) {
        assert(sitehelper_editor_create_active_polygon_action(&f.editor,&action));
        SiteHelperCommandResult result;fail_after=failure;
        assert(!sitehelper_command_history_execute(&f.history,&f.project,&action.command,&result));
        assert(f.history.count==0&&f.history.cursor==0&&
            f.editor.slab_penetration_tool.vertex_count==4&&
            sitehelper_project_find_slab_by_id(&f.project,f.slab_id)->definition.penetrations.count==0);
        editor_action_destroy(&action);
        /* Failure 1 grows history capacity before the domain allocation sweep;
         * reset it so each iteration sees the same three allocation sites. */
        sitehelper_command_history_destroy(&f.history);
        sitehelper_command_history_init(&f.history);
        fail_after=SIZE_MAX;
    }
    SiteHelperCommandResult result;
    assert(sitehelper_editor_create_active_polygon_action(&f.editor,&action));
    assert(execute(&f,&action,&result));

    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_REGION));
    fail_after=0;action=(EditorAction){0};
    assert(!sitehelper_editor_primary_action_in_project(&f.editor,&f.project,
        (Vec2){4000,3000},&action));
    assert(f.editor.slab_region_tool.vertex_count==0);
    fail_after=SIZE_MAX;
    (void)click(&f,4000,3000);(void)click(&f,5000,3000);
    (void)click(&f,5000,4000);(void)click(&f,4000,4000);
    for(size_t failure=0;failure<2;failure++) {
        fail_after=failure;assert(!sitehelper_editor_create_active_polygon_action(&f.editor,&action));
        assert(f.editor.slab_region_tool.vertex_count==4);
    }
    fail_after=SIZE_MAX;assert(sitehelper_editor_create_active_polygon_action(&f.editor,&action));
    editor_action_destroy(&action);

    SlabPolygonFeatureTool growth;slab_polygon_feature_tool_init(&growth);
    slab_polygon_feature_tool_activate(&growth);fail_after=SIZE_MAX;
    assert(slab_polygon_feature_tool_begin(&growth,1,2,(PlanPosition){0,0},0,100));
    for(int i=1;i<8;i++) { assert(slab_polygon_feature_tool_append(&growth,
        (PlanPosition){i,0})); }
    PlanPosition *old_vertices=growth.vertices;fail_after=0;
    assert(!slab_polygon_feature_tool_append(&growth,(PlanPosition){8,0}));
    assert(growth.vertices==old_vertices&&growth.vertex_count==8);
    fail_after=SIZE_MAX;assert(slab_polygon_feature_tool_append(&growth,(PlanPosition){8,0}));
    slab_polygon_feature_tool_destroy(&growth);
    fixture_destroy(&f);
}
#endif

int main(void)
{
    test_penetration_creation_and_failures();
    test_region_creation_and_boundary();
    test_enter_commit_and_parent_lock();
    test_rebate_creation_and_failure();
    test_rebate_diagonal_tool();
    test_transient_invalidation_and_parent_loss();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_polygon_allocation_failures();
#endif
    return 0;
}
