#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_input.h"
#include "app_properties_panel.h"
#include "app_view.h"
#include "presentation_composition.h"

#define MAX_LINES 64

typedef struct {
    size_t line_count;
    Colour line_colours[MAX_LINES];
} Drawing;

static void record_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)start;
    (void)end;
    Drawing *drawing=context;
    assert(drawing->line_count < MAX_LINES);
    drawing->line_colours[drawing->line_count++]=colour;
}

static RoofPortionSpec roof_rectangle(PlanPosition *vertices)
{
    vertices[0]=(PlanPosition){0,0};
    vertices[1]=(PlanPosition){6000,0};
    vertices[2]=(PlanPosition){6000,4000};
    vertices[3]=(PlanPosition){0,4000};
    return (RoofPortionSpec){
        .support_vertices=vertices,
        .support_vertex_count=4,
        .generation=ROOF_PORTION_OPPOSING_SLOPES,
        .slope_ppm=500000,
        .reference_z_mm=2700,
        .direction={1,0},
        .single_slope_reference=ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE
    };
}

static AppPresentationRenderContext render_context(Renderer2D *renderer,
    const SiteHelperProject *project, const SiteHelperEditor *editor,
    const WallRenderStyle *wall_style)
{
    return (AppPresentationRenderContext){
        .renderer=renderer,
        .project=project,
        .editor=editor,
        .wall_style=wall_style
    };
}

int main(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey_id=sitehelper_project_add_storey(&project,0);
    assert(storey_id != DOMAIN_ID_INVALID);
    DomainId wall_id=sitehelper_project_add_wall(&project,storey_id,
        (WallPlanSegment){{0,0},{6000,0}});
    assert(wall_id != DOMAIN_ID_INVALID);

    PlanPosition roof_vertices[4];
    RoofPortionSpec spec=roof_rectangle(roof_vertices);
    DomainId portion_id=DOMAIN_ID_INVALID;
    DomainId roof_id=sitehelper_project_add_roof(&project,storey_id,&spec,&portion_id);
    assert(roof_id != DOMAIN_ID_INVALID && portion_id != DOMAIN_ID_INVALID);

    DomainId note_id=sitehelper_project_add_plan_note(&project,storey_id,
        (PlanPosition){1000,1000},DOMAIN_ID_INVALID,"note");
    assert(note_id != DOMAIN_ID_INVALID);

    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));

    AppPropertiesPanel panel;
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_FRAMING));
    editor_selection_set_wall(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,wall_id);
    assert(app_properties_panel_build(&editor,&project,&panel));
    assert(strcmp(panel.title,"Wall") == 0);
    assert(panel.field_count == 4 && panel.info_count == 3);
    assert(panel.fields[0].property == EDITOR_PROPERTY_WALL_START_X);

    AppInput input={0};
    assert(app_input_begin_property(&input,&editor,&project,EDITOR_PROPERTY_WALL_END_X));
    assert(input.focus == APP_KEYBOARD_FOCUS_PROPERTY_MM);
    assert(input.property_target_kind == EDITOR_SELECTION_WALL);
    assert(input.property_wall_id == wall_id);
    app_input_cancel(&input,&editor);

    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_ROOF));
    const Storey *storey=sitehelper_project_find_storey_by_id_const(&project,storey_id);
    assert(storey != NULL);
    assert(sitehelper_editor_select_roof_at_position(&editor,storey,
        (PlanPoint){3000,2000},0.0));
    assert(app_properties_panel_build(&editor,&project,&panel));
    assert(strcmp(panel.title,"Roof portion") == 0);
    assert(panel.field_count == 0 && panel.info_count >= 4);
    assert(strstr(panel.info_rows[1],"0.500") != NULL);

    Renderer2D *renderer=renderer2d_create();
    assert(renderer != NULL);
    Drawing drawing={0};
    renderer2d_set_backend(renderer,(RendererBackend){
        .context=&drawing,
        .draw_line=record_line
    });

    WallRenderStyle wall_style={
        .timber_colour={100,120,140,255},
        .selected_colour={240,200,80,255}
    };
    editor.current_wall_id=wall_id;

    /* Priority 33 consumes Priority 32 workspace intent. Roof presentation is
     * primary while supporting walls remain subdued context. */
    AppPresentationPolicy policy=app_presentation_policy_for_workspace(
        editor.active_workspace,editor.active_view);
    AppPresentationRenderContext context=render_context(renderer,&project,&editor,
        &wall_style);
    app_presentation_render_viewport(&context,&policy);
    assert(drawing.line_count == 5);
    assert(drawing.line_colours[0].r == 45);
    assert(drawing.line_colours[0].g == 54);
    assert(drawing.line_colours[0].b == 63);
    assert(drawing.line_colours[4].r == 255);
    assert(drawing.line_colours[4].g == 220);
    assert(drawing.line_colours[4].b == 40);

    /* The render adapter itself is now workspace-neutral: visibility belongs to
     * presentation composition, not app_view.c. */
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_DOCUMENTATION));
    drawing=(Drawing){0};
    app_render_roofs(renderer,&project,&editor,NULL,NULL);
    assert(drawing.line_count == 4);

    editor_selection_set_annotation(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,note_id);
    assert(app_properties_panel_build(&editor,&project,&panel));
    assert(strcmp(panel.title,"Note") == 0);
    assert(panel.field_count == 0 && panel.note != NULL);

    drawing=(Drawing){0};
    policy=app_presentation_policy_for_workspace(editor.active_workspace,
        editor.active_view);
    app_presentation_render_viewport(&context,&policy);
    /* Documentation hides roof source intent and keeps the building as context. */
    assert(drawing.line_count == 1);
    assert(drawing.line_colours[0].r == 45);
    assert(drawing.line_colours[0].g == 54);
    assert(drawing.line_colours[0].b == 63);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
    printf("All workspace presentation tests passed.\n");
    return 0;
}
