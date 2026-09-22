#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "wall_render.h"
#include "renderer2d_backend.h"

typedef struct FakeBackendState {
    int fill_rect_called;
    int outline_rect_count;
    int line_count;
    int triangle_count;
    Vec2 starts[16], ends[16];
    Rect2 rects[16];
    Colour colours[16];
    Rect2 outline_rects[16];
    Colour outline_colours[16];
    Vec2 triangle_points[32][3];
    int text_count;
    Vec2 text_positions[16];
    Colour text_colours[16];
    char texts[16][128];
} FakeBackendState;

static void fake_fill_rect(
    void *context,
    Rect2 rect,
    Colour colour
)
{
    FakeBackendState *state = context;

    int index = state->fill_rect_called;

    assert(index < 16);

    state->rects[index] = rect;
    state->colours[index] = colour;

    state->fill_rect_called++;
}


static void fake_draw_rect(
    void *context,
    Rect2 rect,
    Colour colour
)
{
    FakeBackendState *state = context;
    int index = state->outline_rect_count;
    assert(index < 16);
    state->outline_rects[index] = rect;
    state->outline_colours[index] = colour;
    state->outline_rect_count++;
}

static void fake_draw_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)colour;
    FakeBackendState *state = context;
    assert(state->line_count < 16);
    state->starts[state->line_count] = start;
    state->ends[state->line_count++] = end;
}

static void fake_fill_triangle(void *context, Vec2 a, Vec2 b, Vec2 c, Colour colour)
{
    (void)colour;
    FakeBackendState *state=context;
    assert(state->triangle_count < 32);
    state->triangle_points[state->triangle_count][0]=a;
    state->triangle_points[state->triangle_count][1]=b;
    state->triangle_points[state->triangle_count][2]=c;
    state->triangle_count++;
}

static void fake_draw_screen_text(
    void *context, Vec2 position, const char *text, Colour colour)
{
    FakeBackendState *state = context;
    assert(state->text_count < 16);
    int index = state->text_count++;
    state->text_positions[index] = position;
    state->text_colours[index] = colour;
    snprintf(state->texts[index], sizeof state->texts[index], "%s", text);
}

static int find_text(const FakeBackendState *state, const char *text)
{
    for (int i = 0; i < state->text_count; i++) {
        if (strcmp(state->texts[i], text) == 0) {
            return i;
        }
    }
    return -1;
}

static int nearly_equal(double a, double b)
{
    const double epsilon = 0.000001;
    return fabs(a - b) < epsilon;
}

static void test_wall_render_draws_bottom_and_top_plate_and_studs(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .fill_rect = fake_fill_rect
    };

    renderer2d_set_backend(renderer, backend);

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 0.1
    };

    renderer2d_set_camera(renderer, camera);

    Timber studs[] = {
        {
            .length = 2400,
            .depth = 90,
            .width = 35,

            .position = {
                .u = 600,
                .z = 0
            },

            .type = TIMBER_STUD,

            .details.stud = {
                .type = STUD_COMMON
            }
        }
    };

    Timber noggins[] = {
        {
            .length = 565,
            .depth = 90,
            .width = 35,

            .position = {
                .u = 635,
                .z = 800
            },

            .type = TIMBER_NOGGIN,

            .details.noggin = {
                .bay = 0
            }
        }
    };

    Timber members[] = {
        {
            .length = 910,
            .depth = 90,
            .width = 35,

            .position = {
                .u = 965,
                .z = 1720
            },

            .type = TIMBER_HEADER
        },
        {
            .length = 840,
            .depth = 90,
            .width = 35,

            .position = {
                .u = 1000,
                .z = 700
            },

            .type = TIMBER_SILL
        }
    };

    Wall wall = {
        .framing.studs = studs,
        .framing.stud_count = 1,

        .framing.nogs = noggins,
        .framing.nog_count = 1,

        .framing.members = members,
        .framing.member_count = 2,

        .framing.bottomplate = {
            .length = 4200,
            .width = 35,
            .depth = 90,
            .position = {0, 0},
            .type = TIMBER_PLATE
        },

        .framing.topplate = {
            .length = 4200,
            .width = 35,
            .depth = 90,
            .position = {0, 2400},
            .type = TIMBER_PLATE
        }
    };

    WallRenderStyle style = {
        .timber_colour = {
            .r = 200,
            .g = 160,
            .b = 100,
            .a = 255
        },

        .selected_colour = {
            .r = 255,
            .g = 220,
            .b = 40,
            .a = 255
        }
    };

    wall_elevation_render(
        renderer,
        &wall,
        NULL,
        NULL,
        &style
    );

    assert(state.fill_rect_called == 6);

    /* Bottom plate */

    assert(nearly_equal(
        state.rects[0].position.x,
        0.0
    ));

    assert(nearly_equal(
        state.rects[0].position.y,
        596.5
    ));

    assert(nearly_equal(
        state.rects[0].width,
        420.0
    ));

    assert(nearly_equal(
        state.rects[0].height,
        3.5
    ));

    /* Top plate */

    assert(nearly_equal(
        state.rects[1].position.x,
        0.0
    ));

    assert(nearly_equal(
        state.rects[1].position.y,
        356.5
    ));

    assert(nearly_equal(
        state.rects[1].width,
        420.0
    ));

    assert(nearly_equal(
        state.rects[1].height,
        3.5
    ));

    assert(state.fill_rect_called == 6);

    /* Stud */

    assert(nearly_equal(
        state.rects[2].position.x,
        60.0
    ));

    assert(nearly_equal(
        state.rects[2].position.y,
        360.0
    ));

    assert(nearly_equal(
        state.rects[2].width,
        3.5
    ));

    assert(nearly_equal(
        state.rects[2].height,
        240.0
    ));

    /* Noggin */

    assert(nearly_equal(
        state.rects[3].position.x,
        63.5
    ));

    assert(nearly_equal(
        state.rects[3].position.y,
        516.5
    ));

    assert(nearly_equal(
        state.rects[3].width,
        56.5
    ));

    assert(nearly_equal(
        state.rects[3].height,
        3.5
    ));

    /* Header */

    assert(nearly_equal(
        state.rects[4].position.x,
        96.5
    ));

    assert(nearly_equal(
        state.rects[4].position.y,
        424.5
    ));

    assert(nearly_equal(
        state.rects[4].width,
        91.0
    ));

    assert(nearly_equal(
        state.rects[4].height,
        3.5
    ));

    /* Sill */

    assert(nearly_equal(
        state.rects[5].position.x,
        100.0
    ));

    assert(nearly_equal(
        state.rects[5].position.y,
        526.5
    ));

    assert(nearly_equal(
        state.rects[5].width,
        84.0
    ));

    assert(nearly_equal(
        state.rects[5].height,
        3.5
    ));
    FakeBackendState original = state;
    Wall other = wall;
    other.definition.segment = (WallPlanSegment){{-8000, 9000}, {-5480, 12360}};
    state = (FakeBackendState){0};
    wall_elevation_render(renderer, &other, NULL, NULL, &style);
    assert(state.fill_rect_called == original.fill_rect_called);
    for (int i = 0; i < state.fill_rect_called; i++) {
        assert(nearly_equal(state.rects[i].position.x, original.rects[i].position.x));
        assert(nearly_equal(state.rects[i].position.y, original.rects[i].position.y));
        assert(nearly_equal(state.rects[i].width, original.rects[i].width));
        assert(nearly_equal(state.rects[i].height, original.rects[i].height));
    }
    renderer2d_destroy(renderer);
}

static void test_wall_render_uses_selected_colour_for_selected_timber(void)
{
    Renderer2D *renderer =
        renderer2d_create();

    assert(renderer != NULL);

    FakeBackendState state = {0};

    RendererBackend backend = {
        .context = &state,
        .fill_rect = fake_fill_rect
    };

    renderer2d_set_backend(
        renderer,
        backend
    );

    renderer2d_set_viewport(
        renderer,
        (Vec2){0.0, 0.0},
        800.0,
        600.0
    );

    Camera2D camera = {
        .position = {0.0, 0.0},
        .scale = 0.1
    };

    renderer2d_set_camera(
        renderer,
        camera
    );

    Timber studs[] = {
        {
            .length = 2400,
            .depth = 90,
            .width = 35,

            .position = {
                .u = 600,
                .z = 0
            },

            .type = TIMBER_STUD,

            .details.stud = {
                .type = STUD_COMMON
            }
        }
    };

    Wall wall = {
        .framing.studs = studs,
        .framing.stud_count = 1,

        .framing.bottomplate = {
            .length = 4200,
            .width = 35,
            .depth = 90,
            .position = {0, 0},
            .type = TIMBER_PLATE
        },

        .framing.topplate = {
            .length = 4200,
            .width = 35,
            .depth = 90,
            .position = {0, 2400},
            .type = TIMBER_PLATE
        }
    };

    WallRenderStyle style = {
        .timber_colour = {
            .r = 200,
            .g = 160,
            .b = 100,
            .a = 255
        },

        .selected_colour = {
            .r = 255,
            .g = 220,
            .b = 40,
            .a = 255
        }
    };

    WallElevationRenderSelection selection = { .member = &studs[0] };
    wall_elevation_render(
        renderer,
        &wall,
        NULL,
        &selection,
        &style
    );

    /*
     * Draw order:
     *
     * 0 bottom plate
     * 1 top plate
     * 2 stud
     */

    assert(
        state.colours[0].r ==
        style.timber_colour.r
    );

    assert(
        state.colours[1].r ==
        style.timber_colour.r
    );

    assert(
        state.colours[2].r ==
        style.selected_colour.r
    );

    assert(
        state.colours[2].g ==
        style.selected_colour.g
    );

    assert(
        state.colours[2].b ==
        style.selected_colour.b
    );

    assert(
        state.colours[2].a ==
        style.selected_colour.a
    );

    renderer2d_destroy(
        renderer
    );
}

static void test_elevation_ignores_plan_placement(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);

    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &state,
        .fill_rect = fake_fill_rect
    });
    renderer2d_set_viewport(renderer, (Vec2){0.0, 0.0}, 800.0, 600.0);
    renderer2d_set_camera(renderer, (Camera2D){
        .position = {0.0, 0.0}, .scale = 1.0
    });

    Wall first = {
        .definition.segment = { .start = {0, 0}, .end = {100, 0} },
        .framing.bottomplate = {
            .length = 100, .width = 10, .position = {10, 20},
            .type = TIMBER_PLATE
        }
    };
    Wall second = first;
    second.definition.segment = (WallPlanSegment){
        .start = {500, 300}, .end = {560, 380}
    };
    WallRenderStyle style = { .timber_colour = {1, 1, 1, 255} };

    wall_elevation_render(renderer, &first, NULL, NULL, &style);
    wall_elevation_render(renderer, &second, NULL, NULL, &style);

    assert(first.framing.bottomplate.position.u == 10);
    assert(first.framing.bottomplate.position.z == 20);
    assert(second.framing.bottomplate.position.u == 10);
    assert(second.framing.bottomplate.position.z == 20);
    assert(nearly_equal(state.rects[0].position.x, state.rects[2].position.x));
    assert(nearly_equal(state.rects[0].position.y, state.rects[2].position.y));
    assert(nearly_equal(state.rects[0].width, 100.0));
    assert(nearly_equal(state.rects[2].width, 100.0));
    assert(nearly_equal(state.rects[0].height, 10.0));
    assert(nearly_equal(state.rects[2].height, 10.0));

    renderer2d_destroy(renderer);
}

static void test_plan_renders_physical_body_and_optional_datum(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context=&state,.draw_line=fake_draw_line,.fill_rect=fake_fill_rect,
        .fill_triangle=fake_fill_triangle
    });
    Camera2D camera={.position={0,0},.scale=1.0};
    renderer2d_set_camera(renderer,camera);
    renderer2d_set_viewport(renderer,(Vec2){0,0},1000,1000);
    Wall wall={0};
    assert(wall_set_plan_segment(&wall,(WallPlanSegment){{100,200},{500,200}}));
    wall_plan_render(renderer,&wall,&(WallPlanRenderStyle){
        .body_colour={1,2,3,255},.datum_colour={4,5,6,255},.show_datum=true});
    assert(state.triangle_count == 2);
    assert(state.line_count == 1);
    Viewport2D viewport=renderer2d_get_viewport(renderer);
    Vec2 a=camera_screen_to_world(&camera,viewport,state.triangle_points[0][0]);
    Vec2 b=camera_screen_to_world(&camera,viewport,state.triangle_points[0][1]);
    assert(nearly_equal(a.y,245.0));
    assert(nearly_equal(b.y,155.0));
    Vec2 datum_a=camera_screen_to_world(&camera,viewport,state.starts[0]);
    Vec2 datum_b=camera_screen_to_world(&camera,viewport,state.ends[0]);
    assert(nearly_equal(datum_a.x,100.0) && nearly_equal(datum_a.y,200.0));
    assert(nearly_equal(datum_b.x,500.0) && nearly_equal(datum_b.y,200.0));
    renderer2d_destroy(renderer);
}


static void test_elevation_uses_semantic_member_colours(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &state, .fill_rect = fake_fill_rect
    });
    renderer2d_set_viewport(renderer, (Vec2){0, 0}, 4000, 3000);
    renderer2d_set_camera(renderer, (Camera2D){.position = {0, 0}, .scale = 1.0});

    Timber studs[] = {
        {.length=2400,.width=35,.position={100,35},.type=TIMBER_STUD,
            .details.stud={.type=STUD_COMMON}},
        {.length=2400,.width=35,.position={200,35},.type=TIMBER_STUD,
            .details.stud={.type=STUD_KING}},
        {.length=2100,.width=35,.position={300,35},.type=TIMBER_STUD,
            .details.stud={.type=STUD_TRIMMER}},
        {.length=700,.width=35,.position={400,35},.type=TIMBER_STUD,
            .details.stud={.type=STUD_CRIPPLE}}
    };
    Timber noggins[] = {
        {.length=65,.width=35,.position={135,1000},.type=TIMBER_NOGGIN}
    };
    Timber members[] = {
        {.length=900,.width=35,.position={500,2100},.type=TIMBER_HEADER},
        {.length=800,.width=35,.position={550,900},.type=TIMBER_SILL}
    };
    Wall wall = {
        .framing = {
            .studs=studs,.stud_count=4,
            .nogs=noggins,.nog_count=1,
            .members=members,.member_count=2,
            .bottomplate={.length=3000,.width=35,.position={0,0},.type=TIMBER_PLATE},
            .topplate={.length=3000,.width=35,.position={0,2435},.type=TIMBER_PLATE}
        }
    };
    WallRenderStyle style = {
        .timber_colour={1,1,1,255},
        .bottom_plate_colour={10,0,0,255},
        .top_plate_colour={11,0,0,255},
        .common_stud_colour={12,0,0,255},
        .king_stud_colour={13,0,0,255},
        .trimmer_stud_colour={14,0,0,255},
        .cripple_stud_colour={15,0,0,255},
        .noggin_colour={16,0,0,255},
        .header_colour={17,0,0,255},
        .sill_colour={18,0,0,255},
        .selected_colour={99,0,0,255}
    };
    WallElevationRenderSelection selection = {.member=&studs[2]};

    wall_elevation_render(renderer,&wall,NULL,&selection,&style);
    assert(state.fill_rect_called == 9);
    const unsigned char expected[] = {10,11,12,13,99,15,16,17,18};
    for (size_t i=0;i<sizeof expected/sizeof expected[0];i++) {
        assert(state.colours[i].r == expected[i]);
    }
    renderer2d_destroy(renderer);
}

static void test_elevation_draws_wall_and_opening_context(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context=&state,.draw_rect=fake_draw_rect,.fill_rect=fake_fill_rect
    });
    renderer2d_set_viewport(renderer,(Vec2){0,0},4000,3000);
    renderer2d_set_camera(renderer,(Camera2D){.position={0,0},.scale=1.0});

    Opening openings[] = {{
        .id=42,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=900,
        .width=800,.height=900,.custom_allowance=true
    }};
    Wall wall = {
        .definition={
            .segment={{0,0},{3000,0}},
            .openings=openings,.opening_count=1,.opening_capacity=1
        },
        .framing={
            .bottomplate={.length=3000,.width=35,.position={0,0},.type=TIMBER_PLATE},
            .topplate={.length=3000,.width=35,.position={0,2400},.type=TIMBER_PLATE}
        }
    };
    BuildSettings settings = {
        .stud_height=2400,.stud_depth=90,.stud_width=35,
        .stud_spacing=600,.nog_spacing=1200,
        .opening_width_allowance=0,.opening_height_allowance=0,
        .stud_spacing_mode=STUD_SPACING_EVEN
    };
    WallRenderStyle style = {
        .timber_colour={1,1,1,255},
        .wall_extent_colour={30,0,0,255},
        .opening_colour={40,0,0,255},
        .selected_colour={90,0,0,255},
        .show_wall_extent=true,.show_openings=true
    };
    WallElevationRenderSelection selection = {.opening_id=42};

    wall_elevation_render(renderer,&wall,&settings,&selection,&style);
    assert(state.outline_rect_count == 2);
    assert(state.outline_colours[0].r == 30);
    assert(state.outline_colours[1].r == 90);
    assert(nearly_equal(state.outline_rects[0].width,3000.0));
    assert(nearly_equal(state.outline_rects[0].height,2435.0));
    assert(nearly_equal(state.outline_rects[1].width,800.0));
    assert(nearly_equal(state.outline_rects[1].height,900.0));
    renderer2d_destroy(renderer);
}


static void test_elevation_identifies_wall_openings_member_and_dimensions(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context=&state,
        .draw_line=fake_draw_line,
        .draw_rect=fake_draw_rect,
        .fill_rect=fake_fill_rect,
        .draw_screen_text=fake_draw_screen_text
    });
    renderer2d_set_viewport(renderer,(Vec2){0,0},4000,3000);
    renderer2d_set_camera(renderer,(Camera2D){.position={0,0},.scale=1.0});

    Opening openings[] = {{
        .id=42,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=900,
        .width=800,.height=900,.custom_allowance=true
    }};
    Timber studs[] = {{
        .length=2100,.depth=90,.width=35,.position={900,35},.type=TIMBER_STUD,
        .details.stud={.type=STUD_TRIMMER}
    }};
    Wall wall = {
        .id=7,
        .definition={
            .segment={{0,0},{3000,0}},
            .openings=openings,.opening_count=1,.opening_capacity=1
        },
        .framing={
            .studs=studs,.stud_count=1,.stud_capacity=1,
            .bottomplate={.length=3000,.depth=90,.width=35,.position={0,0},.type=TIMBER_PLATE},
            .topplate={.length=3000,.depth=90,.width=35,.position={0,2400},.type=TIMBER_PLATE}
        }
    };
    BuildSettings settings = {
        .stud_height=2400,.stud_depth=90,.stud_width=35,
        .stud_spacing=600,.nog_spacing=1200,
        .opening_width_allowance=0,.opening_height_allowance=0,
        .stud_spacing_mode=STUD_SPACING_EVEN
    };
    WallRenderStyle style = {
        .timber_colour={1,1,1,255},
        .annotation_colour={70,80,90,255},
        .dimension_colour={50,60,70,255},
        .selected_colour={200,210,220,255},
        .show_wall_identity=true,
        .show_opening_labels=true,
        .show_selected_member_label=true,
        .show_dimensions=true
    };
    WallElevationRenderSelection selection = {.member=&studs[0]};

    wall_elevation_render(renderer,&wall,&settings,&selection,&style);
    assert(state.line_count == 10);
    assert(find_text(&state,"Wall 7") >= 0);
    assert(find_text(&state,"Window 42") >= 0);
    assert(find_text(&state,"Generated | Trimmer stud | 90x35 | L 2100") >= 0);
    int width_label=find_text(&state,"3000 mm");
    int height_label=find_text(&state,"2435 mm");
    assert(width_label >= 0 && height_label >= 0);
    assert(state.text_colours[width_label].r == style.dimension_colour.r);
    assert(state.text_colours[height_label].r == style.dimension_colour.r);

    state=(FakeBackendState){0};
    selection=(WallElevationRenderSelection){.opening_id=42};
    wall_elevation_render(renderer,&wall,&settings,&selection,&style);
    int opening_label=find_text(&state,"Window 42");
    int opening_size=find_text(&state,"Editable | 800 x 900 mm");
    assert(opening_label >= 0 && opening_size >= 0);
    assert(state.text_colours[opening_label].r == style.selected_colour.r);
    assert(state.text_colours[opening_size].r == style.selected_colour.r);

    renderer2d_destroy(renderer);
}


static void test_elevation_hover_and_debug_axes_are_transient_affordances(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context=&state, .draw_line=fake_draw_line, .draw_rect=fake_draw_rect,
        .fill_rect=fake_fill_rect, .draw_screen_text=fake_draw_screen_text
    });
    renderer2d_set_viewport(renderer,(Vec2){0,0},4000,3000);
    renderer2d_set_camera(renderer,(Camera2D){.position={0,0},.scale=1.0});

    Opening openings[] = {{
        .id=42,.type=OPENING_WINDOW,.frame_position=1000,.frame_bottom=900,
        .width=800,.height=900,.custom_allowance=true
    }};
    Timber studs[] = {{
        .length=2100,.depth=90,.width=35,.position={900,35},.type=TIMBER_STUD,
        .details.stud={.type=STUD_TRIMMER}
    }};
    Wall wall = {
        .id=7,
        .definition={.segment={{0,0},{3000,0}},.openings=openings,.opening_count=1},
        .framing={
            .studs=studs,.stud_count=1,
            .bottomplate={.length=3000,.depth=90,.width=35,.position={0,0},.type=TIMBER_PLATE},
            .topplate={.length=3000,.depth=90,.width=35,.position={0,2400},.type=TIMBER_PLATE}
        }
    };
    BuildSettings settings = {
        .stud_height=2400,.stud_depth=90,.stud_width=35,.stud_spacing=600,
        .nog_spacing=1200,.stud_spacing_mode=STUD_SPACING_EVEN
    };
    WallRenderStyle style = {
        .timber_colour={1,1,1,255}, .opening_colour={2,2,2,255},
        .annotation_colour={3,3,3,255}, .selected_colour={4,4,4,255},
        .hovered_colour={5,6,7,255}, .debug_axis_colour={8,9,10,255},
        .show_openings=true,.show_opening_labels=true,.show_local_axes=true
    };

    WallElevationRenderSelection interaction = {
        .hovered_member=&studs[0], .hovered_opening_id=DOMAIN_ID_INVALID
    };
    wall_elevation_render(renderer,&wall,&settings,&interaction,&style);
    assert(state.outline_rect_count == 2); /* opening + hovered member */
    assert(state.outline_colours[1].r == style.hovered_colour.r);
    assert(state.line_count == 2);
    assert(find_text(&state,"U") >= 0 && find_text(&state,"Z") >= 0);

    state=(FakeBackendState){0};
    interaction=(WallElevationRenderSelection){.hovered_opening_id=42};
    wall_elevation_render(renderer,&wall,&settings,&interaction,&style);
    assert(state.outline_rect_count == 1);
    assert(state.outline_colours[0].r == style.hovered_colour.r);
    int label=find_text(&state,"Window 42");
    assert(label >= 0 && state.text_colours[label].r == style.hovered_colour.r);

    state=(FakeBackendState){0};
    interaction=(WallElevationRenderSelection){.member=&studs[0],.hovered_member=&studs[0]};
    wall_elevation_render(renderer,&wall,&settings,&interaction,&style);
    assert(state.outline_rect_count == 1); /* opening only: selection wins hover */
    assert(state.colours[2].r == style.selected_colour.r);

    renderer2d_destroy(renderer);
}

int main(void)
{
    test_plan_renders_physical_body_and_optional_datum();
    test_wall_render_draws_bottom_and_top_plate_and_studs();
    test_wall_render_uses_selected_colour_for_selected_timber();
    test_elevation_ignores_plan_placement();
    test_elevation_uses_semantic_member_colours();
    test_elevation_draws_wall_and_opening_context();
    test_elevation_identifies_wall_openings_member_and_dimensions();
    test_elevation_hover_and_debug_axes_are_transient_affordances();

    printf("wall render tests passed\n");

    return 0;
}
