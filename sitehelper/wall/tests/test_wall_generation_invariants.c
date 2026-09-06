#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "test_support.h"

_Static_assert(_Generic((PlanPosition){0}, WallLocalPosition: 0, default: 1),
    "Plan and wall-local positions must be distinct types");
_Static_assert(_Generic(((WallDefinition){0}).segment.start, PlanPosition: 1, default: 0),
    "Wall endpoints must be plan positions");
_Static_assert(_Generic(((WallDefinition){0}).segment.end, PlanPosition: 1, default: 0),
    "Wall endpoints must be plan positions");
_Static_assert(_Generic(((Timber){0}).position, WallLocalPosition: 1, default: 0),
    "Timber must have a wall-local position");

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

    assert(wall_set_plan_segment(wall, (WallPlanSegment){
        .start = { .x = 5000, .y = 3000 },
        .end = { .x = 11000, .y = 3000 }
    }));

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
        a->position.u == b->position.u &&
        a->position.z == b->position.z;
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

static void test_plan_placement_and_direction_do_not_affect_local_framing(void)
{
    const StudSpacingMode modes[] = {
        STUD_SPACING_EVEN,
        STUD_SPACING_MAXIMISE
    };
    const WallPlanSegment segments[] = {
        { .start = {0, 0}, .end = {6000, 0} },
        { .start = {6000, 0}, .end = {0, 0} },
        { .start = {0, 0}, .end = {0, 6000} },
        { .start = {1000, 2000}, .end = {4600, 6800} },
        { .start = {4600, 6800}, .end = {1000, 2000} },
        { .start = {5000, -8000}, .end = {11000, -8000} },
        { .start = {-4200, 7600}, .end = {1800, 7600} },
        { .start = {INT_MAX, INT_MIN}, .end = {INT_MAX - 6000, INT_MIN} }
    };

    for (size_t m = 0; m < sizeof modes / sizeof modes[0]; m++) {
        BuildSettings settings = test_settings(modes[m]);
        Wall baseline = {0};
        configure_wall(&baseline, &settings);
        assert(wall_generate(&baseline, &settings));

        /* Exercise all generated member collections, including headers/sill. */
        assert(baseline.framing.stud_count > 0);
        assert(baseline.framing.nog_count > 0);
        size_t headers = 0;
        size_t sills = 0;
        for (size_t i = 0; i < baseline.framing.member_count; i++) {
            headers += baseline.framing.members[i].type == TIMBER_HEADER;
            sills += baseline.framing.members[i].type == TIMBER_SILL;
        }
        /* The current door path adds trimmers only; the window adds a header. */
        assert(headers == 1);
        assert(sills == 1);

        for (size_t p = 0; p < sizeof segments / sizeof segments[0]; p++) {
            Wall placed = {0};
            test_clone_wall_definition(&baseline, &placed);
            assert(wall_set_plan_segment(&placed, segments[p]));
            assert(wall_length_mm(&placed) == 6000);
            assert(wall_generate(&placed, &settings));

            test_assert_framing_semantically_equal(
                &baseline.framing, &placed.framing
            );
            assert(placed.definition.segment.start.x == segments[p].start.x);
            assert(placed.definition.segment.start.y == segments[p].start.y);
            assert(placed.definition.segment.end.x == segments[p].end.x);
            assert(placed.definition.segment.end.y == segments[p].end.y);

            /* Changing placement after generation must also be harmless. */
            assert(wall_set_plan_segment(&placed, segments[(p + 1) %
                (sizeof segments / sizeof segments[0])]));
            assert(wall_generate(&placed, &settings));
            test_assert_framing_semantically_equal(
                &baseline.framing, &placed.framing
            );
            wall_destroy(&placed);
        }
        wall_destroy(&baseline);
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
    test_plan_placement_and_direction_do_not_affect_local_framing();
    test_generation_has_no_duplicate_structural_members();

    puts("wall generation invariant tests passed");
    return 0;
}
