#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"

static RoofPortionSpec roof_rectangle(PlanPosition *vertices)
{
    vertices[0]=(PlanPosition){0,0};
    vertices[1]=(PlanPosition){10000,0};
    vertices[2]=(PlanPosition){10000,8000};
    vertices[3]=(PlanPosition){0,8000};
    return (RoofPortionSpec){
        .support_vertices=vertices,
        .support_vertex_count=4,
        .generation=ROOF_PORTION_OPPOSING_SLOPES,
        .slope_ppm=414214,
        .reference_z_mm=0,
        .direction={1,0},
        .single_slope_reference=ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE
    };
}

int main(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey_id=sitehelper_project_add_storey(&project,0);
    assert(storey_id != DOMAIN_ID_INVALID);

    DomainId wall_id=sitehelper_project_add_wall(&project,storey_id,
        (WallPlanSegment){{0,4000},{10000,4000}});
    assert(wall_id != DOMAIN_ID_INVALID);

    const PlanPosition slab_outline[]={{0,0},{10000,0},{10000,8000},{0,8000}};
    DomainId slab_id=sitehelper_project_add_slab(&project,storey_id,
        slab_outline,4,100,0);
    assert(slab_id != DOMAIN_ID_INVALID);

    PlanPosition roof_vertices[4];
    RoofPortionSpec roof_spec=roof_rectangle(roof_vertices);
    DomainId portion_id=DOMAIN_ID_INVALID;
    DomainId roof_id=sitehelper_project_add_roof(&project,storey_id,
        &roof_spec,&portion_id);
    assert(roof_id != DOMAIN_ID_INVALID && portion_id != DOMAIN_ID_INVALID);

    DomainId note_id=sitehelper_project_add_plan_note(&project,storey_id,
        (PlanPosition){5000,4000},DOMAIN_ID_INVALID,"overlap");
    assert(note_id != DOMAIN_ID_INVALID);

    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    EditorAction action={0};
    Vec2 overlap={5000,4000};

    /* Compatibility mode retains the legacy document-first mixed precedence. */
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,overlap,&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_NOTE,note_id));

    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_FRAMING));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,overlap,&action));
    assert(editor.selection.kind == EDITOR_SELECTION_WALL);
    assert(editor.selection.wall_id == wall_id);
    assert(editor.current_wall_id == wall_id);

    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_SLAB));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,overlap,&action));
    assert(editor.selection.kind == EDITOR_SELECTION_SLAB);
    assert(editor.selection.slab_id == slab_id);
    assert(editor.current_wall_id == wall_id); /* navigation survives domain selection */

    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_ROOF));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,overlap,&action));
    assert(editor.selection.kind == EDITOR_SELECTION_ROOF_PORTION);
    assert(editor.selection.roof_id == roof_id);
    assert(editor.selection.roof_portion_id == portion_id);
    assert(editor.current_wall_id == wall_id);

    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_DOCUMENTATION));
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,overlap,&action));
    assert(editor_selection_matches_document(&editor.selection,DOCUMENT_OBJECT_NOTE,note_id));
    assert(editor.current_wall_id == wall_id);

    /* Focused workspaces do not accept an unrelated selection even if one is
     * injected directly by a test/caller; reconciliation clears it. */
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_SLAB));
    editor_selection_set_wall(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,wall_id);
    sitehelper_editor_reconcile(&editor,&project);
    assert(editor_selection_is_empty(&editor.selection));
    assert(editor.current_wall_id == wall_id);

    assert(sitehelper_editor_primary_action_in_project(&editor,&project,
        (Vec2){20000,20000},&action));
    assert(editor_selection_is_empty(&editor.selection));
    assert(editor.current_wall_id == wall_id);

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
    printf("All workspace selection tests passed.\n");
    return 0;
}
