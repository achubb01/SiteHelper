#include <assert.h>
#include <stdio.h>

#include "test_support.h"

static BuildSettings test_settings(StudSpacingMode mode)
{
    return (BuildSettings){
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 450,
        .nog_spacing = 900,
        .opening_width_allowance = 20,
        .opening_height_allowance = 20,
        .stud_spacing_mode = mode
    };
}

static void configure_wall(
    Wall *wall,
    const BuildSettings *settings
)
{
    assert(wall != NULL);
    assert(settings != NULL);

    wall->id = 100;

    assert(wall_set_origin(
        wall,
        (Position){ .x = 5000, .y = 3000 }
    ));

    assert(wall_set_length(wall, 6000));

    assert(wall_add_opening(
        wall,
        settings,
        101,
        OPENING_WINDOW,
        900,
        700,
        820,
        1000
    ));

    assert(wall_add_opening(
        wall,
        settings,
        102,
        OPENING_DOOR,
        3300,
        0,
        900,
        2040
    ));
}

static int same_physical_member(
    const Timber *a,
    const Timber *b
)
{
    return
        a->type == b->type &&
        a->length == b->length &&
        a->depth == b->depth &&
        a->width == b->width &&
        a->position.x == b->position.x &&
        a->position.y == b->position.y;
}

static void assert_no_duplicate_members(
    const Timber *members,
    size_t member_count
)
{
    for (size_t i = 0; i < member_count; i++) {
        for (size_t j = i + 1; j < member_count; j++) {
            assert(!same_physical_member(&members[i], &members[j]));
        }
    }
}

static void test_generation_is_semantically_deterministic(void)
{
    const StudSpacingMode modes[] = {
        STUD_SPACING_EVEN,
        STUD_SPACING_MAXIMISE
    };

    for (size_t i = 0; i < sizeof modes / sizeof modes[0]; i++) {
        BuildSettings settings = test_settings(modes[i]);
        Wall first = {0};
        Wall second = {0};

        configure_wall(&first, &settings);
        test_clone_wall_definition(&first, &second);

        assert(wall_generate(&first, &settings));
        assert(wall_generate(&second, &settings));

        test_assert_framing_semantically_equal(
            &first.framing,
            &second.framing
        );

        /* Replacing an existing framing must produce the same structure. */
        assert(wall_generate(&first, &settings));

        test_assert_framing_semantically_equal(
            &first.framing,
            &second.framing
        );

        wall_destroy(&first);
        wall_destroy(&second);
    }
}

static void test_generation_has_no_duplicate_structural_members(void)
{
    const StudSpacingMode modes[] = {
        STUD_SPACING_EVEN,
        STUD_SPACING_MAXIMISE
    };

    for (size_t i = 0; i < sizeof modes / sizeof modes[0]; i++) {
        BuildSettings settings = test_settings(modes[i]);
        Wall wall = {0};

        configure_wall(&wall, &settings);
        assert(wall_generate(&wall, &settings));

        assert_no_duplicate_members(
            wall.framing.studs,
            wall.framing.stud_count
        );

        assert_no_duplicate_members(
            wall.framing.nogs,
            wall.framing.nog_count
        );

        assert_no_duplicate_members(
            wall.framing.members,
            wall.framing.member_count
        );

        wall_destroy(&wall);
    }
}

int main(void)
{
    test_generation_is_semantically_deterministic();
    test_generation_has_no_duplicate_structural_members();

    puts("wall generation invariant tests passed");
    return 0;
}
