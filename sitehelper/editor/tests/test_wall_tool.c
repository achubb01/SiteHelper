#include <assert.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

#include "wall_tool.h"

static void test_click_order_and_both_axes_are_preserved(void)
{
    const WallPlanSegment cases[] = {
        { .start = {1000, 2000}, .end = {5000, 5000} },
        { .start = {5000, 5000}, .end = {1000, 2000} },
        { .start = {5000, 3000}, .end = {1000, 3500} },
        { .start = {1000, 500}, .end = {1000, 6500} }
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        WallTool tool;
        WallPlanSegment segment;
        wall_tool_init(&tool);
        wall_tool_activate(&tool);
        assert(wall_tool_begin(&tool, (Vec2){cases[i].start.x, cases[i].start.y}));
        wall_tool_update(&tool, (Vec2){cases[i].end.x, cases[i].end.y});
        assert(wall_tool_command_data(&tool, &segment));
        assert(segment.start.x == cases[i].start.x);
        assert(segment.start.y == cases[i].start.y);
        assert(segment.end.x == cases[i].end.x);
        assert(segment.end.y == cases[i].end.y);
    }
}

static void test_zero_length_and_cancellation_do_not_produce_data(void)
{
    WallTool tool;
    WallPlanSegment segment = { .start = {12, 34}, .end = {56, 78} };

    wall_tool_init(&tool);
    wall_tool_activate(&tool);
    assert(wall_tool_begin(&tool, (Vec2){ .x = 1000, .y = 500 }));
    assert(!wall_tool_command_data(&tool, &segment));
    wall_tool_update(&tool, (Vec2){1000.9, 500.9});
    assert(!wall_tool_command_data(&tool, &segment));
    wall_tool_update(&tool, (Vec2){NAN, 500});
    assert(!wall_tool_command_data(&tool, &segment));
    wall_tool_update(&tool, (Vec2){(double)INT_MAX + 1.0, 500});
    assert(!wall_tool_command_data(&tool, &segment));
    assert(segment.start.x == 12 && segment.end.y == 78);
    wall_tool_update(&tool, (Vec2){1000, 6000});
    wall_tool_cancel(&tool);
    assert(!wall_tool_command_data(&tool, &segment));
    assert(!wall_tool_command_data(NULL, &segment));
    assert(!wall_tool_command_data(&tool, NULL));
}

static void test_exact_numeric_lengths(void)
{
    WallTool tool;
    wall_tool_init(&tool); wall_tool_activate(&tool);
    const int lengths[] = {1, 2, 3, 90, 600, 2400, 2700, 4200, 1000000, INT_MAX};
    const Vec2 directions[] = {{1,0},{0,1},{-1,0},{0,-1},{1,1},{3,4},{17,-31},{-23,-99},{1,10000}};
    for (size_t l = 0; l < sizeof lengths / sizeof *lengths; l++) {
        for (size_t d = 0; d < sizeof directions / sizeof *directions; d++) {
            assert(wall_tool_begin(&tool, (Vec2){0,0}));
            wall_tool_update(&tool, directions[d]);
            WallPlanSegment segment;
            assert(wall_tool_set_length(&tool, lengths[l]) == WALL_LENGTH_OK);
            assert(wall_tool_command_data(&tool, &segment));
            assert(wall_plan_segment_length_mm(segment) == lengths[l]);
            double norm = hypot(directions[d].x, directions[d].y);
            assert(hypot(segment.end.x - directions[d].x / norm * lengths[l],
                segment.end.y - directions[d].y / norm * lengths[l]) <= 1.415);
        }
    }
    /* Dense directions/lengths catch independent-component rounding errors. */
    for (int length = 1; length <= 4200; length += 7) {
        for (int d = 0; d < 360; d += 7) {
            double angle = d * 0.017453292519943295;
            assert(wall_tool_begin(&tool, (Vec2){1234, -9876}));
            wall_tool_update(&tool, (Vec2){1234 + cos(angle)*100, -9876 + sin(angle)*100});
            WallPlanSegment segment;
            assert(wall_tool_resolve_length(&tool, length, &segment) == WALL_LENGTH_OK);
            assert(wall_plan_segment_length_mm(segment) == length);
        }
    }
    WallPlanSegment segment = {{12,34},{56,78}};
    assert(wall_tool_begin(&tool, (Vec2){10,20}));
    assert(wall_tool_resolve_length(&tool, 4200, &segment) == WALL_LENGTH_DIRECTIONLESS);
    assert(wall_tool_resolve_length(&tool, 0, &segment) == WALL_LENGTH_NONPOSITIVE);
    assert(wall_tool_resolve_length(&tool, -1, &segment) == WALL_LENGTH_NONPOSITIVE);
    wall_tool_update(&tool, (Vec2){NAN,0});
    assert(wall_tool_resolve_length(&tool, 4200, &segment) == WALL_LENGTH_OUT_OF_RANGE);
    assert(wall_tool_begin(&tool, (Vec2){INT_MAX,INT_MAX}));
    wall_tool_update(&tool, (Vec2){(double)INT_MAX+100, INT_MAX});
    assert(wall_tool_resolve_length(&tool, 4200, &segment) == WALL_LENGTH_OUT_OF_RANGE);
    assert(segment.start.x == 12 && segment.end.y == 78);
    wall_tool_update(&tool, (Vec2){(double)INT_MAX-100, INT_MAX});
    assert(wall_tool_resolve_length(&tool, 4200, &segment) == WALL_LENGTH_OK);
    assert(wall_plan_segment_length_mm(segment) == 4200);
    assert(wall_tool_begin(&tool, (Vec2){0,0}));
    wall_tool_update(&tool, (Vec2){DBL_MAX, DBL_MAX});
    assert(wall_tool_resolve_length(&tool, 4200, &segment) == WALL_LENGTH_OK);
    assert(wall_plan_segment_length_mm(segment) == 4200);
    double tiny = nextafter(0.0, 1.0);
    wall_tool_update(&tool, (Vec2){tiny, tiny});
    assert(wall_tool_resolve_length(&tool, 4200, &segment) == WALL_LENGTH_OK);
    assert(wall_plan_segment_length_mm(segment) == 4200);
    wall_tool_cancel(&tool);
    assert(!tool.length_mm && !tool.has_start);
}

int main(void)
{
    test_exact_numeric_lengths();
    test_click_order_and_both_axes_are_preserved();
    test_zero_length_and_cancellation_do_not_produce_data();
    puts("wall tool tests passed");
    return 0;
}
