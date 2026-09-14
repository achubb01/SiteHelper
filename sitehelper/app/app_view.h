#ifndef APP_VIEW_H
#define APP_VIEW_H

#include "sitehelper_editor.h"
#include "wall_render.h"
#include "slab_plan_render.h"

/* Saved cameras are application state. The renderer holds the live camera. */
typedef struct
{
    Camera2D cameras[EDITOR_VIEW_COUNT];
} AppViews;

void app_views_init(AppViews *views, Camera2D initial_camera);
int app_views_set_active(
    AppViews *views, SiteHelperEditor *editor, Renderer2D *renderer, EditorView view
);
void app_render_walls(
    Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const WallRenderStyle *style
);
void app_render_slabs(
    Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const SlabPlanRenderStyle *style
);

/* Caller supplies the normal viewport clip; label formatting stays in app. */
void app_render_measurement(Renderer2D *renderer, const SiteHelperEditor *editor);

#endif
