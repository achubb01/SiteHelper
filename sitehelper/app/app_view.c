#include "app_view.h"
#include "appstate.h"

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
    const Room *room = app_current_room_const(project, editor);
    for (size_t i = 0; room != NULL && i < room->wall_count; i++) {
        const Wall *wall = build_find_wall_by_id_const(
            &project->structure, room->wall_ids[i]
        );
        if (wall != NULL) {
            wall_plan_render(renderer, wall, wall->id == editor->current_wall_id
                ? style->selected_colour : style->timber_colour);
        }
    }
}
