#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "wall_render.h"
#include "../renderer2d/renderer2d_backend.h"

typedef struct FakeBackendState {
    int fill_rect_called;
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

    BuildSettings settings = {
        .stud_width = 35,
        .stud_height = 2400
    };

    Timber studs[] = {
        {
            .length = 2400,
            .depth = 90,
            .width = 35,

            .position = {
                .x = 600,
                .y = 0
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
                .x = 635,
                .y = 800
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
                .x = 965,
                .y = 1720
            },

            .type = TIMBER_HEADER
        },
        {
            .length = 840,
            .depth = 90,
            .width = 35,

            .position = {
                .x = 1000,
                .y = 700
            },

            .type = TIMBER_SILL
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 1,

        .nogs = noggins,
        .nog_count = 1,

        .members = members,
        .member_count = 2,

        .bottomplate = {
            .length = 4200,
            .width = 35,
            .depth = 90,
            .position = {0, 0},
            .type = TIMBER_PLATE
        },

        .topplate = {
            .length = 4200,
            .width = 35,
            .depth = 90,
            .position = {0, 2400},
            .type = TIMBER_PLATE
        }
    };

    WallSelection selection = {
    .selected = NULL
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

    wall_render(
        renderer,
        &wall,
        &selection,
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
    renderer2d_destroy(renderer);
}

int main(void)
{
    test_wall_render_draws_bottom_and_top_plate_and_studs();

    printf("wall render tests passed\n");

    return 0;
}