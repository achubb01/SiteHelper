#include <assert.h>
#include <stdio.h>

#include "app_view.h"

typedef struct { size_t lines; Colour last; } Drawing;
static void line(void *context, Vec2 a, Vec2 b, Colour colour)
{ (void)a; (void)b; Drawing *d=context; d->lines++; d->last=colour; }

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0);
    DomainId b=sitehelper_project_add_storey(&project,3000);
    DomainId symbol=sitehelper_project_add_plan_symbol(&project,a,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){100,200},(DocumentPlanDirection){0,0});
    assert(a&&b&&symbol);
    assert(sitehelper_project_add_plan_symbol(&project,b,
        DOCUMENT_PLAN_SYMBOL_POINT_MARKER,(PlanPosition){100,200},(DocumentPlanDirection){0,0}));

    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0};
    renderer2d_set_backend(renderer,(RendererBackend){.context=&drawing,.draw_line=line});
    renderer2d_set_camera(renderer,(Camera2D){.scale=2.0});
    renderer2d_set_viewport(renderer,(Vec2){0,0},800,600);

    app_render_plan_symbols(renderer,&project,&editor);
    assert(drawing.lines==6);
    DomainId direction=sitehelper_project_add_plan_symbol(&project,a,
        DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION,(PlanPosition){300,400},
        (DocumentPlanDirection){3,4});
    assert(direction);
    drawing=(Drawing){0}; app_render_plan_symbols(renderer,&project,&editor);
    assert(drawing.lines==13);
    editor_selection_set_symbol(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,direction);
    drawing=(Drawing){0}; app_render_plan_symbols(renderer,&project,&editor);
    assert(drawing.lines==13&&drawing.last.r==255&&drawing.last.g==220);
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    drawing=(Drawing){0}; app_render_plan_symbols(renderer,&project,&editor);
    assert(drawing.lines==0);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
    puts("All plan symbol render tests passed.");
    return 0;
}
