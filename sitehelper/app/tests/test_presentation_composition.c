#include <assert.h>
#include <stdio.h>

#include "presentation_composition.h"

typedef struct
{
    size_t lines;
    size_t clip_begin;
    size_t clip_end;
    Colour last_colour;
} Drawing;

static void record_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)start;
    (void)end;
    Drawing *drawing = context;
    drawing->lines++;
    drawing->last_colour = colour;
}

static void record_clip_begin(void *context, Rect2 rect)
{
    (void)rect;
    ((Drawing *)context)->clip_begin++;
}

static void record_clip_end(void *context)
{
    ((Drawing *)context)->clip_end++;
}

static void test_workspace_policies(void)
{
    AppPresentationPolicy general = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_GENERAL, EDITOR_VIEW_PLAN);
    assert(general.grid == APP_PRESENTATION_NORMAL);
    assert(general.slabs == APP_PRESENTATION_NORMAL);
    assert(general.walls == APP_PRESENTATION_NORMAL);
    assert(general.roofs == APP_PRESENTATION_HIDDEN);
    assert(general.symbols == APP_PRESENTATION_NORMAL);
    assert(general.dimension_overlay == APP_PRESENTATION_NORMAL);
    assert(general.documentation_overlays == APP_PRESENTATION_NORMAL);

    AppPresentationPolicy elevation = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_WALL_ELEVATION);
    assert(elevation.grid == APP_PRESENTATION_NORMAL);
    assert(elevation.walls == APP_PRESENTATION_PRIMARY);
    assert(elevation.framing_overlays == APP_PRESENTATION_PRIMARY);
    assert(elevation.snap_overlay == APP_PRESENTATION_PRIMARY);
    assert(elevation.slabs == APP_PRESENTATION_HIDDEN);
    assert(elevation.dimension_overlay == APP_PRESENTATION_HIDDEN);

    AppPresentationPolicy framing = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_FRAMING, EDITOR_VIEW_PLAN);
    assert(framing.walls == APP_PRESENTATION_PRIMARY);
    assert(framing.slabs == APP_PRESENTATION_CONTEXT);
    assert(framing.dimensions == APP_PRESENTATION_NORMAL);
    assert(framing.symbols == APP_PRESENTATION_CONTEXT);
    assert(framing.framing_overlays == APP_PRESENTATION_PRIMARY);
    assert(framing.dimension_overlay == APP_PRESENTATION_NORMAL);
    assert(framing.documentation_overlays == APP_PRESENTATION_HIDDEN);

    AppPresentationPolicy slab = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_SLAB, EDITOR_VIEW_PLAN);
    assert(slab.slabs == APP_PRESENTATION_PRIMARY);
    assert(slab.walls == APP_PRESENTATION_CONTEXT);
    assert(slab.symbols == APP_PRESENTATION_CONTEXT);
    assert(slab.slab_overlays == APP_PRESENTATION_PRIMARY);
    assert(slab.dimension_overlay == APP_PRESENTATION_NORMAL);

    AppPresentationPolicy roof = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_ROOF, EDITOR_VIEW_PLAN);
    assert(roof.roofs == APP_PRESENTATION_PRIMARY);
    assert(roof.roof_overlays == APP_PRESENTATION_PRIMARY);
    assert(roof.walls == APP_PRESENTATION_CONTEXT);
    assert(roof.slabs == APP_PRESENTATION_CONTEXT);
    assert(roof.dimension_overlay == APP_PRESENTATION_NORMAL);

    AppPresentationPolicy documentation = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_DOCUMENTATION, EDITOR_VIEW_PLAN);
    assert(documentation.grid == APP_PRESENTATION_NORMAL);
    assert(documentation.walls == APP_PRESENTATION_CONTEXT);
    assert(documentation.slabs == APP_PRESENTATION_CONTEXT);
    assert(documentation.dimensions == APP_PRESENTATION_PRIMARY);
    assert(documentation.symbols == APP_PRESENTATION_PRIMARY);
    assert(documentation.callouts == APP_PRESENTATION_PRIMARY);
    assert(documentation.notes == APP_PRESENTATION_PRIMARY);
    assert(documentation.revision_clouds == APP_PRESENTATION_PRIMARY);
    assert(documentation.dimension_overlay == APP_PRESENTATION_PRIMARY);
    assert(documentation.documentation_overlays == APP_PRESENTATION_PRIMARY);

    AppPresentationPolicy unsupported = app_presentation_policy_for_workspace(
        EDITOR_WORKSPACE_SLAB, EDITOR_VIEW_WALL_ELEVATION);
    assert(unsupported.grid == APP_PRESENTATION_HIDDEN);
    assert(unsupported.slabs == APP_PRESENTATION_HIDDEN);
    assert(unsupported.walls == APP_PRESENTATION_HIDDEN);

    assert(!app_presentation_emphasis_visible(APP_PRESENTATION_HIDDEN));
    assert(app_presentation_emphasis_visible(APP_PRESENTATION_CONTEXT));
    assert(app_presentation_emphasis_visible(APP_PRESENTATION_NORMAL));
    assert(app_presentation_emphasis_visible(APP_PRESENTATION_PRIMARY));
}

static void test_composer_owns_context_emphasis(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);

    DomainId storey_id = sitehelper_project_add_storey(&project, 0);
    assert(storey_id != DOMAIN_ID_INVALID);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey_id));
    DomainId wall_id = sitehelper_project_add_wall(
        &project, storey_id, (WallPlanSegment){{0, 0}, {1000, 0}});
    assert(wall_id != DOMAIN_ID_INVALID);
    editor.current_wall_id = wall_id;

    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing,
        .draw_line = record_line,
        .set_clip_rect = record_clip_begin,
        .clear_clip_rect = record_clip_end
    });
    renderer2d_set_camera(renderer, (Camera2D){.scale = 1.0});
    renderer2d_set_viewport(renderer, (Vec2){0, 0}, 800, 600);

    WallRenderStyle wall_style = {
        .timber_colour = {100, 120, 140, 255},
        .selected_colour = {240, 200, 80, 255}
    };
    AppPresentationRenderContext context = {
        .renderer = renderer,
        .project = &project,
        .editor = &editor,
        .wall_style = &wall_style
    };
    AppPresentationPolicy policy = {0};
    policy.walls = APP_PRESENTATION_CONTEXT;

    app_presentation_render_viewport(&context, &policy);
    assert(drawing.clip_begin == 1 && drawing.clip_end == 1);
    assert(drawing.lines == 1);
    /* Context tone preserves Priority 32D's 45% muted wall treatment and does
     * not expose a remembered current_wall_id as a focused selection. */
    assert(drawing.last_colour.r == 45);
    assert(drawing.last_colour.g == 54);
    assert(drawing.last_colour.b == 63);
    assert(drawing.last_colour.a == 255);

    drawing = (Drawing){0};
    policy.walls = APP_PRESENTATION_HIDDEN;
    app_presentation_render_viewport(&context, &policy);
    assert(drawing.clip_begin == 1 && drawing.clip_end == 1);
    assert(drawing.lines == 0);

    DomainId symbol_id = sitehelper_project_add_plan_symbol(
        &project, storey_id, DOCUMENT_PLAN_SYMBOL_POINT_MARKER,
        (PlanPosition){200, 200}, (DocumentPlanDirection){0, 0});
    assert(symbol_id != DOMAIN_ID_INVALID);
    drawing = (Drawing){0};
    policy = (AppPresentationPolicy){0};
    policy.symbols = APP_PRESENTATION_CONTEXT;
    app_presentation_render_viewport(&context, &policy);
    assert(drawing.lines == 6);
    assert(drawing.last_colour.r == 54);
    assert(drawing.last_colour.g == 94);
    assert(drawing.last_colour.b == 114);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_primary_emphasis_is_visually_distinct(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);

    DomainId storey_id = sitehelper_project_add_storey(&project, 0);
    assert(storey_id != DOMAIN_ID_INVALID);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey_id));
    assert(sitehelper_project_add_wall(
        &project, storey_id, (WallPlanSegment){{0, 0}, {1000, 0}}) != DOMAIN_ID_INVALID);

    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing,
        .draw_line = record_line,
        .set_clip_rect = record_clip_begin,
        .clear_clip_rect = record_clip_end
    });

    WallRenderStyle wall_style = {
        .timber_colour = {100, 120, 140, 255},
        .selected_colour = {255, 220, 40, 255}
    };
    AppPresentationStyle presentation_style = app_presentation_style_default();
    AppPresentationRenderContext context = {
        .renderer = renderer,
        .project = &project,
        .editor = &editor,
        .wall_style = &wall_style,
        .presentation_style = &presentation_style
    };
    AppPresentationPolicy policy = {0};
    policy.walls = APP_PRESENTATION_PRIMARY;

    app_presentation_render_viewport(&context, &policy);
    assert(drawing.lines == 1);
    assert(drawing.last_colour.r == 118);
    assert(drawing.last_colour.g == 136);
    assert(drawing.last_colour.b == 153);

    drawing = (Drawing){0};
    policy.walls = APP_PRESENTATION_NORMAL;
    app_presentation_render_viewport(&context, &policy);
    assert(drawing.lines == 1);
    assert(drawing.last_colour.r == 100);
    assert(drawing.last_colour.g == 120);
    assert(drawing.last_colour.b == 140);

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_workspace_policies();
    test_composer_owns_context_emphasis();
    test_primary_emphasis_is_visually_distinct();
    puts("All presentation composition tests passed.");
    return 0;
}
