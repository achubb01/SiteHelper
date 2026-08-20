#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "wall.h"

static const DomainId TEST_OPENING_ID = 1;
static const DomainId TEST_OPENING_ID_2 = 2;

// void wall_destroy(Wall *wall)
// {
//     if (wall == NULL) {
//         return;
//     }

//     free(wall->framing.studs);
//     free(wall->framing.nogs);
//     free(wall->framing.openings);

//     free(wall->framing.members);

//     wall->framing.members = NULL;
//     wall->framing.member_count = 0;
//     wall->framing.member_capacity = 0;

//     wall->framing.openings = NULL;
//     wall->framing.opening_count = 0;
//     wall->framing.opening_capacity = 0;

//     wall->framing.studs = NULL;
//     wall->framing.nogs = NULL;

//     wall->framing.stud_count = 0;
//     wall->framing.stud_capacity = 0;

//     wall->framing.nog_count = 0;
//     wall->framing.nog_capacity = 0;
// }

// void room_destroy(Room *room)
// {
//     if (room == NULL) {
//         return;
//     }

//     for (size_t i = 0;
//          i < room->wall_count;
//          i++) {

//         wall_destroy(
//             &room->walls[i]
//         );
//     }

//     free(room->walls);

//     room->walls = NULL;
//     room->wall_count = 0;
//     room->wall_capacity = 0;
// }

// void build_destroy(
//     BuildStructure *structure
// )
// {
//     if (structure == NULL) {
//         return;
//     }

//     for (size_t i = 0;
//          i < structure->room_count;
//          i++) {

//         room_destroy(
//             &structure->rooms[i]
//         );
//     }

//     free(structure->rooms);

//     structure->rooms = NULL;
//     structure->room_count = 0;
//     structure->room_capacity = 0;
// }

static int bay_is_opening(
    const Wall *wall,
    const BuildSettings *settings,
    const Timber *left,
    const Timber *right
)
{
    assert(wall != NULL);
    assert(settings != NULL);
    assert(left != NULL);
    assert(right != NULL);

    int bay_left =
        left->position.x +
        settings->stud_width;

    int bay_right =
        right->position.x;

    for (size_t i = 0;
         i < wall->definition.opening_count;
         i++) {

        const Opening *opening =
            &wall->definition.openings[i];

        int opening_left =
            opening->frame_position;

        int opening_right =
            opening_left +
            opening_frame_width(
                opening,
                settings
            );

        /*
         * Temporary diagnostics.
         */
        // if (right->position.x -
        //         left->position.x >
        //     settings->stud_spacing) {

        //     fprintf(
        //         stderr,
        //         "\nCHECKING LARGE BAY\n"
        //         "bay left:      %d\n"
        //         "bay right:     %d\n"
        //         "opening index: %zu\n"
        //         "opening left:  %d\n"
        //         "opening right: %d\n"
        //         "opening width: %d\n"
        //         "opening type:  %d\n",
        //         bay_left,
        //         bay_right,
        //         i,
        //         opening_left,
        //         opening_right,
        //         opening_frame_width(
        //             opening,
        //             settings
        //         ),
        //         (int)opening->type
        //     );
        // }

        if (bay_left == opening_left &&
            bay_right == opening_right) {

            return 1;
        }
    }

    return 0;
}

static void assert_valid_vertical_member_spacing(
    const Wall *wall,
    const BuildSettings *settings
)
{
    assert(wall != NULL);
    assert(settings != NULL);

    for (size_t i = 1;
         i < wall->framing.stud_count;
         i++) {

        const Timber *left =
            &wall->framing.studs[i - 1];

        const Timber *right =
            &wall->framing.studs[i];

        int spacing =
            right->position.x -
            left->position.x;

        /*
         * Negative means the array isn't
         * actually sorted.
         */
        assert(spacing >= 0);

        /*
         * Two separate vertical members can
         * occupy the same x coordinate.
         *
         * Example:
         * lower cripple + upper cripple.
         */
        if (spacing == 0) {
            continue;
        }

        /*
         * Closely packed framing such as
         * KING | TRIMMER is intentional.
         */
        if (spacing <= settings->stud_width) {
            continue;
        }

        if (bay_is_opening(
                wall,
                settings,
                left,
                right)) {

            continue;
        }

        if (spacing > settings->stud_spacing) {

            fprintf(
                stderr,
                "\nUNRECOGNISED LARGE BAY\n"
                "left x:    %d\n"
                "right x:   %d\n"
                "bay left:  %d\n"
                "bay right: %d\n"
                "left type: %d\n"
                "right type:%d\n",
                left->position.x,
                right->position.x,
                left->position.x +
                    settings->stud_width,
                right->position.x,
                (int)left->details.stud.type,
                (int)right->details.stud.type
            );
        }

        assert(
            spacing <=
            settings->stud_spacing
        );
    }
}

static void assert_studs_inside_wall(
    const Wall *wall,
    const BuildSettings *settings
)
{
    for (size_t i = 0;
         i < wall->framing.stud_count;
         i++) {

        const Timber *stud =
            &wall->framing.studs[i];

        assert(stud->position.x >= 0);
        assert(stud->position.y >= 0);

        assert(
        stud->width ==
        settings->stud_width
    );

    assert(
        stud->depth ==
        settings->stud_depth
    );

    assert(
        stud->position.x +
        stud->width
        <= wall->framing.bottomplate.length
    );

        assert(
            stud->position.y +
            stud->length
            <= settings->stud_height
        );
    }
}

static void assert_wall_end_studs(
    const Wall *wall,
    const BuildSettings *settings
)
{
    assert(wall->framing.stud_count > 0);

    assert(
        wall->framing.studs[0].position.x == 0
    );

    const Timber *last =
        &wall->framing.studs[
            wall->framing.stud_count - 1
        ];

    assert(
        last->position.x +
        last->width ==
        wall->framing.bottomplate.length
    );
}

static void assert_valid_cripples(
    const Wall *wall,
    const BuildSettings *settings
)
{
    for (size_t i = 0;
         i < wall->framing.stud_count;
         i++) {

        const Timber *stud =
            &wall->framing.studs[i];

        if (stud->details.stud.type !=
            STUD_CRIPPLE) {
            continue;
        }

        assert(stud->length > 0);

        assert(stud->position.x >= 0);
        assert(stud->position.y >= 0);

        assert(
            stud->position.y +
            stud->length
            <= settings->stud_height
        );
    }
}

static void assert_valid_plates(
    const Wall *wall,
    const BuildSettings *settings
)
{
    assert(wall != NULL);
    assert(settings != NULL);

    const Timber *bottom =
        &wall->framing.bottomplate;

    const Timber *top =
        &wall->framing.topplate;

    assert(bottom->type == TIMBER_PLATE);
    assert(top->type == TIMBER_PLATE);

    assert(bottom->length > 0);

    assert(
        top->length ==
        bottom->length
    );

    assert(
        bottom->width ==
        settings->stud_width
    );

    assert(
        top->width ==
        settings->stud_width
    );

    assert(
        bottom->depth ==
        settings->stud_depth
    );

    assert(
        top->depth ==
        settings->stud_depth
    );

    assert(bottom->position.x == 0);
    assert(bottom->position.y == 0);

    assert(top->position.x == 0);

    assert(
        top->position.y ==
        settings->stud_height
    );
}

static void assert_valid_generated_wall(
    const Wall *wall,
    const BuildSettings *settings
)
{
    assert_valid_plates(
        wall,
        settings
    );

    assert_valid_vertical_member_spacing(
        wall,
        settings
    );

    assert_studs_inside_wall(
        wall,
        settings
    );

    assert_wall_end_studs(
        wall,
        settings
    );

    assert_valid_cripples(
        wall,
        settings
    );
}


static void print_property_case(
    int wall_length,
    int opening_x,
    int opening_bottom,
    int opening_width,
    int opening_height,
    StudSpacingMode mode
)
{
    fprintf(
        stderr,
        "Property case: "
        "wall=%d, "
        "x=%d, "
        "bottom=%d, "
        "width=%d, "
        "height=%d, "
        "mode=%d\n",
        wall_length,
        opening_x,
        opening_bottom,
        opening_width,
        opening_height,
        (int)mode
    );
}

static void test_property_single_opening_geometry(void)
{
    int wall_lengths[] = {
        3000,
        3600,
        4200,
        5000,
        6000
    };

    int opening_widths[] = {
        600,
        820,
        1000,
        1200
    };

    int opening_bottoms[] = {
        300,
        500,
        700,
        900,
        1100
    };

    int opening_heights[] = {
        400,
        600,
        800,
        1000,
        1200
    };

    StudSpacingMode modes[] = {
        STUD_SPACING_EVEN,
        STUD_SPACING_MAXIMISE
    };

    for (size_t mode_index = 0;
         mode_index <
            sizeof modes / sizeof modes[0];
         mode_index++) {

        for (size_t wall_index = 0;
             wall_index <
                sizeof wall_lengths /
                sizeof wall_lengths[0];
             wall_index++) {

            for (size_t width_index = 0;
                width_index <
                    sizeof opening_widths /
                    sizeof opening_widths[0];
                width_index++) {

                for (size_t bottom_index = 0;
                    bottom_index <
                        sizeof opening_bottoms /
                        sizeof opening_bottoms[0];
                    bottom_index++) {

                    for (size_t height_index = 0;
                        height_index <
                            sizeof opening_heights /
                            sizeof opening_heights[0];
                        height_index++) {

                        for (int opening_x = 100;
                            opening_x <
                                wall_lengths[wall_index];
                            opening_x += 25) {

                            Wall wall = {0};

                            BuildSettings settings = {
                                .stud_height = 2400,
                                .stud_width = 35,
                                .stud_depth = 90,
                                .stud_spacing = 450,
                                .nog_spacing = 900,

                                .opening_width_allowance = 20,
                                .opening_height_allowance = 20,

                                .stud_spacing_mode =
                                    modes[mode_index]
                            };

                            assert(
                                wall_set_length(
                                    &wall,
                                    wall_lengths[wall_index]
                                )
                            );

                            int added =
                                wall_add_opening(
                                    &wall,
                                    &settings,
                                    TEST_OPENING_ID,
                                    OPENING_WINDOW,
                                    opening_x,
                                    opening_bottoms[
                                        bottom_index
                                    ],
                                    opening_widths[
                                        width_index
                                    ],
                                    opening_heights[
                                        height_index
                                    ]
                                );

                            if (!added) {
                                wall_destroy(&wall);
                                continue;
                            }

                            if (!wall_generate(
                                    &wall,
                                    &settings)) {

                                print_property_case(
                                    wall_lengths[wall_index],
                                    opening_x,
                                    opening_bottoms[
                                        bottom_index
                                    ],
                                    opening_widths[
                                        width_index
                                    ],
                                    opening_heights[
                                        height_index
                                    ],
                                    modes[mode_index]
                                );

                                assert(0);
                            }

                            assert_valid_generated_wall(
                                &wall,
                                &settings
                            );

                            wall_destroy(&wall);
                        }
                    }
                }
            }
        }
    }
}

static void test_wall_generates_complete_plates(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
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
        wall_generate(
            &wall,
            &settings
        )
    );

    /*
     * Bottom plate
     */

    assert(
        wall.framing.bottomplate.type ==
        TIMBER_PLATE
    );

    assert(
        wall.framing.bottomplate.length ==
        4200
    );

    assert(
        wall.framing.bottomplate.width ==
        35
    );

    assert(
        wall.framing.bottomplate.depth ==
        90
    );

    assert(
        wall.framing.bottomplate.position.x ==
        0
    );

    assert(
        wall.framing.bottomplate.position.y ==
        0
    );

    /*
     * Top plate
     */

    assert(
        wall.framing.topplate.type ==
        TIMBER_PLATE
    );

    assert(
        wall.framing.topplate.length ==
        4200
    );

    assert(
        wall.framing.topplate.width ==
        35
    );

    assert(
        wall.framing.topplate.depth ==
        90
    );

    assert(
        wall.framing.topplate.position.x ==
        0
    );

    assert(
        wall.framing.topplate.position.y ==
        2400
    );

    wall_destroy(&wall);
}

static void test_maximise_spacing(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(wall_generate(
        &wall,
        &settings
    ));

    /*
     * First stud must start at the wall origin.
     */
    assert(
        wall.framing.studs[0]
            .position.x == 0
    );

    /*
     * Last stud's reference face must land at:
     *
     * wall length - stud width
     */
    assert(
        wall.framing.studs[wall.framing.stud_count - 1]
            .position.x
        ==
        4200 - 35
    );

    /*
     * No stud spacing may exceed the configured maximum.
     */
    for (size_t i = 1;
         i < wall.framing.stud_count;
         i++) {

        int previous =
            wall.framing.studs[i - 1]
                .position.x;

        int current =
            wall.framing.studs[i]
                .position.x;

        int spacing =
            current - previous;

        assert(spacing <= settings.stud_spacing);
        assert(spacing > 0);
    }

    wall_destroy(&wall);
}

static void test_even_spacing(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode = STUD_SPACING_EVEN
    };

    assert(wall_set_length(&wall, 4200));

    assert(wall_generate(
        &wall,
        &settings
    ));

    assert(
        wall.framing.studs[0]
            .position.x == 0
    );

    assert(
        wall.framing.studs[wall.framing.stud_count - 1]
            .position.x
        ==
        4200 - 35
    );

    for (size_t i = 1;
         i < wall.framing.stud_count;
         i++) {

        int spacing =
            wall.framing.studs[i]
                .position.x
            -
            wall.framing.studs[i - 1]
                .position.x;

        assert(spacing <= settings.stud_spacing);
        assert(spacing > 0);
    }

    wall_destroy(&wall);
}

static void test_noggins(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(wall_generate(
        &wall,
        &settings
    ));

    assert(wall.framing.nog_count > 0);

    for (size_t i = 0;
         i < wall.framing.nog_count;
         i++) {

        Timber *noggin =
            &wall.framing.nogs[i];

        assert(noggin->length > 0);

        assert(
            noggin->details.noggin.bay + 1
            <
            wall.framing.stud_count
        );

        assert(
            noggin->position.y > 0
        );

        assert(
            noggin->position.y
            <
            settings.stud_height
        );
    }

    wall_destroy(&wall);
}

static void test_maximise_uses_standard_spacing(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(wall_generate(
        &wall,
        &settings
    ));

    size_t max_gap_count = 0;

    for (size_t i = 1;
         i < wall.framing.stud_count;
         i++) {

        int spacing =
            wall.framing.studs[i]
                .position.x
            -
            wall.framing.studs[i - 1]
                .position.x;

        if (spacing == settings.stud_spacing) {
            max_gap_count++;
        }
    }

    assert(max_gap_count == 8);

    wall_destroy(&wall);
}

static void test_wall_regeneration(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            4200
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    size_t first_stud_count =
        wall.framing.stud_count;

    size_t first_nog_count =
        wall.framing.nog_count;

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert(
        wall.framing.stud_count ==
        first_stud_count
    );

    assert(
        wall.framing.nog_count ==
        first_nog_count
    );

    wall_destroy(&wall);
}

static void test_add_opening(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2040
        )
    );

    assert(wall.definition.opening_count == 1);

    Opening *opening =
        &wall.definition.openings[0];

    assert(opening->type == OPENING_DOOR);
    assert(opening->frame_position == 1000);
    assert(opening->width == 820);
    assert(opening->height == 2040);

    assert(
        opening_frame_width(
            opening,
            &settings
        ) == 840
    );

    assert(
        opening_frame_height(
            opening,
            &settings
        ) == 2060
    );

    wall_destroy(&wall);
}

static void test_reject_opening_outside_wall(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20
    };

    assert(wall_set_length(&wall, 3000));

    assert(
        !wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            2500,
            0,
            820,
            2040
        )
    );

    assert(wall.definition.opening_count == 0);

    wall_destroy(&wall);
}

static void test_reject_opening_too_tall(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        !wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2400
        )
    );

    assert(wall.definition.opening_count == 0);

    wall_destroy(&wall);
}

static void test_opening_removes_interfering_studs(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .opening_width_allowance = 20,
        .opening_height_allowance = 20,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2040
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    int opening_start =
        opening->frame_position;

    int opening_end =
        opening_start +
        opening_frame_width(
            opening,
            &settings
        );

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        /*
         * King studs are deliberately beside
         * the opening, so this test is concerned
         * with leftover COMMON studs.
         */
        if (stud->details.stud.type !=
            STUD_COMMON) {
            continue;
        }

        int stud_start =
            stud->position.x;

        int stud_end =
            stud_start +
            settings.stud_width;

        /*
         * A stud is safely outside the opening if:
         *
         * it finishes before the opening starts
         *
         * OR
         *
         * it starts after the opening ends.
         */
        assert(
            stud_end <= opening_start ||
            stud_start >= opening_end
        );
    }

    wall_destroy(&wall);
}

static void test_opening_places_king_studs(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2040
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    int opening_start =
        opening->frame_position;

    int opening_end =
        opening_start +
        opening_frame_width(
            opening,
            &settings
        );

    int expected_left_trimmer =
        opening_start -
        settings.stud_width;

    int expected_right_trimmer =
        opening_end;

    int expected_left_king = 
        expected_left_trimmer - 
        settings.stud_width;

    int expected_right_king = 
        expected_right_trimmer + 
        settings.stud_width;

    bool found_left_king = false;
    bool found_right_king = false;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type !=
            STUD_KING) {
            continue;
        }

        int position =
            stud->position.x;

        if (position == expected_left_king) {
            found_left_king = true;
        }

        if (position == expected_right_king) {
            found_right_king = true;
        }
    }

    assert(found_left_king);
    assert(found_right_king);

    wall_destroy(&wall);
}

static void test_opening_clear_width_matches_frame_width(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2040
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    Timber *left_trimmer = NULL;
    Timber *right_trimmer = NULL;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type !=
            STUD_TRIMMER) {
            continue;
        }

        if (left_trimmer == NULL) {
            left_trimmer = stud;
        } else {
            right_trimmer = stud;
        }
    }

    assert(left_trimmer != NULL);
    assert(right_trimmer != NULL);

    int left_inside_face =
        left_trimmer->position.x
        + settings.stud_width;

    int right_inside_face =
        right_trimmer->position.x;

    int actual_clear_width =
        right_inside_face -
        left_inside_face;

    int expected_clear_width =
        opening_frame_width(
            opening,
            &settings
        );

    assert(
        actual_clear_width ==
        expected_clear_width
    );

    wall_destroy(&wall);
}

static void test_window_blocks_only_intersecting_noggins(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 800,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    int opening_left =
        opening->frame_position;

    int opening_right =
        opening_left +
        opening_frame_width(
            opening,
            &settings
        );

    int opening_bottom =
        opening->frame_bottom;

    int opening_top =
        opening_bottom +
        opening_frame_height(
            opening,
            &settings
        );

    for (size_t i = 0;
         i < wall.framing.nog_count;
         i++) {

        Timber *noggin =
            &wall.framing.nogs[i];

        size_t bay =
            noggin->details.noggin.bay;

        Timber *left =
            &wall.framing.studs[bay];

        Timber *right =
            &wall.framing.studs[bay + 1];

        int noggin_left =
            left->position.x +
            settings.stud_width;

        int noggin_right =
            right->position.x;

        int y =
            noggin->position.y;

        int horizontal_overlap =
            noggin_left < opening_right &&
            noggin_right > opening_left;

        int vertical_overlap =
            y > opening_bottom &&
            y < opening_top;

        assert(
            !(horizontal_overlap &&
              vertical_overlap)
        );
    }

    wall_destroy(&wall);
}

static void test_door_places_trimmer_studs(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2040
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    int opening_start =
        opening->frame_position;

    int opening_end =
        opening_start +
        opening_frame_width(
            opening,
            &settings
        );

    int expected_left_trimmer =
        opening_start -
        settings.stud_width;

    int expected_right_trimmer =
        opening_end;

    int expected_length =
        opening_frame_height(
            opening,
            &settings
        );

    bool found_left = false;
    bool found_right = false;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type !=
            STUD_TRIMMER) {
            continue;
        }

        assert(stud->length == expected_length);

        if (stud->position.x ==
            expected_left_trimmer) {

            found_left = true;
        }

        if (stud->position.x ==
            expected_right_trimmer) {

            found_right = true;
        }
    }

    assert(found_left);
    assert(found_right);

    wall_destroy(&wall);
}

static void test_noggin_coordinates(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));
    assert(wall_generate(&wall, &settings));

    for (size_t i = 0;
         i < wall.framing.nog_count;
         i++) {

        Timber *noggin =
            &wall.framing.nogs[i];

        size_t bay =
            noggin->details.noggin.bay;

        Timber *left =
            &wall.framing.studs[bay];

        Timber *right =
            &wall.framing.studs[bay + 1];

        /*
         * Noggin begins immediately after
         * the left stud.
         */
        assert(
            noggin->position.x ==
            left->position.x +
            settings.stud_width
        );

        /*
         * Its length should take it exactly
         * to the next stud.
         */
        assert(
            noggin->position.x +
            noggin->length ==
            right->position.x
        );

        /*
         * Noggins must exist somewhere
         * above the bottom and below the top.
         */
        assert(noggin->position.y > 0);
        assert(
            noggin->position.y <
            settings.stud_height
        );
    }

    wall_destroy(&wall);
}

static void test_door_does_not_generate_header(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_DOOR,
            1000,
            0,
            820,
            2040
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    bool found_header = false;

    for (size_t i = 0;
         i < wall.framing.member_count;
         i++) {

        Timber *member =
            &wall.framing.members[i];

        if (member->type == TIMBER_HEADER) {
            found_header = true;
            break;
        }
    }

    assert(!found_header);

    wall_destroy(&wall);
}

static void test_window_places_header_and_sill(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,

            1000,   /* frame_position */
            700,    /* frame_bottom */

            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    int frame_width =
        opening_frame_width(
            opening,
            &settings
        );

    int frame_height =
        opening_frame_height(
            opening,
            &settings
        );

    /*
     * HEADER
     *
     * The header sits across both trimmers.
     */
    int expected_header_x =
        opening->frame_position
        - settings.stud_width;

    int expected_header_y =
        opening->frame_bottom
        + frame_height;

    int expected_header_length =
        frame_width
        + (2 * settings.stud_width);

    /*
     * SILL
     *
     * The sill fits inside the trimmers,
     * so it spans only the clear opening.
     */
    int expected_sill_x =
        opening->frame_position;

    int expected_sill_y =
        opening->frame_bottom;

    int expected_sill_length =
        frame_width;

    bool found_header = false;
    bool found_sill = false;

    size_t header_count = 0;
    size_t sill_count = 0;

    for (size_t i = 0;
         i < wall.framing.member_count;
         i++) {

        Timber *member =
            &wall.framing.members[i];

        if (member->type == TIMBER_HEADER) {

            header_count++;
            found_header = true;

            assert(
                member->position.x ==
                expected_header_x
            );

            assert(
                member->position.y ==
                expected_header_y
            );

            assert(
                member->length ==
                expected_header_length
            );
        }

        if (member->type == TIMBER_SILL) {

            sill_count++;
            found_sill = true;

            assert(
                member->position.x ==
                expected_sill_x
            );

            assert(
                member->position.y ==
                expected_sill_y
            );

            assert(
                member->length ==
                expected_sill_length
            );
        }
    }

    assert(found_header);
    assert(found_sill);

    assert(header_count == 1);
    assert(sill_count == 1);

    wall_destroy(&wall);
}

static void test_window_places_lower_cripples(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    bool found_lower_cripple = false;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type !=
            STUD_CRIPPLE) {
            continue;
        }

        /*
         * For this first test, we only care
         * about cripples beginning at the
         * bottom framing reference.
         */
        if (stud->position.y != 0) {
            continue;
        }

        found_lower_cripple = true;

        /*
         * Lower cripples should finish
         * exactly at the sill.
         */
        assert(
            stud->length ==
            opening->frame_bottom
        );
    }

    assert(found_lower_cripple);

    wall_destroy(&wall);
}

static void test_window_lower_cripple_spacing(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(wall_generate(&wall, &settings));

    Opening *opening =
        &wall.definition.openings[0];

    int frame_width =
        opening_frame_width(
            opening,
            &settings
        );

    int expected_first_position =
        opening->frame_position;

    int expected_last_position =
        opening->frame_position
        + frame_width
        - settings.stud_width;

    Timber *previous_cripple = NULL;
    Timber *first_cripple = NULL;
    Timber *last_cripple = NULL;

    size_t cripple_count = 0;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type !=
            STUD_CRIPPLE) {
            continue;
        }

        /*
         * For now we're testing lower
         * cripples only.
         */
        if (stud->position.y != 0) {
            continue;
        }

        if (first_cripple == NULL) {
            first_cripple = stud;
        }

        /*
         * Every cripple must finish
         * at the underside of the sill.
         */
        assert(
            stud->length ==
            opening->frame_bottom
        );

        if (previous_cripple != NULL) {

            int spacing =
                stud->position.x
                - previous_cripple->position.x;

            assert(spacing > 0);

            assert(
                spacing <=
                settings.stud_spacing
            );
        }

        previous_cripple = stud;
        last_cripple = stud;

        cripple_count++;
    }

    assert(cripple_count >= 2);

    assert(first_cripple != NULL);
    assert(last_cripple != NULL);

    /*
     * The sill must be supported at
     * both ends.
     */
    assert(
        first_cripple->position.x ==
        expected_first_position
    );

    assert(
        last_cripple->position.x ==
        expected_last_position
    );

    wall_destroy(&wall);
}

static void test_window_places_upper_cripples(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    Opening *opening =
        &wall.definition.openings[0];

    int header_y =
        opening->frame_bottom +
        opening_frame_height(
            opening,
            &settings
        );

    int expected_y =
        header_y +
        settings.stud_width;

    int expected_length =
        settings.stud_height -
        expected_y;

    bool found_upper_cripple = false;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type !=
            STUD_CRIPPLE) {
            continue;
        }

        /*
         * Ignore lower cripples.
         */
        if (stud->position.y == 0) {
            continue;
        }

        found_upper_cripple = true;

        assert(
            stud->position.y ==
            expected_y
        );

        assert(
            stud->length ==
            expected_length
        );
    }

    assert(found_upper_cripple);

    wall_destroy(&wall);
}

static void test_window_upper_and_lower_cripples_align(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 4200));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    size_t lower_count = 0;
    size_t upper_count = 0;

    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *lower =
            &wall.framing.studs[i];

        if (lower->details.stud.type !=
            STUD_CRIPPLE) {
            continue;
        }

        /*
         * Only inspect lower cripples.
         */
        if (lower->position.y != 0) {
            continue;
        }

        lower_count++;

        bool found_matching_upper = false;

        for (size_t j = 0;
             j < wall.framing.stud_count;
             j++) {

            Timber *upper =
                &wall.framing.studs[j];

            if (upper->details.stud.type !=
                STUD_CRIPPLE) {
                continue;
            }

            if (upper->position.y == 0) {
                continue;
            }

            if (upper->position.x ==
                lower->position.x) {

                found_matching_upper = true;
                break;
            }
        }

        assert(found_matching_upper);
    }

    /*
     * Count upper cripples separately.
     */
    for (size_t i = 0;
         i < wall.framing.stud_count;
         i++) {

        Timber *stud =
            &wall.framing.studs[i];

        if (stud->details.stud.type ==
                STUD_CRIPPLE &&
            stud->position.y != 0) {

            upper_count++;
        }
    }

    assert(lower_count > 0);
    assert(upper_count > 0);

    /*
     * There should be a one-to-one
     * vertical alignment.
     */
    assert(lower_count == upper_count);

    wall_destroy(&wall);
}

static void test_reject_overlapping_opening_assemblies(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            5000
        )
    );

    /*
     * First opening.
     */
    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    /*
     * Second opening deliberately close
     * enough that the framing assemblies
     * collide.
     */
    assert(
        !wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID_2,
            OPENING_DOOR,
            1800,
            0,
            820,
            2040
        )
    );

    /*
     * Rejected opening must not have been
     * inserted into the wall.
     */
    assert(
        wall.definition.opening_count == 1
    );

    wall_destroy(&wall);
}

static void test_accept_separated_openings(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

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
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID_2,
            OPENING_DOOR,
            3000,
            0,
            820,
            2040
        )
    );

    assert(
        wall.definition.opening_count == 2
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    wall_destroy(&wall);
}

static void test_opening_does_not_break_max_stud_spacing(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .opening_width_allowance = 20,
        .opening_height_allowance = 20,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 5000));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1700,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert_valid_vertical_member_spacing(
        &wall,
        &settings
    );

    wall_destroy(&wall);
}

static void test_all_vertical_member_spacing_with_openings(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    assert(wall_set_length(&wall, 6000));

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1400,
            700,
            820,
            1000
        )
    );

    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID_2,
            OPENING_DOOR,
            3500,
            0,
            820,
            2040
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert_valid_vertical_member_spacing(
        &wall,
        &settings
    );

    wall_destroy(&wall);
}

static void test_opening_positions_preserve_spacing(void)
{
    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    for (int opening_x = 200;
         opening_x <= 3500;
         opening_x += 25) {

        Wall wall = {0};

        assert(
            wall_set_length(
                &wall,
                5000
            )
        );

        if (!wall_add_opening(
                &wall,
                &settings,
                TEST_OPENING_ID,
                OPENING_WINDOW,
                opening_x,
                700,
                820,
                1000)) {

            wall_destroy(&wall);
            continue;
        }

        assert(
            wall_generate(
                &wall,
                &settings
            )
        );

        assert_valid_vertical_member_spacing(
            &wall,
            &settings
        );

        wall_destroy(&wall);
    }
}

static void test_repairs_spacing_after_opening(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            6000
        )
    );

    /*
     * This placement previously produced:
     *
     * king   = 1325
     * common = 1800
     *
     * spacing = 475 > 450
     */
    assert(
        wall_add_opening(
            &wall,
            &settings,
            TEST_OPENING_ID,
            OPENING_WINDOW,
            450,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert_valid_vertical_member_spacing(
        &wall,
        &settings
    );

    wall_destroy(&wall);
}

static void test_wall_destroy_resets_generated_wall(void)
{
    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    Wall wall = {0};

    assert(wall_set_length(
        &wall,
        4200
    ));

    assert(wall_generate(
        &wall,
        &settings
    ));

    wall_destroy(
        &wall
    );

    assert(wall.framing.studs == NULL);
    assert(wall.framing.stud_count == 0);
    assert(wall.framing.stud_capacity == 0);

    assert(wall.framing.nogs == NULL);
    assert(wall.framing.nog_count == 0);
    assert(wall.framing.nog_capacity == 0);

    assert(wall.framing.members == NULL);
    assert(wall.framing.member_count == 0);
    assert(wall.framing.member_capacity == 0);

    assert(wall.definition.openings == NULL);
    assert(wall.definition.opening_count == 0);
    assert(wall.definition.opening_capacity == 0);
}

static void test_wall_destroy_accepts_null(void)
{
    wall_destroy(NULL);
}

static void test_wall_destroy_can_be_called_twice(void)
{
    Wall wall = {0};

    wall_destroy(&wall);
    wall_destroy(&wall);
}

static void
test_wall_definition_survives_framing_destroy(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,

        .opening_width_allowance = 20,
        .opening_height_allowance = 20,

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
            TEST_OPENING_ID,
            OPENING_WINDOW,
            1000,
            700,
            820,
            1000
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert(
        wall.definition.length ==
        4200
    );

    assert(
        wall.definition.opening_count ==
        1
    );

    Opening expected =
        wall.definition.openings[0];

    assert(
        wall.framing.stud_count > 0
    );

    wall_framing_destroy(
        &wall.framing
    );

    /*
     * Authoritative state survives.
     */
    assert(
        wall.definition.length ==
        4200
    );

    assert(
        wall.definition.opening_count ==
        1
    );

    Opening *opening =
        &wall.definition.openings[0];

    assert(
        opening->type ==
        expected.type
    );

    assert(
        opening->frame_position ==
        expected.frame_position
    );

    assert(
        opening->frame_bottom ==
        expected.frame_bottom
    );

    assert(
        opening->width ==
        expected.width
    );

    assert(
        opening->height ==
        expected.height
    );

    /*
     * Generated state is gone.
     */
    assert(
        wall.framing.studs == NULL
    );

    assert(
        wall.framing.stud_count == 0
    );

    assert(
        wall.framing.nogs == NULL
    );

    assert(
        wall.framing.nog_count == 0
    );

    assert(
        wall.framing.members == NULL
    );

    assert(
        wall.framing.member_count == 0
    );

    assert(
        wall.framing.bottomplate.length ==
        0
    );

    assert(
        wall.framing.topplate.length ==
        0
    );

    /*
     * Same definition can regenerate
     * the framing.
     */
    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert(
        wall.definition.length ==
        4200
    );

    assert(
        wall.definition.opening_count ==
        1
    );

    assert(
        wall.framing.stud_count > 0
    );

    assert(
        wall.framing.bottomplate.length ==
        4200
    );

    wall_destroy(&wall);
}

static void
test_failed_wall_regeneration_preserves_existing_framing(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_width = 35,
        .stud_depth = 90,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            4200
        )
    );

    /*
     * Establish a valid committed framing.
     */
    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    assert(
        wall.framing.stud_count > 0
    );

    assert(
        wall.framing.nog_count > 0
    );

    Timber *studs_before =
        wall.framing.studs;

    Timber *nogs_before =
        wall.framing.nogs;

    Timber *members_before =
        wall.framing.members;

    size_t stud_count_before =
        wall.framing.stud_count;

    size_t stud_capacity_before =
        wall.framing.stud_capacity;

    size_t nog_count_before =
        wall.framing.nog_count;

    size_t nog_capacity_before =
        wall.framing.nog_capacity;

    size_t member_count_before =
        wall.framing.member_count;

    size_t member_capacity_before =
        wall.framing.member_capacity;

    Timber bottomplate_before =
        wall.framing.bottomplate;

    Timber topplate_before =
        wall.framing.topplate;

    /*
     * Force generation to fail after it
     * has entered the generation process.
     */
    settings.stud_spacing_mode =
        (StudSpacingMode)999;

    assert(
        !wall_generate(
            &wall,
            &settings
        )
    );

    /*
     * Failure must preserve the previously
     * committed framing exactly.
     */
    assert(
        wall.framing.studs ==
        studs_before
    );

    assert(
        wall.framing.nogs ==
        nogs_before
    );

    assert(
        wall.framing.members ==
        members_before
    );

    assert(
        wall.framing.stud_count ==
        stud_count_before
    );

    assert(
        wall.framing.stud_capacity ==
        stud_capacity_before
    );

    assert(
        wall.framing.nog_count ==
        nog_count_before
    );

    assert(
        wall.framing.nog_capacity ==
        nog_capacity_before
    );

    assert(
        wall.framing.member_count ==
        member_count_before
    );

    assert(
        wall.framing.member_capacity ==
        member_capacity_before
    );

    assert(
        wall.framing.bottomplate.length ==
        bottomplate_before.length
    );

    assert(
        wall.framing.bottomplate.position.y ==
        bottomplate_before.position.y
    );

    assert(
        wall.framing.topplate.length ==
        topplate_before.length
    );

    assert(
        wall.framing.topplate.position.y ==
        topplate_before.position.y
    );

    wall_destroy(
        &wall
    );
}

int main(void)
{
    test_wall_generates_complete_plates();
    test_maximise_spacing();
    test_even_spacing();
    test_noggins();
    test_maximise_uses_standard_spacing();
    test_wall_regeneration();
    test_add_opening();
    test_reject_opening_outside_wall();
    test_reject_opening_too_tall();
    test_opening_removes_interfering_studs();
    test_opening_places_king_studs();
    test_opening_clear_width_matches_frame_width();
    test_window_blocks_only_intersecting_noggins();
    test_door_places_trimmer_studs();
    test_noggin_coordinates();
    test_door_does_not_generate_header();
    test_window_places_header_and_sill();
    test_window_places_lower_cripples();
    test_window_lower_cripple_spacing();
    test_window_places_upper_cripples();
    test_window_upper_and_lower_cripples_align();
    test_reject_overlapping_opening_assemblies();
    test_accept_separated_openings();
    test_opening_does_not_break_max_stud_spacing();
    test_all_vertical_member_spacing_with_openings();
    test_opening_positions_preserve_spacing();
    test_property_single_opening_geometry();
    test_repairs_spacing_after_opening();
    
    test_wall_destroy_resets_generated_wall();
    test_wall_destroy_accepts_null();
    test_wall_destroy_can_be_called_twice();

    test_wall_definition_survives_framing_destroy();

    test_failed_wall_regeneration_preserves_existing_framing();

    printf("All sitehelper tests passed.\n");

    return 0;
}