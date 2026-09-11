#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "wall_render.h"
#include "renderer2d_backend.h"

typedef struct FakeBackendState {
    int fill_rect_called;
    int line_count;
    Vec2 starts[16], ends[16];
    Rect2 rects[16];
    Colour colours[16];
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

static void fake_draw_line(void *context, Vec2 start, Vec2 end, Colour colour)
{
    (void)colour;
    FakeBackendState *state = context;
    assert(state->line_count < 16);
    state->starts[state->line_count] = start;
    state->ends[state->line_count++] = end;
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
    wall_elevation_render(renderer, &other, NULL, &style);
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

    wall_elevation_render(
        renderer,
        &wall,
        &studs[0],
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

    wall_elevation_render(renderer, &first, NULL, &style);
    wall_elevation_render(renderer, &second, NULL, &style);

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

static void test_plan_renders_exact_ordered_endpoints(void)
{
    Renderer2D *renderer = renderer2d_create();
    assert(renderer != NULL);
    FakeBackendState state = {0};
    renderer2d_set_backend(renderer, (RendererBackend){
        .context = &state, .draw_line = fake_draw_line, .fill_rect = fake_fill_rect
    });
    Camera2D camera = {.position = {-250, 125}, .scale = 0.25};
    renderer2d_set_camera(renderer, camera);
    renderer2d_set_viewport(renderer, (Vec2){40, 20}, 800, 600);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    WallPlanSegment segments[] = {
        {{1000, 2000}, {5000, 5000}},
        {{5000, 5000}, {1000, 2000}},
        {{-700, 900}, {-700, -500}},
        {{800, -400}, {2400, -400}}
    };
    for (size_t i = 0; i < sizeof segments / sizeof segments[0]; i++) {
        Wall wall = {.definition.segment = segments[i]};
        wall_plan_render(renderer, &wall, (Colour){1, 2, 3, 255});
        Vec2 start = camera_screen_to_world(&camera, viewport, state.starts[i]);
        Vec2 end = camera_screen_to_world(&camera, viewport, state.ends[i]);
        assert(nearly_equal(start.x, segments[i].start.x));
        assert(nearly_equal(start.y, segments[i].start.y));
        assert(nearly_equal(end.x, segments[i].end.x));
        assert(nearly_equal(end.y, segments[i].end.y));
    }
    assert(state.line_count == 4);
    assert(state.fill_rect_called == 0);
    renderer2d_destroy(renderer);
}

int main(void)
{
    test_plan_renders_exact_ordered_endpoints();
    test_wall_render_draws_bottom_and_top_plate_and_studs();
    test_wall_render_uses_selected_colour_for_selected_timber();
    test_elevation_ignores_plan_placement();

    printf("wall render tests passed\n");

    return 0;
}
