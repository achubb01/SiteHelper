#include <assert.h>
#include <stdio.h>

#include "wall_query.h"
#include "wall.h"


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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == &studs[0]);
    assert(
        hit.kind ==
        WALL_MEMBER_STUD
    );
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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == &wall.framing.bottomplate);
    assert(
        hit.kind ==
        WALL_MEMBER_BOTTOM_PLATE
    );
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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == &wall.framing.topplate);
    assert(
        hit.kind ==
        WALL_MEMBER_TOP_PLATE
    );
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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == &nogs[0]);
    assert(
        hit.kind ==
        WALL_MEMBER_NOGGIN
    );
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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == &members[0]);
    assert(
        hit.kind ==
        WALL_MEMBER_GENERATED
    );
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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == &members[0]);
    assert(
        hit.kind ==
        WALL_MEMBER_GENERATED
    );
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

    WallMemberHit hit =
        wall_find_member_at_position(
            &wall,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == NULL);
    assert(
        hit.kind ==
        WALL_MEMBER_NONE
    );

    assert(
        hit.timber == NULL
    );
}


static void test_returns_null_for_null_wall(void)
{
    Position position = {
        .x = 100,
        .y = 100
    };

    WallMemberHit hit =
        wall_find_member_at_position(
            NULL,
            position
        );

    const Timber *result =
        hit.timber;

    assert(result == NULL);
}

static void test_find_opening_by_id(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            4200
        )
    );

    DomainId opening_id = 42;

    assert(
        wall_add_opening(
            &wall,
            &settings,
            opening_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200
        )
    );

    Opening *opening =
        wall_find_opening_by_id(
            &wall,
            opening_id
        );

    assert(opening != NULL);

    assert(
        opening->id ==
        opening_id
    );

    assert(
        opening->frame_position ==
        1200
    );

    wall_destroy(
        &wall
    );
}

static void test_find_opening_by_id_rejects_invalid_id(void)
{
    Wall wall = {0};

    assert(
        wall_find_opening_by_id(
            &wall,
            DOMAIN_ID_INVALID
        ) == NULL
    );
}

static void test_find_opening_by_id_returns_null_when_missing(void)
{
    Wall wall = {0};

    assert(
        wall_find_opening_by_id(
            &wall,
            999
        ) == NULL
    );
}

static void test_opening_identity_survives_reallocation(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            12000
        )
    );

    DomainId first_id = 1;

    assert(
        wall_add_opening(
            &wall,
            &settings,
            first_id,
            OPENING_WINDOW,
            1200,
            900,
            600,
            600
        )
    );

    /*
     * Initial capacity starts at 1 and doubles,
     * so adding more openings forces realloc().
     */
    assert(
        wall_add_opening(
            &wall,
            &settings,
            2,
            OPENING_WINDOW,
            3000,
            900,
            600,
            600
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            3,
            OPENING_WINDOW,
            4800,
            900,
            600,
            600
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            4,
            OPENING_WINDOW,
            6600,
            900,
            600,
            600
        )
    );

    Opening *opening =
        wall_find_opening_by_id(
            &wall,
            first_id
        );

    assert(opening != NULL);

    assert(
        opening->id ==
        first_id
    );

    assert(
        opening->frame_position ==
        1200
    );

    wall_destroy(
        &wall
    );
}

static void
test_remove_opening_by_id_removes_only_target(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            6000
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            1,
            OPENING_WINDOW,
            600,
            900,
            900,
            1200
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            2,
            OPENING_WINDOW,
            3000,
            900,
            1200,
            1200
        )
    );

    assert(
        wall.definition.opening_count
        == 2
    );

    assert(
        wall_remove_opening_by_id(
            &wall,
            1
        )
    );

    assert(
        wall.definition.opening_count
        == 1
    );

    assert(
        wall_find_opening_by_id_const(
            &wall,
            1
        ) == NULL
    );

    assert(
        wall_find_opening_by_id_const(
            &wall,
            2
        ) != NULL
    );

    wall_destroy(
        &wall
    );
}

static void
test_remove_missing_opening_preserves_definition(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            4200
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            1,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200
        )
    );

    size_t count_before =
        wall.definition.opening_count;

    assert(
        !wall_remove_opening_by_id(
            &wall,
            999
        )
    );

    assert(
        wall.definition.opening_count
        == count_before
    );

    assert(
        wall_find_opening_by_id_const(
            &wall,
            1
        ) != NULL
    );

    wall_destroy(
        &wall
    );
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
    test_find_opening_by_id();
    test_find_opening_by_id_rejects_invalid_id();
    test_find_opening_by_id_returns_null_when_missing();
    test_opening_identity_survives_reallocation();

    test_remove_opening_by_id_removes_only_target();
    test_remove_missing_opening_preserves_definition();

    printf("All wall query tests passed.\n");

    return 0;
}