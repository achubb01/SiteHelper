#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sitehelper_editor.h"
#include "slab.h"

static DomainId make_slab_project(SiteHelperProject *project, DomainId *storey_id)
{
    sitehelper_project_init(project);
    *storey_id=sitehelper_project_add_storey(project,0); assert(*storey_id);
    const PlanPosition outer[]={{0,0},{1000,0},{1000,1000},{0,1000}};
    DomainId slab_id=sitehelper_project_add_slab(project,*storey_id,outer,4,100,0);
    assert(slab_id);
    Slab *slab=sitehelper_project_find_slab_by_id(project,slab_id); assert(slab);
    const PlanPosition hole[]={{400,400},{600,400},{600,600},{400,600}};
    const PlanPosition region[]={{100,100},{900,100},{900,900},{100,900}};
    assert(slab_add_penetration(slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(slab,region,4,-50,80)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(slab,0,0,300,100,20)==SLAB_SUCCESS);
    return slab_id;
}

static void click(SiteHelperEditor *editor, const SiteHelperProject *project,
    double x, double y)
{
    EditorAction action;
    assert(sitehelper_editor_primary_action_in_project(editor,project,
        (Vec2){x,y},&action));
    assert(action.kind==EDITOR_ACTION_NONE);
}

static void test_plan_clicks_select_slab_features_without_mutation(void)
{
    SiteHelperProject project; DomainId storey_id;
    DomainId slab_id=make_slab_project(&project,&storey_id);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    DomainId next_id=project.domain_ids.next;
    const Slab *slab=sitehelper_project_find_slab_by_id_const(&project,slab_id);
    const PlanPosition *outer_pointer=slab->definition.outline.vertices;
    Slab slab_bytes=*slab;
    PlanPosition outer_before[4],hole_before[4],region_before[4];
    memcpy(outer_before,slab->definition.outline.vertices,sizeof outer_before);
    memcpy(hole_before,slab->definition.penetrations.items[0].outline.vertices,
        sizeof hole_before);
    memcpy(region_before,slab->definition.regions.items[0].outline.vertices,
        sizeof region_before);

    click(&editor,&project,50,5);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB_EDGE_REBATE);
    assert(editor.selection.slab_id==slab_id&&editor.selection.slab_feature_index==0);
    click(&editor,&project,500,500);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB_PENETRATION);
    click(&editor,&project,200,200);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB_REGION);
    click(&editor,&project,990,990);
    assert(editor.selection.kind==EDITOR_SELECTION_SLAB&&editor.selection.slab_id==slab_id);
    click(&editor,&project,2000,2000);
    assert(editor_selection_is_empty(&editor.selection));
    assert(project.domain_ids.next==next_id);
    slab=sitehelper_project_find_slab_by_id_const(&project,slab_id);
    assert(slab->definition.outline.vertices==outer_pointer);
    assert(memcmp(slab,&slab_bytes,sizeof slab_bytes)==0);
    assert(memcmp(slab->definition.outline.vertices,outer_before,sizeof outer_before)==0);
    assert(memcmp(slab->definition.penetrations.items[0].outline.vertices,hole_before,
        sizeof hole_before)==0);
    assert(memcmp(slab->definition.regions.items[0].outline.vertices,region_before,
        sizeof region_before)==0);
    assert(slab->definition.penetrations.count==1&&slab->definition.regions.count==1&&
        slab->definition.edge_rebates.count==1);
    sitehelper_project_destroy(&project);
}

static void test_reconciliation_clears_ephemeral_references(void)
{
    SiteHelperProject project; DomainId storey_id;
    DomainId slab_id=make_slab_project(&project,&storey_id);
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    Slab *slab=sitehelper_project_find_slab_by_id(&project,slab_id);

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        slab_id,EDITOR_SELECTION_SLAB_REGION,0);
    assert(slab_remove_region(slab,0)==SLAB_SUCCESS);
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor_selection_is_empty(&editor.selection));

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        slab_id,EDITOR_SELECTION_SLAB_EDGE_REBATE,8);
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor_selection_is_empty(&editor.selection));

    editor_selection_set_slab_feature(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,
        slab_id,EDITOR_SELECTION_SLAB_PENETRATION,0);
    assert(sitehelper_project_remove_slab_by_id(&project,slab_id));
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor_selection_is_empty(&editor.selection));

    const PlanPosition outer[]={{0,0},{100,0},{100,100},{0,100}};
    slab_id=sitehelper_project_add_slab(&project,storey_id,outer,4,100,0); assert(slab_id);
    editor_selection_set_slab(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id);
    DomainId other_storey=sitehelper_project_add_storey(&project,3000); assert(other_storey);
    assert(sitehelper_editor_set_current_storey(&editor,&project,other_storey));
    assert(editor_selection_is_empty(&editor.selection));

    SiteHelperProject replacement; sitehelper_project_init(&replacement);
    DomainId replacement_storey=sitehelper_project_add_storey(&replacement,0);
    assert(replacement_storey==storey_id);
    assert(sitehelper_project_add_room(&replacement,replacement_storey)==2);
    const PlanPosition replacement_outer[]={{0,0},{500,0},{500,500},{0,500}};
    DomainId replacement_slab=sitehelper_project_add_slab(&replacement,
        replacement_storey,replacement_outer,4,120,-20);
    assert(replacement_slab==slab_id);
    editor.current_storey_id=storey_id;
    editor_selection_set_slab(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,slab_id);
    sitehelper_editor_project_replaced(&editor,&replacement);
    assert(editor_selection_is_empty(&editor.selection));
    sitehelper_project_destroy(&replacement); sitehelper_project_destroy(&project);
}

int main(void)
{
    test_plan_clicks_select_slab_features_without_mutation();
    test_reconciliation_clears_ephemeral_references();
    puts("All slab plan selection tests passed.");
    return 0;
}
