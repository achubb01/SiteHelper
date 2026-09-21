#include "presentation_overlay_render.h"

void app_render_framing_overlay(
    Renderer2D *renderer,
    const SiteHelperEditor *editor
)
{
    if (renderer == NULL || editor == NULL) {
        return;
    }

    Rect2 preview_rect;
    if (sitehelper_editor_get_opening_preview_rect(editor, &preview_rect)) {
        renderer2d_draw_rect(
            renderer,
            preview_rect,
            (Colour){100, 180, 255, 255}
        );
    }

    WallPlanSegment preview_segment;
    if (sitehelper_editor_get_wall_preview_segment(editor, &preview_segment)) {
        renderer2d_draw_line(
            renderer,
            (Vec2){preview_segment.start.x, preview_segment.start.y},
            (Vec2){preview_segment.end.x, preview_segment.end.y},
            (Colour){100, 220, 150, 255}
        );
    }
}
