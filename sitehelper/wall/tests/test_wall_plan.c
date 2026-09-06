#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "wall.h"

static void test_derived_lengths(void)
{
    const struct {
        WallPlanSegment segment;
        int length;
    } cases[] = {
        {{{0, 0}, {6000, 0}}, 6000},
        {{{0, 0}, {0, 6000}}, 6000},
        {{{0, 0}, {3600, 4800}}, 6000},
        {{{0, 0}, {1000, 1000}}, 1414},
        {{{0, 0}, {2, 2}}, 3},
        {{{6000, 0}, {0, 0}}, 6000},
        {{{INT_MIN, INT_MAX}, {INT_MIN + 6000, INT_MAX}}, 6000},
        {{{0, 0}, {INT_MAX, 0}}, INT_MAX},
        {{{-1, 0}, {INT_MAX, 0}}, 0},
        {{{INT_MIN, INT_MIN}, {INT_MAX, INT_MAX}}, 0},
        {{{12, 34}, {12, 34}}, 0}
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        Wall wall = { .definition.segment = cases[i].segment };
        assert(wall_plan_segment_length_mm(cases[i].segment) == cases[i].length);
        assert(wall_length_mm(&wall) == cases[i].length);
    }
    assert(wall_length_mm(NULL) == 0);
}

static void test_segment_mutation_is_atomic(void)
{
    Wall wall = {0};
    WallPlanSegment ordered = { .start = {4600, 6800}, .end = {1000, 2000} };
    assert(wall_set_plan_segment(&wall, ordered));
    assert(!wall_set_plan_segment(&wall, (WallPlanSegment){0}));
    assert(!wall_set_plan_segment(&wall, (WallPlanSegment){
        .start = {INT_MIN, 0}, .end = {INT_MAX, 0}
    }));
    assert(!wall_set_plan_segment(NULL, ordered));
    assert(wall.definition.segment.start.x == 4600);
    assert(wall.definition.segment.start.y == 6800);
    assert(wall.definition.segment.end.x == 1000);
    assert(wall.definition.segment.end.y == 2000);
    assert(wall_length_mm(&wall) == 6000);
}

int main(void)
{
    test_derived_lengths();
    test_segment_mutation_is_atomic();
    puts("wall plan tests passed");
    return 0;
}
