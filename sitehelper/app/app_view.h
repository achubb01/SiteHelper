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

/* Optional styling input for persistent documentation layers. Render adapters
 * remain workspace-neutral; presentation composition supplies contextual tone. */
typedef struct
{
    unsigned int colour_scale_percent;
} AppAnnotationRenderStyle;

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
/* Roof source-intent layer. Draws authoritative support polygons, not derived
 * planes/ridges/hips/valleys. Presentation policy owns workspace visibility. */
void app_render_roofs(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor);
/* Project-owned persistent dimension overlay; Plan view only. */
void app_render_plan_dimensions(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor);
/* Transient three-click dimension authoring overlay; Plan view only. */
void app_render_plan_dimension_preview(Renderer2D *renderer, const SiteHelperEditor *editor);

/* Project-owned point-marker symbol overlay; fixed-pixel target, Plan view only. */
void app_render_plan_symbols(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const AppAnnotationRenderStyle *style);

/* Project-owned leader/callout overlay and transient two-point authoring preview. */
void app_render_plan_callouts(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const AppAnnotationRenderStyle *style);
void app_render_plan_callout_preview(Renderer2D *renderer, const SiteHelperEditor *editor);

/* Project-owned note overlay; fixed-pixel marker/text, Plan view only. */
void app_render_plan_notes(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const AppAnnotationRenderStyle *style);

/* Project-owned revision markup. Boundary is authoritative; scallops are derived. */
void app_render_plan_revision_clouds(Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const AppAnnotationRenderStyle *style);
void app_render_plan_revision_cloud_preview(Renderer2D *renderer,
    const SiteHelperEditor *editor);

/* Caller supplies the normal viewport clip; label formatting stays in app. */
void app_render_measurement(Renderer2D *renderer, const SiteHelperEditor *editor);

#endif
