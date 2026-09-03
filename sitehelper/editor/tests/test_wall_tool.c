#include <assert.h>
#include <stdio.h>

#include "wall_tool.h"

static void test_right_to_left_normalises_wall_data(void)
{
    WallTool tool;
    Position origin;
    int length;

    wall_tool_init(&tool);
    wall_tool_activate(&tool);
    assert(wall_tool_begin(&tool, (Vec2){ .x = 5000, .y = 3000 }));
    wall_tool_update(&tool, (Vec2){ .x = 1000, .y = 3500 });

    assert(wall_tool_command_data(&tool, &origin, &length));
    assert(origin.x == 1000);
    assert(origin.y == 3000);
    assert(length == 4000);
}

static void test_zero_length_and_cancellation_do_not_produce_data(void)
{
    WallTool tool;
    Position origin;
    int length;

    wall_tool_init(&tool);
    wall_tool_activate(&tool);
    assert(wall_tool_begin(&tool, (Vec2){ .x = 1000, .y = 500 }));
    assert(!wall_tool_command_data(&tool, &origin, &length));
    wall_tool_cancel(&tool);
    assert(!wall_tool_command_data(&tool, &origin, &length));
}

int main(void)
{
    test_right_to_left_normalises_wall_data();
    test_zero_length_and_cancellation_do_not_produce_data();
    puts("wall tool tests passed");
    return 0;
}
