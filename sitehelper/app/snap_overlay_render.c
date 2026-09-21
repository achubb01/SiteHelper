#include "presentation_overlay_render.h"

void app_render_snap_overlay(
    Renderer2D *renderer,
    const SiteHelperEditor *editor
)
{
    if (renderer == NULL || editor == NULL || !sitehelper_editor_has_snap(editor)) {
        return;
    }

    const SnapResult *snap_result = sitehelper_editor_get_snap_result(editor);
    if (snap_result == NULL) {
        return;
    }

    Colour marker_colour;
    switch (snap_result->type) {
        case SNAP_ENDPOINT:
            marker_colour = (Colour){255, 180, 60, 255};
            break;
        case SNAP_GRID:
            marker_colour = (Colour){80, 200, 255, 255};
            break;
        case SNAP_WALL_CENTRELINE:
            marker_colour = (Colour){220, 120, 255, 255};
            break;
        case SNAP_INTERSECTION:
            marker_colour = (Colour){80, 255, 120, 255};
            break;
        default:
            return;
    }

    const double marker_radius = 40.0;
    Vec2 position = snap_result->position;

    renderer2d_draw_line(
        renderer,
        (Vec2){position.x - marker_radius, position.y},
        (Vec2){position.x + marker_radius, position.y},
        marker_colour
    );
    renderer2d_draw_line(
        renderer,
        (Vec2){position.x, position.y - marker_radius},
        (Vec2){position.x, position.y + marker_radius},
        marker_colour
    );
}
