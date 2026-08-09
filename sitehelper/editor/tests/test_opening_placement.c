#include <assert.h>
#include <stdio.h>

#include "opening_placement.h"

static void test_finds_bay_containing_position(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
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
        .stud_count = 2
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    tool.width = 900;

    OpeningPlacement placement =
        opening_find_placement(
            &wall,
            (Vec2){600.0, 1000.0},
            &tool
        );

    assert(placement.valid == 1);
    assert(placement.bay_index == 0);

    assert(
        placement.preview.position.x
        == 35.0
    );

    assert(
        placement.preview.position.y
        == 900.0
    );

    assert(
        placement.preview.width
        == 900.0
    );
}

static void test_rejects_opening_wider_than_bay(void)
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
        }
    };

    Wall wall = {
        .studs = studs,
        .stud_count = 2
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    tool.width = 900;

    OpeningPlacement placement =
        opening_find_placement(
            &wall,
            (Vec2){300.0, 1000.0},
            &tool
        );

    assert(placement.valid == 0);
}

int main (void) {
    test_finds_bay_containing_position();
    test_rejects_opening_wider_than_bay();
}