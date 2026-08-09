#include <assert.h>
#include <stdio.h>

#include "opening_placement.h"

static void test_opening_can_span_multiple_bays(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {600, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1200, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1800, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 4
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    tool.width = 1200;

    OpeningPlacement placement =
        opening_find_placement(
            &wall,
            (Vec2){300.0, 1000.0},
            &tool
        );

    assert(placement.valid == 1);

    assert(placement.left == 300.0);
    assert(placement.bottom == 900.0);

    assert(placement.width == 1200);
    assert(placement.height == 1200);

    assert(
        placement.start_bay_index == 0
    );

    assert(
        placement.end_bay_index == 2
    );
}

static void test_opening_past_wall_end_is_invalid(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {600, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1200, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1800, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 4
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    tool.width = 1200;

    OpeningPlacement placement =
        opening_find_placement(
            &wall,
            (Vec2){1000.0, 1000.0},
            &tool
        );

    assert(placement.valid == 0);
}

static void test_internal_studs_do_not_make_placement_invalid(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {600, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1200, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1800, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 4
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    tool.width = 900;

    OpeningPlacement placement =
        opening_find_placement(
            &wall,
            (Vec2){400.0, 1000.0},
            &tool
        );

    assert(placement.valid == 1);

    /*
     * The span crosses the stud at x = 600,
     * but that is fine. Framing logic can
     * remove/replace internal studs later.
     */
    assert(placement.left == 400.0);
    assert(placement.bottom == 900.0);
    assert(placement.width == 900);
    assert(placement.height == 1200);
}

static void test_opening_before_wall_start_is_invalid(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {600, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1200, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 3
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    tool.width = 900;

    OpeningPlacement placement =
        opening_find_placement(
            &wall,
            (Vec2){-100.0, 1000.0},
            &tool
        );

    assert(placement.valid == 0);
}

int main(void)
{
    test_opening_can_span_multiple_bays();
    test_opening_past_wall_end_is_invalid();
    test_internal_studs_do_not_make_placement_invalid();
    test_opening_before_wall_start_is_invalid();

    printf(
        "All opening placement tests passed.\n"
    );

    return 0;
}