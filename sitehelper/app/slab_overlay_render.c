#include "presentation_overlay_render.h"

#include <math.h>
#include <stddef.h>

static void app_draw_plan_marker(
    Renderer2D *renderer,
    PlanPoint point,
    double size_pixels,
    Colour colour
)
{
    if (renderer == NULL || size_pixels <= 0.0) {
        return;
    }

    Camera2D camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    Vec2 screen = camera_world_to_screen(
        &camera,
        viewport,
        (Vec2){point.x, point.y}
    );

    if (!isfinite(screen.x) || !isfinite(screen.y)) {
        return;
    }

    renderer2d_fill_screen_rect(
        renderer,
        (Rect2){
            .position = {
                screen.x - size_pixels * 0.5,
                screen.y - size_pixels * 0.5
            },
            .width = size_pixels,
            .height = size_pixels
        },
        colour
    );
}

static void app_render_slab_polygon_preview(
    Renderer2D *renderer,
    const PlanPosition *vertices,
    size_t vertex_count,
    PlanPoint cursor,
    int has_cursor,
    Colour colour,
    Colour closing_colour
)
{
    for (size_t i = 1; i < vertex_count; i++) {
        renderer2d_draw_line(
            renderer,
            (Vec2){vertices[i - 1].x, vertices[i - 1].y},
            (Vec2){vertices[i].x, vertices[i].y},
            colour
        );
    }

    if (!has_cursor || vertex_count == 0) {
        return;
    }

    PlanPosition last = vertices[vertex_count - 1];
    renderer2d_draw_line(
        renderer,
        (Vec2){last.x, last.y},
        (Vec2){cursor.x, cursor.y},
        colour
    );

    if (vertex_count >= 2) {
        renderer2d_draw_line(
            renderer,
            (Vec2){cursor.x, cursor.y},
            (Vec2){vertices[0].x, vertices[0].y},
            closing_colour
        );
    }
}

void app_render_slab_overlay(
    Renderer2D *renderer,
    const SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    if (renderer == NULL || editor == NULL) {
        return;
    }

    const PlanPosition *vertices = NULL;
    size_t vertex_count = 0;
    PlanPoint cursor = {0};
    int has_cursor = 0;

    if (sitehelper_editor_get_slab_preview(
            editor,
            &vertices,
            &vertex_count,
            &cursor,
            &has_cursor)) {
        app_render_slab_polygon_preview(
            renderer,
            vertices,
            vertex_count,
            cursor,
            has_cursor,
            (Colour){100, 220, 150, 255},
            (Colour){80, 150, 115, 255}
        );
    }

    EditorSlabPolygonPreviewKind feature_kind;
    DomainId feature_slab_id;
    if (sitehelper_editor_get_slab_feature_polygon_preview(
            editor,
            &feature_kind,
            &feature_slab_id,
            &vertices,
            &vertex_count,
            &cursor,
            &has_cursor)) {
        (void)feature_slab_id;
        Colour colour = feature_kind == EDITOR_SLAB_POLYGON_PREVIEW_PENETRATION
            ? (Colour){230, 120, 120, 255}
            : (Colour){100, 190, 230, 255};
        app_render_slab_polygon_preview(
            renderer,
            vertices,
            vertex_count,
            cursor,
            has_cursor,
            colour,
            (Colour){
                (unsigned char)(colour.r / 2U),
                (unsigned char)(colour.g / 2U),
                (unsigned char)(colour.b / 2U),
                255
            }
        );
    }

    DomainId rebate_slab_id;
    size_t rebate_edge_index;
    PlanPoint rebate_start;
    PlanPoint rebate_end;
    int rebate_has_end;
    if (sitehelper_editor_get_slab_rebate_preview(
            editor,
            &rebate_slab_id,
            &rebate_edge_index,
            &rebate_start,
            &rebate_end,
            &rebate_has_end)) {
        (void)rebate_slab_id;
        (void)rebate_edge_index;
        Colour colour = {235, 165, 85, 255};
        if (rebate_has_end) {
            renderer2d_draw_line(
                renderer,
                (Vec2){rebate_start.x, rebate_start.y},
                (Vec2){rebate_end.x, rebate_end.y},
                colour
            );
        }
        renderer2d_draw_rect(
            renderer,
            (Rect2){
                .position = {rebate_start.x - 12.0, rebate_start.y - 12.0},
                .width = 24.0,
                .height = 24.0
            },
            colour
        );
    }

    EditorSlabGeometryOverlay geometry;
    if (project != NULL && sitehelper_editor_get_slab_geometry_overlay(
            editor,
            project,
            &geometry)) {
        Colour handle_colour = {255, 205, 80, 255};
        Colour active_colour = {255, 245, 150, 255};

        if (geometry.active_vertex_index != SIZE_MAX && geometry.has_preview) {
            for (size_t i = 0; i < geometry.vertex_count; i++) {
                size_t next = i + 1 == geometry.vertex_count ? 0 : i + 1;
                PlanPoint a = i == geometry.active_vertex_index
                    ? geometry.preview
                    : (PlanPoint){geometry.vertices[i].x, geometry.vertices[i].y};
                PlanPoint b = next == geometry.active_vertex_index
                    ? geometry.preview
                    : (PlanPoint){geometry.vertices[next].x, geometry.vertices[next].y};
                renderer2d_draw_line(
                    renderer,
                    (Vec2){a.x, a.y},
                    (Vec2){b.x, b.y},
                    active_colour
                );
            }
        }

        for (size_t i = 0; i < geometry.vertex_count; i++) {
            PlanPoint point = {geometry.vertices[i].x, geometry.vertices[i].y};
            Colour colour = geometry.active_vertex_index == i
                ? active_colour
                : handle_colour;
            app_draw_plan_marker(renderer, point, 8.0, colour);
        }

        if (geometry.active_vertex_index != SIZE_MAX && geometry.has_preview) {
            app_draw_plan_marker(renderer, geometry.preview, 10.0, active_colour);
        }
    }
}
