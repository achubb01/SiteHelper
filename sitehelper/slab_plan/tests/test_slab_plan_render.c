#include <assert.h>
#include <stdio.h>

#include "app_view.h"

typedef struct {
    size_t line_count, marker_count, selected_lines, parent_lines;
} Drawing;

static void line(void *context, Vec2 a, Vec2 b, Colour colour)
{
    (void)a; (void)b;
    Drawing *drawing=context;
    drawing->line_count++;
    if (colour.r==255&&colour.g==220&&colour.b==40) { drawing->selected_lines++; }
    if (colour.r==150&&colour.g==150&&colour.b==105) { drawing->parent_lines++; }
}

static void fill(void *context, Rect2 rect, Colour colour)
{
    (void)rect; (void)colour;
    ((Drawing *)context)->marker_count++;
}

static SlabPlanRenderStyle style(void)
{
    return (SlabPlanRenderStyle){
        .outline_colour={100,100,100,255}, .penetration_colour={200,0,0,255},
        .region_colour={0,100,200,255}, .rebate_colour={200,100,0,255},
        .selected_colour={255,220,40,255},
        .selected_parent_colour={150,150,105,255}, .marker_size_pixels=6
    };
}

static Slab make_featured_slab(DomainId id)
{
    const PlanPosition outer[]={{0,0},{1000,0},{1000,1000},{0,1000}};
    const PlanPosition region[]={{0,0},{300,0},{300,300},{0,300}};
    const PlanPosition second_region[]={{700,0},{1000,0},{1000,300},{700,300}};
    const PlanPosition hole[]={{500,500},{700,500},{700,700},{500,700}};
    Slab slab={0};
    assert(slab_build(id,outer,4,100,0,&slab)==SLAB_SUCCESS);
    assert(slab_add_penetration(&slab,hole,4)==SLAB_SUCCESS);
    assert(slab_add_region(&slab,region,4,-50,80)==SLAB_SUCCESS);
    assert(slab_add_region(&slab,second_region,4,25,120)==SLAB_SUCCESS);
    assert(slab_add_edge_rebate(&slab,3,0,400,100,20)==SLAB_SUCCESS);
    return slab;
}

static void test_feature_primitives_and_selection(void)
{
    Slab slab=make_featured_slab(10);
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0};
    renderer2d_set_backend(renderer,(RendererBackend){
        .context=&drawing,.draw_line=line,.fill_rect=fill});
    renderer2d_set_camera(renderer,(Camera2D){.scale=1});
    renderer2d_set_viewport(renderer,(Vec2){0,0},2000,2000);
    SlabPlanRenderStyle render_style=style();
    SlabPlanHit selected={SLAB_PLAN_HIT_EDGE_REBATE,10,0};
    slab_plan_render(renderer,&slab,&selected,&render_style);
    assert(drawing.line_count==17); /* outer 4 + regions 8 + void 4 + rebate 1 */
    assert(drawing.marker_count==6); /* void vertices plus rebate endpoints */
    assert(drawing.selected_lines==1&&drawing.parent_lines==4);

    drawing=(Drawing){0}; selected=(SlabPlanHit){SLAB_PLAN_HIT_PENETRATION,10,0};
    slab_plan_render(renderer,&slab,&selected,&render_style);
    assert(drawing.selected_lines==4&&drawing.parent_lines==4);
    slab.definition.thickness_mm=0;
    drawing=(Drawing){0}; slab_plan_render(renderer,&slab,&selected,&render_style);
    assert(drawing.line_count==0&&drawing.marker_count==0);
    slab.definition.thickness_mm=100;
    renderer2d_destroy(renderer); slab_destroy(&slab);
}

static void test_concave_and_clockwise_outlines_render_closed(void)
{
    const PlanPosition concave[]={{0,0},{500,0},{500,200},{200,200},{200,500},{0,500}};
    const PlanPosition clockwise[]={{1000,0},{1000,400},{1400,400},{1400,0}};
    Slab first={0},second={0};
    assert(slab_build(20,concave,6,100,0,&first)==SLAB_SUCCESS);
    assert(slab_build(21,clockwise,4,100,0,&second)==SLAB_SUCCESS);
    Storey storey={.id=1};
    assert(slab_collection_append(&storey.slabs,&first)==SLAB_SUCCESS);
    assert(slab_collection_append(&storey.slabs,&second)==SLAB_SUCCESS);
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0}; renderer2d_set_backend(renderer,(RendererBackend){
        .context=&drawing,.draw_line=line,.fill_rect=fill});
    SlabPlanRenderStyle render_style=style();
    slab_plan_render_storey(renderer,&storey,NULL,&render_style);
    assert(drawing.line_count==10);
    renderer2d_destroy(renderer); slab_collection_destroy(&storey.slabs);
}

static void test_application_plan_and_current_storey_scope(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId first_storey=sitehelper_project_add_storey(&project,0);
    DomainId second_storey=sitehelper_project_add_storey(&project,3000);
    const PlanPosition a[]={{0,0},{100,0},{100,100},{0,100}};
    const PlanPosition b[]={{1000,0},{1200,0},{1200,200},{1000,200}};
    DomainId first_slab=sitehelper_project_add_slab(&project,first_storey,a,4,100,0);
    assert(first_slab&&sitehelper_project_add_slab(&project,second_storey,b,4,100,0));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,first_storey));
    editor_selection_set_slab(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,first_slab);
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0}; renderer2d_set_backend(renderer,(RendererBackend){
        .context=&drawing,.draw_line=line,.fill_rect=fill});
    SlabPlanRenderStyle render_style=style();
    app_render_slabs(renderer,&project,&editor,&render_style);
    assert(drawing.line_count==4&&drawing.selected_lines==4);
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    drawing=(Drawing){0}; app_render_slabs(renderer,&project,&editor,&render_style);
    assert(drawing.line_count==0);
    renderer2d_destroy(renderer); sitehelper_project_destroy(&project);
}

int main(void)
{
    test_feature_primitives_and_selection();
    test_concave_and_clockwise_outlines_render_closed();
    test_application_plan_and_current_storey_scope();
    puts("All slab plan render tests passed.");
    return 0;
}
