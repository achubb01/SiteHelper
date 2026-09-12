#include "app_view.h"
#include "appstate.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

void app_views_init(AppViews *views, Camera2D initial_camera)
{
    if (views == NULL) {
        return;
    }
    for (int view = 0; view < EDITOR_VIEW_COUNT; view++) {
        views->cameras[view] = initial_camera;
    }
}

int app_views_set_active(
    AppViews *views, SiteHelperEditor *editor, Renderer2D *renderer, EditorView view
)
{
    if (views == NULL || editor == NULL || renderer == NULL ||
        view < EDITOR_VIEW_PLAN || view >= EDITOR_VIEW_COUNT) {
        return 0;
    }
    views->cameras[editor->active_view] = renderer2d_get_camera(renderer);
    if (!sitehelper_editor_set_active_view(editor, view)) {
        return 0;
    }
    renderer2d_set_camera(renderer, views->cameras[view]);
    return 1;
}

void app_render_walls(
    Renderer2D *renderer, const SiteHelperProject *project,
    const SiteHelperEditor *editor, const WallRenderStyle *style
)
{
    if (renderer == NULL || project == NULL || editor == NULL || style == NULL) {
        return;
    }
    if (editor->active_view == EDITOR_VIEW_WALL_ELEVATION) {
        const Wall *wall = app_current_wall_const(project, editor);
        if (wall != NULL) {
            const WallSelection *selection = editor_selection_get_wall_member(
                &editor->selection, wall->id
            );
            wall_elevation_render(renderer, wall,
                wall_selection_resolve(selection, wall), style);
        }
        return;
    }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL) { return; }
    for (size_t i = 0; i < storey->structure.wall_count; i++) {
        const Wall *wall = &storey->structure.walls[i];
        wall_plan_render(renderer, wall, wall->id == editor->current_wall_id
            ? style->selected_colour : style->timber_colour);
    }
}

void app_render_measurement(Renderer2D *renderer, const SiteHelperEditor *editor)
{
    PlanMeasurementQuery query;
    if (renderer == NULL || !sitehelper_editor_get_measurement(editor, &query)) { return; }
    Vec2 a = {query.start.x, query.start.y}, b = {query.end.x, query.end.y};
    /* Half each coordinate before adding to avoid overflowing a finite midpoint. */
    Vec2 midpoint = {a.x * 0.5 + b.x * 0.5, a.y * 0.5 + b.y * 0.5};
    Camera2D camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 label = camera_world_to_screen(&camera, viewport, midpoint);
    Vec2 screen_a = camera_world_to_screen(&camera, viewport, a);
    Vec2 screen_b = camera_world_to_screen(&camera, viewport, b);
    if (!isfinite(label.x) || !isfinite(label.y) ||
        !isfinite(screen_a.x) || !isfinite(screen_a.y) ||
        !isfinite(screen_b.x) || !isfinite(screen_b.y)) { return; }
    Colour colour = query.completed ? (Colour){255, 220, 90, 255} : (Colour){110, 210, 255, 255};
    renderer2d_draw_line(renderer, a, b, colour);
    /* Enough room for any finite double's fixed integer digits plus suffix.
     * Explicit round gives nearest mm, half upward, without narrowing to int. */
    char text[DBL_MAX_10_EXP + 32];
    snprintf(text, sizeof text, "%.0f mm", round(query.distance_mm));
    label.y -= 12.0; /* Pixels above the physical midpoint. */
    renderer2d_draw_screen_text(renderer, label, text, colour);
}
