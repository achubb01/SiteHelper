#include <assert.h>

#include "app_input.h"
#include "command_history.h"
#include "slab.h"

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition hole0[]={{1000,1000},{1500,1000},{1500,1500},{1000,1500}};
static const PlanPosition hole1[]={{2000,1000},{2500,1000},{2500,1500},{2000,1500}};
static const PlanPosition region[]={{4000,1000},{5000,1000},{5000,2000},{4000,2000}};

static AppInputResult delete_key(AppInput *input,SiteHelperEditor *editor,
    EditorAction *action)
{
    PlatformEvent event={.type=PLATFORM_EVENT_KEY_DOWN,
        .data.key_down={.key=PLATFORM_KEY_DELETE,.modifiers=PLATFORM_MODIFIER_NONE}};
    return app_input_route(input,editor,&event,action);
}

static void execute_delete(SiteHelperProject *project,SiteHelperEditor *editor,
    SiteHelperCommandHistory *history,AppInput *input,SiteHelperCommandType expected)
{
    EditorAction action={0};SiteHelperCommandResult result;
    assert(delete_key(input,editor,&action)==APP_INPUT_COMMAND);
    assert(action.kind==EDITOR_ACTION_COMMAND&&action.command.type==expected);
    assert(sitehelper_command_history_execute(history,project,&action.command,&result));
    sitehelper_editor_complete_action(editor,&action,&result);
    assert(editor->selection.kind==EDITOR_SELECTION_NONE);
    sitehelper_editor_reconcile(editor,project);
    editor_action_destroy(&action);
}

int main(void)
{
    SiteHelperProject project;sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId slab_id=sitehelper_project_add_slab(&project,storey,outer,4,100,0);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(slab_add_penetration(slab,hole0,4)==SLAB_SUCCESS);
    assert(slab_add_penetration(slab,hole1,4)==SLAB_SUCCESS);
    assert(slab_add_region(slab,region,4,-50,75)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,0,1000,100,20)==SLAB_SUCCESS);
    DomainId next=project.domain_ids.next;

    SiteHelperEditor editor;sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    AppInput input={0};

    /* Deleting index zero shifts another feature into zero; completion must
     * clear the ephemeral selection before reconciliation can retarget it. */
    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        slab_id,EDITOR_SELECTION_SLAB_PENETRATION,0);
    execute_delete(&project,&editor,&history,&input,
        SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION);
    assert(slab->definition.penetrations.count==1&&slab->definition.regions.count==1&&
        slab->definition.edge_rebates.count==1&&sitehelper_project_find_slab_by_id(&project,slab_id));
    assert(project.domain_ids.next==next&&sitehelper_command_history_undo(&history,&project));
    assert(slab->definition.penetrations.count==2);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        slab_id,EDITOR_SELECTION_SLAB_REGION,0);
    execute_delete(&project,&editor,&history,&input,SITEHELPER_COMMAND_DELETE_SLAB_REGION);
    assert(slab->definition.regions.count==0&&slab->definition.penetrations.count==2&&
        slab->definition.edge_rebates.count==1&&project.domain_ids.next==next);
    assert(sitehelper_command_history_undo(&history,&project)&&slab->definition.regions.count==1);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        slab_id,EDITOR_SELECTION_SLAB_EDGE_REBATE,0);
    execute_delete(&project,&editor,&history,&input,SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE);
    assert(slab->definition.edge_rebates.count==0&&slab->definition.penetrations.count==2&&
        slab->definition.regions.count==1&&project.domain_ids.next==next);
    assert(sitehelper_command_history_undo(&history,&project)&&slab->definition.edge_rebates.count==1);

    sitehelper_command_history_destroy(&history);sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);return 0;
}
