#include <assert.h>
#include <limits.h>
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

int main(void)
{
    test_click_order_and_both_axes_are_preserved();
    test_zero_length_and_cancellation_do_not_produce_data();
    puts("wall tool tests passed");
    return 0;
}
