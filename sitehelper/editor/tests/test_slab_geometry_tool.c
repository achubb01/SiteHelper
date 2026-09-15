#include <assert.h>

#include "command_history.h"
#include "sitehelper_editor.h"
#include "slab.h"

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition hole[]={{1000,1000},{2500,1000},{2500,2500},{1000,2500}};
static const PlanPosition region[]={{5000,1000},{7000,1000},{7000,3000},{5000,3000}};

typedef struct {
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    DomainId storey_id;
    DomainId slab_id;
} Fixture;

static void fixture_init(Fixture *f)
{
    sitehelper_project_init(&f->project);
    f->storey_id=sitehelper_project_add_storey(&f->project,0);
    f->slab_id=sitehelper_project_add_slab(&f->project,f->storey_id,outer,4,100,0);
    assert(f->slab_id!=DOMAIN_ID_INVALID);
    Slab *slab=sitehelper_project_find_slab_by_id(&f->project,f->slab_id);
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(slab,region,4,-20,80)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,0,7000,100,20)==SLAB_SUCCESS);
    sitehelper_editor_init(&f->editor);
    assert(sitehelper_editor_set_current_storey(&f->editor,&f->project,f->storey_id));
    sitehelper_command_history_init(&f->history);
}

static void fixture_destroy(Fixture *f)
{
    sitehelper_command_history_destroy(&f->history);
    sitehelper_editor_destroy(&f->editor);
    sitehelper_project_destroy(&f->project);
}

static EditorAction click(Fixture *f,double x,double y)
{
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&f->editor,&f->project,
        (Vec2){x,y},&action));
    return action;
}

static int execute(Fixture *f,EditorAction *action)
{
    SiteHelperCommandResult result;
    int ok=sitehelper_command_history_execute(&f->history,&f->project,&action->command,&result);
    if(ok){
        sitehelper_editor_complete_action(&f->editor,action,&result);
        sitehelper_editor_reconcile(&f->editor,&f->project);
    }
    editor_action_destroy(action);
    return ok;
}

static void select_outer(Fixture *f)
{
    editor_selection_set_slab(&f->editor.selection,EDITOR_SELECTION_SCOPE_PLAN,f->slab_id);
    assert(sitehelper_editor_set_active_tool(&f->editor,EDITOR_TOOL_SLAB_GEOMETRY));
}

static void test_outer_move_preview_and_history(void)
{
    Fixture f;fixture_init(&f);select_outer(&f);
    DomainId next=f.project.domain_ids.next;
    EditorSlabGeometryOverlay overlay;
    assert(sitehelper_editor_get_slab_geometry_overlay(&f.editor,&f.project,&overlay));
    assert(overlay.kind==EDITOR_SLAB_GEOMETRY_OUTLINE&&overlay.vertex_count==4&&
        overlay.active_vertex_index==SIZE_MAX);

    EditorAction action=click(&f,10010,5);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_geometry_tool.has_vertex&&
        f.editor.slab_geometry_tool.vertex_index==1&&f.history.count==0);
    sitehelper_editor_pointer_move_in_project(&f.editor,&f.project,(Vec2){11010,5});
    assert(sitehelper_editor_get_slab_geometry_overlay(&f.editor,&f.project,&overlay));
    assert(overlay.active_vertex_index==1&&overlay.has_preview&&
        overlay.preview.x==11000&&overlay.preview.y==0);
    action=click(&f,11010,5);
    assert(action.kind==EDITOR_ACTION_COMMAND&&
        action.command.type==SITEHELPER_COMMAND_MOVE_SLAB_VERTEX);
    assert(execute(&f,&action));
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    assert(slab->definition.outline.vertices[1].x==11000&&f.history.count==1&&
        f.project.domain_ids.next==next&&f.editor.active_tool==EDITOR_TOOL_SLAB_GEOMETRY&&
        !f.editor.slab_geometry_tool.has_vertex&&f.editor.selection.kind==EDITOR_SELECTION_SLAB);
    assert(sitehelper_command_history_undo(&f.history,&f.project));
    assert(slab->definition.outline.vertices[1].x==10000);
    assert(sitehelper_command_history_redo(&f.history,&f.project));
    assert(slab->definition.outline.vertices[1].x==11000);
    fixture_destroy(&f);
}


static void test_unsnapped_preview_matches_commit_candidate(void)
{
    Fixture f;fixture_init(&f);select_outer(&f);
    f.editor.snap.settings.grid_enabled=0;
    EditorAction action=click(&f,10000,0);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_geometry_tool.has_vertex);
    sitehelper_editor_pointer_move_in_project(&f.editor,&f.project,(Vec2){11000.9,5.9});
    EditorSlabGeometryOverlay overlay;
    assert(sitehelper_editor_get_slab_geometry_overlay(&f.editor,&f.project,&overlay));
    assert(overlay.has_preview&&overlay.preview.x==11000&&overlay.preview.y==5);
    action=click(&f,11000.9,5.9);
    assert(action.kind==EDITOR_ACTION_COMMAND&&
        action.command.data.move_slab_vertex.new_position.x==11000&&
        action.command.data.move_slab_vertex.new_position.y==5);
    editor_action_destroy(&action);
    fixture_destroy(&f);
}

static void test_invalid_move_retains_interaction(void)
{
    Fixture f;fixture_init(&f);select_outer(&f);
    EditorAction action=click(&f,10000,0);
    assert(action.kind==EDITOR_ACTION_NONE&&f.editor.slab_geometry_tool.has_vertex);
    action=click(&f,6000,0);
    assert(action.kind==EDITOR_ACTION_COMMAND);
    assert(!execute(&f,&action));
    Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    assert(slab->definition.outline.vertices[1].x==10000&&f.history.count==0&&
        f.editor.slab_geometry_tool.has_vertex);
    assert(sitehelper_editor_cancel_tool_interaction(&f.editor));
    assert(!f.editor.slab_geometry_tool.has_vertex);

    /* A second click at the original point is an explicit no-op/cancel. */
    action=click(&f,10000,0);assert(f.editor.slab_geometry_tool.has_vertex);
    action=click(&f,10000,0);assert(action.kind==EDITOR_ACTION_NONE&&
        !f.editor.slab_geometry_tool.has_vertex&&f.history.count==0);
    fixture_destroy(&f);
}

static void test_subordinate_targets_and_rebate_exclusion(void)
{
    Fixture f;fixture_init(&f);Slab *slab=sitehelper_project_find_slab_by_id(&f.project,f.slab_id);
    editor_selection_set_slab_feature(&f.editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        f.slab_id,EDITOR_SELECTION_SLAB_PENETRATION,0);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_GEOMETRY));
    EditorAction action=click(&f,2500,1000);assert(f.editor.slab_geometry_tool.has_vertex);
    action=click(&f,3000,1000);assert(action.kind==EDITOR_ACTION_COMMAND&&execute(&f,&action));
    assert(slab->definition.penetrations.items[0].outline.vertices[1].x==3000&&
        f.editor.selection.kind==EDITOR_SELECTION_SLAB_PENETRATION);

    editor_selection_set_slab_feature(&f.editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        f.slab_id,EDITOR_SELECTION_SLAB_REGION,0);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_GEOMETRY));
    action=click(&f,7000,3000);assert(f.editor.slab_geometry_tool.has_vertex);
    action=click(&f,7500,3000);assert(action.kind==EDITOR_ACTION_COMMAND&&execute(&f,&action));
    assert(slab->definition.regions.items[0].outline.vertices[2].x==7500&&
        f.editor.selection.kind==EDITOR_SELECTION_SLAB_REGION);

    editor_selection_set_slab_feature(&f.editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        f.slab_id,EDITOR_SELECTION_SLAB_EDGE_REBATE,0);
    assert(sitehelper_editor_set_active_tool(&f.editor,EDITOR_TOOL_SLAB_GEOMETRY));
    EditorSlabGeometryOverlay overlay;
    assert(!sitehelper_editor_get_slab_geometry_overlay(&f.editor,&f.project,&overlay));
    action=click(&f,1000,0);
    assert(action.kind==EDITOR_ACTION_NONE&&!f.editor.slab_geometry_tool.has_vertex);
    fixture_destroy(&f);
}

static void test_context_invalidation_and_parent_loss(void)
{
    Fixture f;fixture_init(&f);select_outer(&f);
    (void)click(&f,10000,0);assert(f.editor.slab_geometry_tool.has_vertex);
    assert(sitehelper_editor_set_active_view(&f.editor,EDITOR_VIEW_WALL_ELEVATION));
    assert(!f.editor.slab_geometry_tool.has_vertex&&f.editor.active_tool==EDITOR_TOOL_SELECT);
    assert(sitehelper_editor_set_active_view(&f.editor,EDITOR_VIEW_PLAN));

    select_outer(&f);(void)click(&f,10000,0);
    assert(sitehelper_project_remove_slab_by_id(&f.project,f.slab_id));
    sitehelper_editor_pointer_move_in_project(&f.editor,&f.project,(Vec2){11000,0});
    assert(!f.editor.slab_geometry_tool.has_vertex);
    fixture_destroy(&f);
}

int main(void)
{
    test_outer_move_preview_and_history();
    test_unsnapped_preview_matches_commit_candidate();
    test_invalid_move_retains_interaction();
    test_subordinate_targets_and_rebate_exclusion();
    test_context_invalidation_and_parent_loss();
    return 0;
}
