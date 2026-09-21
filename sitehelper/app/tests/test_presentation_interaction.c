#include <assert.h>
#include <stdio.h>

#include "app_view.h"

#define MAX_LINES 32

typedef struct
{
    size_t line_count;
    Colour colours[MAX_LINES];
} Drawing;

static void record_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)start;
    (void)end;
    Drawing *drawing = context;
    assert(drawing->line_count < MAX_LINES);
    drawing->colours[drawing->line_count++] = colour;
}

static int same_colour(Colour a, Colour b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static RoofPortionSpec roof_portion(PlanPosition *vertices, int wing)
{
    if (!wing) {
        vertices[0] = (PlanPosition){0, 0};
        vertices[1] = (PlanPosition){12000, 0};
        vertices[2] = (PlanPosition){12000, 8000};
        vertices[3] = (PlanPosition){0, 8000};
    } else {
        vertices[0] = (PlanPosition){4000, 4000};
        vertices[1] = (PlanPosition){8000, 4000};
        vertices[2] = (PlanPosition){8000, 11000};
        vertices[3] = (PlanPosition){4000, 11000};
    }
    return (RoofPortionSpec){
        .support_vertices = vertices,
        .support_vertex_count = 4,
        .generation = ROOF_PORTION_OPPOSING_SLOPES,
        .slope_ppm = wing ? 577350 : 414214,
        .reference_z_mm = 2700,
        .direction = wing ? (RoofDirection){0, 1} : (RoofDirection){1, 0},
        .single_slope_reference = ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE
    };
}

static void test_wall_selection_is_not_navigation(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey_id = sitehelper_project_add_storey(&project, 0);
    DomainId wall_id = sitehelper_project_add_wall(&project, storey_id,
        (WallPlanSegment){{0, 0}, {4200, 0}});
    assert(storey_id != DOMAIN_ID_INVALID && wall_id != DOMAIN_ID_INVALID);

    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey_id));
    editor.current_wall_id = wall_id;

    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing,
        .draw_line = record_line
    });

    WallRenderStyle wall_style = {.timber_colour = {20, 30, 40, 255}};
    AppInteractionStyle interaction = {
        .selected_colour = {1, 2, 3, 255},
        .selection_owner_colour = {4, 5, 6, 255},
        .navigation_colour = {7, 8, 9, 255}
    };

    app_render_walls(renderer, &project, &editor, &wall_style, &interaction);
    assert(drawing.line_count == 1);
    assert(same_colour(drawing.colours[0], interaction.navigation_colour));

    editor_selection_set_wall(&editor.selection, EDITOR_SELECTION_SCOPE_PLAN, wall_id);
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &wall_style, &interaction);
    assert(drawing.line_count == 1);
    assert(same_colour(drawing.colours[0], interaction.selected_colour));

    editor_selection_set_opening(&editor.selection, EDITOR_SELECTION_SCOPE_PLAN,
        wall_id, 999);
    drawing = (Drawing){0};
    app_render_walls(renderer, &project, &editor, &wall_style, &interaction);
    assert(drawing.line_count == 1);
    assert(same_colour(drawing.colours[0], interaction.selection_owner_colour));

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_roof_portion_selection_highlights_owner_separately(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey_id = sitehelper_project_add_storey(&project, 0);
    assert(storey_id != DOMAIN_ID_INVALID);

    PlanPosition first_vertices[4];
    RoofPortionSpec first_spec = roof_portion(first_vertices, 0);
    DomainId first_portion = DOMAIN_ID_INVALID;
    DomainId roof_id = sitehelper_project_add_roof(&project, storey_id,
        &first_spec, &first_portion);
    assert(roof_id != DOMAIN_ID_INVALID && first_portion != DOMAIN_ID_INVALID);

    PlanPosition second_vertices[4];
    RoofPortionSpec second_spec = roof_portion(second_vertices, 1);
    DomainId second_portion = sitehelper_project_add_roof_portion_composed(
        &project, roof_id, &second_spec, first_portion, ROOF_COMPOSITION_INTERSECTS);
    assert(second_portion != DOMAIN_ID_INVALID);

    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey_id));
    editor_selection_set_roof_portion(&editor.selection, EDITOR_SELECTION_SCOPE_PLAN,
        roof_id, second_portion);

    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    Drawing drawing = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &drawing,
        .draw_line = record_line
    });

    AppInteractionStyle interaction = {
        .selected_colour = {10, 20, 30, 255},
        .selection_owner_colour = {40, 50, 60, 255},
        .navigation_colour = {70, 80, 90, 255}
    };
    app_render_roofs(renderer, &project, &editor, NULL, &interaction);

    assert(drawing.line_count == 8);
    for (size_t i = 0; i < 4; i++) {
        assert(same_colour(drawing.colours[i], interaction.selection_owner_colour));
    }
    for (size_t i = 4; i < 8; i++) {
        assert(same_colour(drawing.colours[i], interaction.selected_colour));
    }

    renderer2d_destroy(renderer);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_wall_selection_is_not_navigation();
    test_roof_portion_selection_highlights_owner_separately();
    printf("All presentation interaction tests passed.\n");
    return 0;
}
