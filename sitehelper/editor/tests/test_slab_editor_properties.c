#include <assert.h>
#include "editor_properties.h"
#include "command_history.h"
#include "slab.h"

static const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
static const PlanPosition region[]={{0,0},{3000,0},{3000,2000},{0,2000}};
static const PlanPosition hole[]={{5000,3000},{6000,3000},{6000,4000},{5000,4000}};

int main(void)
{
    SiteHelperProject project;sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId slab_id=sitehelper_project_add_slab(&project,storey,outer,4,100,-10);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(slab_add_region(slab,region,4,-50,75)==SLAB_SUCCESS);
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,1000,3000,100,20)==SLAB_SUCCESS);

    SiteHelperEditor editor;sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    EditorProperties properties;int value;SiteHelperCommand command;SiteHelperCommandResult result;

    editor_selection_set_slab(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id);
    assert(sitehelper_editor_inspect_properties(&editor,&project,&properties));
    assert(properties.kind==EDITOR_SELECTION_SLAB&&properties.data.slab.thickness_mm==100&&
        properties.data.slab.top_level_offset_mm==-10&&properties.data.slab.vertex_count==4);
    assert(sitehelper_editor_property_millimetres(&editor,&project,EDITOR_PROPERTY_SLAB_THICKNESS,&value)&&value==100);
    EditorPropertyEdit edit={.property=EDITOR_PROPERTY_SLAB_THICKNESS,.value.millimetres=125};
    assert(sitehelper_editor_create_property_command(&editor,&project,&edit,&command));
    assert(command.type==SITEHELPER_COMMAND_EDIT_SLAB);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);sitehelper_editor_reconcile(&editor,&project);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB&&slab->definition.thickness_mm==125);
    assert(sitehelper_command_history_undo(&history,&project)&&slab->definition.thickness_mm==100);
    assert(sitehelper_command_history_redo(&history,&project)&&slab->definition.thickness_mm==125);

    edit=(EditorPropertyEdit){.property=EDITOR_PROPERTY_SLAB_THICKNESS,.value.millimetres=0};
    assert(sitehelper_editor_create_property_command(&editor,&project,&edit,&command));
    assert(!sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);assert(slab->definition.thickness_mm==125);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_REGION,0);
    assert(sitehelper_editor_inspect_properties(&editor,&project,&properties));
    assert(properties.kind==EDITOR_SELECTION_SLAB_REGION&&properties.data.slab_region.top_level_offset_mm==-50&&
        properties.data.slab_region.thickness_mm==75);
    edit=(EditorPropertyEdit){.property=EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL,.value.millimetres=-80};
    assert(sitehelper_editor_create_property_command(&editor,&project,&edit,&command));
    assert(command.type==SITEHELPER_COMMAND_EDIT_SLAB_REGION);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);sitehelper_editor_reconcile(&editor,&project);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB_REGION&&
        slab->definition.regions.items[0].top_level_offset_mm==-80);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_EDGE_REBATE,0);
    assert(sitehelper_editor_inspect_properties(&editor,&project,&properties));
    assert(properties.kind==EDITOR_SELECTION_SLAB_EDGE_REBATE&&
        properties.data.slab_edge_rebate.definition.edge_index==0&&
        properties.data.slab_edge_rebate.length_mm==2000);
    edit=(EditorPropertyEdit){.property=EDITOR_PROPERTY_SLAB_REBATE_WIDTH,.value.millimetres=150};
    assert(sitehelper_editor_create_property_command(&editor,&project,&edit,&command));
    assert(command.type==SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE);
    assert(sitehelper_command_history_execute(&history,&project,&command,&result));
    sitehelper_command_destroy(&command);sitehelper_editor_reconcile(&editor,&project);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB_EDGE_REBATE&&
        slab->definition.edge_rebates.items[0].width_mm==150);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_PENETRATION,0);
    assert(sitehelper_editor_inspect_properties(&editor,&project,&properties)&&
        properties.kind==EDITOR_SELECTION_SLAB_PENETRATION&&properties.data.slab_penetration.vertex_count==4);
    assert(!sitehelper_editor_property_millimetres(&editor,&project,EDITOR_PROPERTY_SLAB_THICKNESS,&value));
    edit=(EditorPropertyEdit){.property=EDITOR_PROPERTY_SLAB_THICKNESS,.value.millimetres=200};
    assert(!sitehelper_editor_create_property_command(&editor,&project,&edit,&command));

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_REGION,99);
    assert(!sitehelper_editor_inspect_properties(&editor,&project,&properties));

    sitehelper_command_history_destroy(&history);sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
    return 0;
}
