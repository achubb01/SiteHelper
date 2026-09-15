#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "app_input.h"
#include "app_properties_panel.h"
#include "command_history.h"
#include "slab.h"

static AppInputResult key(AppInput *input,SiteHelperEditor *editor,
    const SiteHelperProject *project,PlatformKey keycode,EditorAction *action)
{
    PlatformEvent event={.type=PLATFORM_EVENT_KEY_DOWN,
        .data.key_down={.key=keycode,.modifiers=PLATFORM_MODIFIER_NONE}};
    return app_input_route_in_project(input,editor,project,&event,action);
}
static void text(AppInput *input,SiteHelperEditor *editor,
    const SiteHelperProject *project,const char *value)
{
    PlatformEvent event={.type=PLATFORM_EVENT_TEXT_INPUT};
    snprintf(event.data.text_input.text,sizeof event.data.text_input.text,"%s",value);
    EditorAction action={0};
    assert(app_input_route_in_project(input,editor,project,&event,&action)==APP_INPUT_CONSUMED);
}
static void erase(AppInput *input,SiteHelperEditor *editor,
    const SiteHelperProject *project,EditorAction *action)
{
    while(input->text.length){assert(key(input,editor,project,PLATFORM_KEY_BACKSPACE,action)==APP_INPUT_CONSUMED);}
}

int main(void)
{
    const PlanPosition outer[]={{0,0},{10000,0},{10000,8000},{0,8000}};
    const PlanPosition region_outline[]={{0,0},{3000,0},{3000,2000},{0,2000}};
    const PlanPosition hole[]={{5000,3000},{6000,3000},{6000,4000},{5000,4000}};
    SiteHelperProject project;sitehelper_project_init(&project);
    DomainId storey=sitehelper_project_add_storey(&project,0);
    DomainId slab_id=sitehelper_project_add_slab(&project,storey,outer,4,100,-10);
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);
    assert(slab_add_region(slab,region_outline,4,-50,75)==SLAB_SUCCESS);
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,1000,3000,100,20)==SLAB_SUCCESS);
    SiteHelperEditor editor;sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey));
    SiteHelperCommandHistory history;sitehelper_command_history_init(&history);
    AppInput input={0};EditorAction action={0};SiteHelperCommandResult result;

    editor_selection_set_slab(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id);
    AppPropertiesPanel panel;assert(app_properties_panel_build(&editor,&project,&panel));
    assert(strcmp(panel.title,"Slab")==0&&panel.field_count==2&&
        panel.fields[0].property==EDITOR_PROPERTY_SLAB_THICKNESS);
    Rect2 bounds={{100,0},260,600};EditorProperty property;
    Rect2 first=app_properties_panel_field_bounds(bounds,0);
    assert(app_properties_panel_hit(&panel,bounds,(Vec2){first.position.x+2,first.position.y+2},&property));
    assert(property==EDITOR_PROPERTY_SLAB_THICKNESS);

    assert(app_input_begin_property(&input,&editor,&project,property));
    assert(input.focus==APP_KEYBOARD_FOCUS_PROPERTY_MM&&strcmp(input.text.text,"100")==0&&
        input.replace_on_next_text_input&&app_input_valid(&input));
    /* Normal CAD typing replaces the seeded current value without requiring
     * manual Backspace/Delete of every digit. */
    text(&input,&editor,&project,"125mm");
    assert(strcmp(input.text.text,"125mm")==0&&!input.replace_on_next_text_input&&
        input.millimetres==125&&app_input_valid(&input));
    assert(key(&input,&editor,&project,PLATFORM_KEY_ENTER,&action)==APP_INPUT_COMMAND);
    assert(action.command.type==SITEHELPER_COMMAND_EDIT_SLAB);
    assert(sitehelper_command_history_execute(&history,&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);editor_action_destroy(&action);
    sitehelper_editor_reconcile(&editor,&project);app_input_cancel(&input,&editor);
    assert(slab->definition.thickness_mm==125&&history.count==1);

    /* Parsed integers can still fail authoritative domain validation; the draft remains. */
    assert(app_input_begin_property(&input,&editor,&project,EDITOR_PROPERTY_SLAB_THICKNESS));
    text(&input,&editor,&project,"0");
    assert(key(&input,&editor,&project,PLATFORM_KEY_ENTER,&action)==APP_INPUT_COMMAND);
    assert(!sitehelper_command_history_execute(&history,&project,&action.command,&result));
    editor_action_destroy(&action);input.command_failed=1;
    assert(input.focus==APP_KEYBOARD_FOCUS_PROPERTY_MM&&strcmp(input.text.text,"0")==0&&
        strstr(app_input_feedback(&input),"rejected")!=NULL&&history.count==1);
    input.command_failed=0;erase(&input,&editor,&project,&action);text(&input,&editor,&project,"0.15m");
    assert(input.millimetres==150&&app_input_valid(&input));

    /* Focus is bound to the value selection, not merely the field enum. */
    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_REGION,0);
    app_input_refresh_in_project(&input,&editor,&project);
    assert(input.focus==APP_KEYBOARD_FOCUS_NONE);
    assert(app_input_begin_property(&input,&editor,&project,EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL));
    assert(strcmp(input.text.text,"-50")==0);
    /* Enter without a changed value closes editing without polluting history. */
    size_t history_count=history.count,history_cursor=history.cursor;
    assert(key(&input,&editor,&project,PLATFORM_KEY_ENTER,&action)==APP_INPUT_CONSUMED);
    assert(input.focus==APP_KEYBOARD_FOCUS_NONE&&history.count==history_count&&
        history.cursor==history_cursor&&action.kind==EDITOR_ACTION_NONE);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_EDGE_REBATE,0);
    assert(app_properties_panel_build(&editor,&project,&panel)&&panel.field_count==4);
    assert(app_input_begin_property(&input,&editor,&project,EDITOR_PROPERTY_SLAB_REBATE_START));
    assert(strcmp(input.text.text,"1000")==0);app_input_cancel(&input,&editor);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id,
        EDITOR_SELECTION_SLAB_PENETRATION,0);
    assert(app_properties_panel_build(&editor,&project,&panel)&&panel.field_count==0&&panel.note!=NULL);
    assert(!app_input_begin_property(&input,&editor,&project,EDITOR_PROPERTY_SLAB_THICKNESS));

    sitehelper_command_history_destroy(&history);sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);return 0;
}
