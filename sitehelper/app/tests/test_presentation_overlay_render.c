#include <assert.h>
#include <stdio.h>

#include "presentation_overlay_render.h"

#define MAX_RECORDED_LINES 16

typedef struct
{
    size_t line_count;
    size_t fill_screen_rect_count;
    size_t clip_begin_count;
    size_t clip_end_count;
    Vec2 starts[MAX_RECORDED_LINES];
    Vec2 ends[MAX_RECORDED_LINES];
    Colour colours[MAX_RECORDED_LINES];
} Drawing;

static void record_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    Drawing *drawing = context;
    assert(drawing->line_count < MAX_RECORDED_LINES);
    size_t index = drawing->line_count++;
    drawing->starts[index] = start;
    drawing->ends[index] = end;
    drawing->colours[index] = colour;
}

static void record_fill_screen_rect(void *context, Rect2 rect, Colour colour)
{
    (void)rect;
    (void)colour;
    ((Drawing *)context)->fill_screen_rect_count++;
}

static void record_clip_begin(void *context, Rect2 rect)
{
    (void)rect;
    ((Drawing *)context)->clip_begin_count++;
}

static void record_clip_end(void *context)
{
    ((Drawing *)context)->clip_end_count++;
}

static Renderer2D *make_renderer(Drawing *drawing)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = drawing,
        .draw_line = record_line,
        .fill_rect = record_fill_screen_rect,
        .set_clip_rect = record_clip_begin,
        .clear_clip_rect = record_clip_end
    });
    renderer2d_set_camera(renderer, (Camera2D){.scale = 1.0});
    renderer2d_set_viewport(renderer, (Vec2){0, 0}, 800, 600);
    return renderer;
}

static void test_framing_overlay_owns_preview_geometry_only(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.active_tool = EDITOR_TOOL_WALL;
    wall_tool_activate(&editor.wall_tool);
    assert(wall_tool_begin(&editor.wall_tool, (Vec2){100, 200}));
    wall_tool_update(&editor.wall_tool, (Vec2){900, 600});

    Drawing drawing = {0};
    Renderer2D *renderer = make_renderer(&drawing);
    app_render_framing_overlay(renderer, &editor);

    assert(drawing.line_count == 1);
    assert(drawing.colours[0].r == 100 && drawing.colours[0].g == 220 &&
        drawing.colours[0].b == 150);
    assert(drawing.clip_begin_count == 0 && drawing.clip_end_count == 0);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
}

static void test_slab_overlay_owns_sketch_geometry_only(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.active_tool = EDITOR_TOOL_SLAB;
    slab_tool_activate(&editor.slab_tool);
    assert(slab_tool_append(&editor.slab_tool, 1, (PlanPosition){0, 0}));
    assert(slab_tool_append(&editor.slab_tool, 1, (PlanPosition){1000, 0}));
    slab_tool_update(&editor.slab_tool, (PlanPoint){1000, 800});

    Drawing drawing = {0};
    Renderer2D *renderer = make_renderer(&drawing);
    app_render_slab_overlay(renderer, NULL, &editor);

    assert(drawing.line_count == 3);
    assert(drawing.clip_begin_count == 0 && drawing.clip_end_count == 0);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
}

static void test_snap_overlay_owns_marker_geometry_only(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    sitehelper_editor_set_snap_result(&editor, (SnapResult){
        .type = SNAP_ENDPOINT,
        .position = {500, 700}
    });

    Drawing drawing = {0};
    Renderer2D *renderer = make_renderer(&drawing);
    app_render_snap_overlay(renderer, &editor);

    assert(drawing.line_count == 2);
    assert(drawing.ends[0].x - drawing.starts[0].x == 80);
    assert(drawing.starts[1].y - drawing.ends[1].y == 80);
    assert(drawing.colours[0].r == 255 && drawing.colours[0].g == 180 &&
        drawing.colours[0].b == 60);
    assert(drawing.clip_begin_count == 0 && drawing.clip_end_count == 0);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
}

int main(void)
{
    test_framing_overlay_owns_preview_geometry_only();
    test_slab_overlay_owns_sketch_geometry_only();
    test_snap_overlay_owns_marker_geometry_only();
    puts("All presentation overlay render tests passed.");
    return 0;
}
