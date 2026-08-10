#include <assert.h>
#include <stdio.h>

#include "wall_query.h"


static void test_finds_stud_at_world_position(void)
{
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

    Wall wall = {
        .framing.studs = studs,
        .framing.stud_count = 1
    };

    Position position = {
        .x = 620,
        .y = 1000
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == &studs[0]);
}


static void test_finds_bottom_plate(void)
{
    Wall wall = {
        .framing.bottomplate = {
            .length = 4200,
            .depth = 90,
            .width = 35,
            .position = {
                .x = 0,
                .y = 0
            },
            .type = TIMBER_PLATE
        }
    };

    Position position = {
        .x = 1000,
        .y = 20
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == &wall.framing.bottomplate);
}


static void test_finds_top_plate(void)
{
    Wall wall = {
        .framing.topplate = {
            .length = 4200,
            .depth = 90,
            .width = 35,
            .position = {
                .x = 0,
                .y = 2400
            },
            .type = TIMBER_PLATE
        }
    };

    Position position = {
        .x = 1500,
        .y = 2420
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == &wall.framing.topplate);
}


static void test_finds_noggin(void)
{
    Timber nogs[] = {
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

    Wall wall = {
        .framing.nogs = nogs,
        .framing.nog_count = 1
    };

    Position position = {
        .x = 800,
        .y = 820
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == &nogs[0]);
}


static void test_finds_header(void)
{
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
        }
    };

    Wall wall = {
        .framing.members = members,
        .framing.member_count = 1
    };

    Position position = {
        .x = 1200,
        .y = 1730
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == &members[0]);
}


static void test_finds_sill(void)
{
    Timber members[] = {
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
        .framing.members = members,
        .framing.member_count = 1
    };

    Position position = {
        .x = 1300,
        .y = 720
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == &members[0]);
}


static void test_returns_null_for_empty_space(void)
{
    Timber studs[] = {
        {
            .length = 2400,
            .depth = 90,
            .width = 35,
            .position = {
                .x = 600,
                .y = 0
            },
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .framing.studs = studs,
        .framing.stud_count = 1
    };

    Position position = {
        .x = 500,
        .y = 1000
    };

    const Timber *selected =
        wall_find_timber_at_position(
            &wall,
            position
        );

    assert(selected == NULL);
}


static void test_returns_null_for_null_wall(void)
{
    Position position = {
        .x = 100,
        .y = 100
    };

    const Timber *selected =
        wall_find_timber_at_position(
            NULL,
            position
        );

    assert(selected == NULL);
}


int main(void)
{
    test_finds_stud_at_world_position();
    test_finds_bottom_plate();
    test_finds_top_plate();
    test_finds_noggin();
    test_finds_header();
    test_finds_sill();
    test_returns_null_for_empty_space();
    test_returns_null_for_null_wall();

    printf("All wall query tests passed.\n");

    return 0;
}