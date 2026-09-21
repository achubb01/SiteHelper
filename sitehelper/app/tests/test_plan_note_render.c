#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_view.h"

typedef struct { size_t markers,texts; Colour last; char last_text[256]; } Drawing;
static void fill(void *context, Rect2 rect, Colour colour)
{ (void)rect; Drawing *d=context; d->markers++; d->last=colour; }
static void text(void *context, Vec2 position, const char *value, Colour colour)
{ (void)position; Drawing *d=context; d->texts++; d->last=colour;
  snprintf(d->last_text,sizeof d->last_text,"%s",value); }

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0);
    DomainId b=sitehelper_project_add_storey(&project,3000);
    DomainId note=sitehelper_project_add_plan_note(&project,a,(PlanPosition){100,200},0,
        "First line\nSecond line");
    assert(note&&sitehelper_project_add_plan_note(&project,b,(PlanPosition){100,200},0,"Other"));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0};
    renderer2d_set_backend(renderer,(RendererBackend){.context=&drawing,
        .fill_rect=fill,.draw_screen_text=text});
    renderer2d_set_camera(renderer,(Camera2D){.scale=1});
    renderer2d_set_viewport(renderer,(Vec2){0,0},800,600);

    app_render_plan_notes(renderer,&project,&editor,NULL);
    assert(drawing.markers==1&&drawing.texts==2&&strcmp(drawing.last_text,"Second line")==0);
    editor_selection_set_annotation(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,note);
    drawing=(Drawing){0}; app_render_plan_notes(renderer,&project,&editor,NULL);
    assert(drawing.markers==1&&drawing.last.r==255&&drawing.last.g==220);
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    drawing=(Drawing){0}; app_render_plan_notes(renderer,&project,&editor,NULL);
    assert(drawing.markers==0&&drawing.texts==0);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
    puts("All plan note render tests passed.");
    return 0;
}
