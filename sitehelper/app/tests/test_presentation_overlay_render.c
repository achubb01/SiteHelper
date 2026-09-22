#include <assert.h>
#include <stdio.h>

#include "presentation_overlay_render.h"

#define MAX_RECORDED_LINES 16

typedef struct
{
    size_t line_count;
    size_t fill_screen_rect_count;
    size_t draw_rect_count;
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

static void record_draw_rect(void *context, Rect2 rect, Colour colour)
{
    (void)rect;
    (void)colour;
    ((Drawing *)context)->draw_rect_count++;
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
        .draw_rect = record_draw_rect,
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
    app_render_framing_overlay(renderer, NULL, &editor, NULL);

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


static void test_framing_overlay_draws_selected_opening_grips_and_drag_candidate(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    DomainId storey_id=sitehelper_project_add_storey(&project,0);
    DomainId wall_id=sitehelper_project_add_wall(&project,storey_id,
        (WallPlanSegment){{0,0},{5000,0}});
    Wall *wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&project,storey_id,&settings));
    DomainId opening_id=domain_id_generate(&project.domain_ids);
    assert(wall_add_opening_definition(wall,&settings,&(Opening){
        .id=opening_id,.type=OPENING_WINDOW,.frame_position=1000,
        .frame_bottom=700,.width=800,.height=1000,.custom_allowance=true}));
    assert(wall_generate(wall,&settings));
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    editor.current_wall_id=wall_id;
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_FRAMING));
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    editor_selection_set_opening(&editor.selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,wall_id,opening_id);

    Drawing drawing={0};
    Renderer2D *renderer=make_renderer(&drawing);
    AppInteractionStyle interaction=app_interaction_style_default();
    app_render_framing_overlay(renderer,&project,&editor,&interaction);
    assert(drawing.fill_screen_rect_count == 5);
    assert(drawing.line_count == 0);

    assert(sitehelper_editor_begin_opening_edit_in_project(&editor,&project,
        (Vec2){1800,1200},20.0));
    sitehelper_editor_update_opening_edit_in_project(&editor,&project,
        (Vec2){2000,1200});
    drawing=(Drawing){0};
    RendererBackend backend={.context=&drawing,.draw_line=record_line,
        .draw_rect=record_draw_rect,.fill_rect=record_fill_screen_rect,.set_clip_rect=record_clip_begin,
        .clear_clip_rect=record_clip_end};
    renderer2d_set_backend(renderer,backend);
    app_render_framing_overlay(renderer,&project,&editor,&interaction);
    assert(drawing.fill_screen_rect_count == 5);
    assert(drawing.draw_rect_count == 1);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_framing_overlay_owns_preview_geometry_only();
    test_framing_overlay_draws_selected_opening_grips_and_drag_candidate();
    test_slab_overlay_owns_sketch_geometry_only();
    test_snap_overlay_owns_marker_geometry_only();
    puts("All presentation overlay render tests passed.");
    return 0;
}
