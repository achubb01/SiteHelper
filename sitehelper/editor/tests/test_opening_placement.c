#include <assert.h>
#include <stdio.h>

#include "opening_placement.h"

static OpeningTool test_tool(void)
{
    OpeningTool tool;

    opening_tool_init(&tool);
    tool.width = 1200;

    return tool;
}

static void test_finds_candidate_geometry(void)
{
    OpeningTool tool = test_tool();

    OpeningPlacement placement = opening_find_placement(
        (Vec2){300.0, 1000.0},
        &tool
    );

    assert(placement.has_candidate);
    assert(placement.left == 300.0);
    assert(placement.bottom == 900.0);
    assert(placement.width == 1200);
    assert(placement.height == 1200);
}

static void test_candidate_past_wall_end_is_preserved(void)
{
    OpeningTool tool = test_tool();

    OpeningPlacement placement = opening_find_placement(
        (Vec2){1000.0, 1000.0},
        &tool
    );

    assert(placement.has_candidate);
    assert(placement.left == 1000.0);
}

static void test_candidate_before_wall_start_is_preserved(void)
{
    OpeningTool tool = test_tool();

    OpeningPlacement placement = opening_find_placement(
        (Vec2){-100.0, 1000.0},
        &tool
    );

    assert(placement.has_candidate);
    assert(placement.left == -100.0);
}

static void test_unusable_tool_has_no_candidate(void)
{
    OpeningTool tool = test_tool();
    tool.width = 0;

    OpeningPlacement placement = opening_find_placement(
        (Vec2){300.0, 1000.0},
        &tool
    );

    assert(!placement.has_candidate);
    assert(!opening_placement_is_valid(&placement));
}

int main(void)
{
    test_finds_candidate_geometry();
    test_candidate_past_wall_end_is_preserved();
    test_candidate_before_wall_start_is_preserved();
    test_unusable_tool_has_no_candidate();

    puts("opening placement tests passed");
    return 0;
}
