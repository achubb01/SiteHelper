#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_view.h"

typedef struct { size_t lines,texts; Colour last; char last_text[64]; } Drawing;
static void line(void *context, Vec2 a, Vec2 b, Colour colour)
{ (void)a; (void)b; Drawing *d=context; d->lines++; d->last=colour; }
static void text(void *context, Vec2 position, const char *value, Colour colour)
{ (void)position; Drawing *d=context; d->texts++; d->last=colour;
  snprintf(d->last_text,sizeof d->last_text,"%s",value); }
static DocumentDimensionReference fixed(int x,int y)
{ return (DocumentDimensionReference){.kind=DOCUMENT_DIMENSION_FIXED_POINT,.position={x,y}}; }

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0);
    DomainId b=sitehelper_project_add_storey(&project,3000);
    DomainId dimension=sitehelper_project_add_plan_dimension(&project,a,fixed(0,0),fixed(3000,4000),250);
    assert(dimension&&sitehelper_project_add_plan_dimension(&project,b,fixed(0,0),fixed(1000,0),100));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0};
    renderer2d_set_backend(renderer,(RendererBackend){.context=&drawing,.draw_line=line,.draw_screen_text=text});
    renderer2d_set_camera(renderer,(Camera2D){.scale=1});
    renderer2d_set_viewport(renderer,(Vec2){0,0},800,600);

    app_render_plan_dimensions(renderer,&project,&editor,NULL);
    assert(drawing.lines==5&&drawing.texts==1&&strcmp(drawing.last_text,"5000 mm")==0);
    editor_selection_set_dimension(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,dimension);
    drawing=(Drawing){0}; app_render_plan_dimensions(renderer,&project,&editor,NULL);
    assert(drawing.lines==5&&drawing.texts==1&&drawing.last.r==255&&drawing.last.g==220);
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    drawing=(Drawing){0}; app_render_plan_dimensions(renderer,&project,&editor,NULL);
    assert(drawing.lines==0&&drawing.texts==0);

    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_PLAN));
    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_DIMENSION));
    assert(plan_dimension_tool_set_first(&editor.dimension_tool,fixed(0,0),(PlanPosition){0,0}));
    assert(plan_dimension_tool_update_pointer(&editor.dimension_tool,(PlanPoint){3000,0}));
    drawing=(Drawing){0}; app_render_plan_dimension_preview(renderer,&editor);
    assert(drawing.lines==5&&drawing.texts==1&&strcmp(drawing.last_text,"3000 mm")==0);
    assert(plan_dimension_tool_set_second(&editor.dimension_tool,fixed(3000,0),(PlanPosition){3000,0}));
    assert(plan_dimension_tool_update_pointer(&editor.dimension_tool,(PlanPoint){1500,250}));
    drawing=(Drawing){0}; app_render_plan_dimension_preview(renderer,&editor);
    assert(drawing.lines==5&&drawing.texts==1&&strcmp(drawing.last_text,"3000 mm")==0);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
    puts("All plan dimension render tests passed.");
    return 0;
}
