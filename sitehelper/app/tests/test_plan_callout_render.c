#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_view.h"

typedef struct { size_t lines,texts; Colour last_line,last_text; char text[128]; } Drawing;
static void line(void *context, Vec2 a, Vec2 b, Colour colour)
{ (void)a;(void)b; Drawing *d=context; d->lines++; d->last_line=colour; }
static void screen_text(void *context, Vec2 p, const char *text, Colour colour)
{ (void)p; Drawing *d=context; d->texts++; d->last_text=colour; snprintf(d->text,sizeof d->text,"%s",text); }

int main(void)
{
    SiteHelperProject project; sitehelper_project_init(&project);
    DomainId a=sitehelper_project_add_storey(&project,0),b=sitehelper_project_add_storey(&project,3000);
    DomainId id=sitehelper_project_add_plan_callout(&project,a,(PlanPosition){0,0},
        (PlanPosition){1000,500},"Check this");
    assert(a&&b&&id);
    assert(sitehelper_project_add_plan_callout(&project,b,(PlanPosition){0,0},
        (PlanPosition){1000,500},"Other"));
    SiteHelperEditor editor; sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor,&project,a));
    Renderer2D *renderer=renderer2d_create(); assert(renderer);
    Drawing drawing={0};
    renderer2d_set_backend(renderer,(RendererBackend){.context=&drawing,.draw_line=line,.draw_screen_text=screen_text});
    renderer2d_set_camera(renderer,(Camera2D){.scale=2.0});
    renderer2d_set_viewport(renderer,(Vec2){0,0},800,600);

    app_render_plan_callouts(renderer,&project,&editor);
    assert(drawing.lines==3&&drawing.texts==1&&strcmp(drawing.text,"Check this")==0);
    editor_selection_set_callout(&editor.selection,EDITOR_SELECTION_SCOPE_PLAN,id);
    drawing=(Drawing){0}; app_render_plan_callouts(renderer,&project,&editor);
    assert(drawing.lines==3&&drawing.texts==1&&drawing.last_text.r==255&&drawing.last_text.g==220);

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_CALLOUT));
    EditorAction action={0};
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){2000,2000},&action));
    sitehelper_editor_pointer_move_in_project(&editor,&project,(Vec2){2500,2200});
    drawing=(Drawing){0}; app_render_plan_callout_preview(renderer,&editor);
    assert(drawing.lines==3&&drawing.texts==0);
    assert(sitehelper_editor_primary_action_in_project(&editor,&project,(Vec2){2500,2200},&action));
    drawing=(Drawing){0}; app_render_plan_callout_preview(renderer,&editor);
    assert(drawing.lines==3);

    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    drawing=(Drawing){0}; app_render_plan_callouts(renderer,&project,&editor);
    assert(drawing.lines==0&&drawing.texts==0);
    renderer2d_destroy(renderer); sitehelper_editor_destroy(&editor); sitehelper_project_destroy(&project);
    puts("All plan callout render tests passed.");
    return 0;
}
