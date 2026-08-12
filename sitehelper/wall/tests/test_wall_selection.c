#include <assert.h>
#include <stdio.h>

#include "wall_selection.h"


static Timber make_common_stud(
    int x
)
{
    Timber timber = {
        .length = 2400,
        .depth = 90,
        .width = 35,

        .position = {
            .x = x,
            .y = 0
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = STUD_COMMON
        }
    };

    return timber;
}


static void test_initialises_empty(void)
{
    WallSelection selection;

    wall_selection_init(
        &selection
    );

    assert(
        wall_selection_is_empty(
            &selection
        )
    );
}


static void test_clear_makes_selection_empty(void)
{
    WallSelection selection;

    wall_selection_init(
        &selection
    );

    Timber timber =
        make_common_stud(
            600
        );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &timber
    );

    assert(
        !wall_selection_is_empty(
            &selection
        )
    );

    wall_selection_clear(
        &selection
    );

    assert(
        wall_selection_is_empty(
            &selection
        )
    );
}


static void test_set_stores_value_not_pointer(void)
{
    WallSelection selection;

    wall_selection_init(
        &selection
    );

    Timber timber =
        make_common_stud(
            600
        );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &timber
    );

    timber.position.x =
        9999;

    assert(
        selection.timber.position.x ==
        600
    );

    assert(
        selection.kind ==
        WALL_MEMBER_STUD
    );
}


static void test_setting_null_timber_clears_selection(void)
{
    WallSelection selection;

    wall_selection_init(
        &selection
    );

    Timber timber =
        make_common_stud(
            600
        );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &timber
    );

    assert(
        !wall_selection_is_empty(
            &selection
        )
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        NULL
    );

    assert(
        wall_selection_is_empty(
            &selection
        )
    );
}


static void test_setting_none_kind_clears_selection(void)
{
    WallSelection selection;

    wall_selection_init(
        &selection
    );

    Timber timber =
        make_common_stud(
            600
        );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &timber
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_NONE,
        &timber
    );

    assert(
        wall_selection_is_empty(
            &selection
        )
    );
}


static void test_resolves_selected_stud(void)
{
    Timber studs[] = {
        make_common_stud(0),
        make_common_stud(600),
        make_common_stud(1200)
    };

    Wall wall = {0};

    wall.framing.studs =
        studs;

    wall.framing.stud_count =
        3;

    WallSelection selection;

    wall_selection_init(
        &selection
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &studs[1]
    );

    const Timber *resolved =
        wall_selection_resolve(
            &selection,
            &wall
        );

    assert(
        resolved ==
        &studs[1]
    );
}


static void test_selection_resolves_after_framing_is_replaced(void)
{
    Timber original_studs[] = {
        make_common_stud(0),
        make_common_stud(600),
        make_common_stud(1200)
    };

    Wall wall = {0};

    wall.framing.studs =
        original_studs;

    wall.framing.stud_count =
        3;

    WallSelection selection;

    wall_selection_init(
        &selection
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &original_studs[1]
    );

    const Timber *before =
        wall_selection_resolve(
            &selection,
            &wall
        );

    assert(
        before ==
        &original_studs[1]
    );

    /*
     * Simulate regeneration.
     *
     * The wall now owns completely different
     * framing storage containing an equivalent
     * generated stud.
     */
    Timber regenerated_studs[] = {
        make_common_stud(0),
        make_common_stud(600),
        make_common_stud(1200)
    };

    wall.framing.studs =
        regenerated_studs;

    wall.framing.stud_count =
        3;

    const Timber *after =
        wall_selection_resolve(
            &selection,
            &wall
        );

    assert(
        after ==
        &regenerated_studs[1]
    );

    assert(
        after != before
    );
}


static void test_resolve_returns_null_when_member_no_longer_exists(void)
{
    Timber original_studs[] = {
        make_common_stud(0),
        make_common_stud(600),
        make_common_stud(1200)
    };

    Wall wall = {0};

    wall.framing.studs =
        original_studs;

    wall.framing.stud_count =
        3;

    WallSelection selection;

    wall_selection_init(
        &selection
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &original_studs[1]
    );

    /*
     * Simulate regeneration where the
     * selected x=600 stud disappears.
     */
    Timber regenerated_studs[] = {
        make_common_stud(0),
        make_common_stud(1200)
    };

    wall.framing.studs =
        regenerated_studs;

    wall.framing.stud_count =
        2;

    assert(
        wall_selection_resolve(
            &selection,
            &wall
        ) == NULL
    );
}


static void test_reconcile_clears_missing_member(void)
{
    Timber original_studs[] = {
        make_common_stud(600)
    };

    Wall wall = {0};

    wall.framing.studs =
        original_studs;

    wall.framing.stud_count =
        1;

    WallSelection selection;

    wall_selection_init(
        &selection
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &original_studs[0]
    );

    wall.framing.studs =
        NULL;

    wall.framing.stud_count =
        0;

    wall_selection_reconcile(
        &selection,
        &wall
    );

    assert(
        wall_selection_is_empty(
            &selection
        )
    );
}


static void test_kind_prevents_cross_category_resolution(void)
{
    Timber stud =
        make_common_stud(
            600
        );

    Timber noggin = {
        .length = stud.length,
        .depth = stud.depth,
        .width = stud.width,

        .position = stud.position,

        .type = TIMBER_STUD,

        .details.stud = {
            .type = STUD_COMMON
        }
    };

    Wall wall = {0};

    wall.framing.nogs =
        &noggin;

    wall.framing.nog_count =
        1;

    WallSelection selection;

    wall_selection_init(
        &selection
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_STUD,
        &stud
    );

    /*
     * Even though an equivalent Timber value
     * exists in another framing category,
     * a STUD selection must only search studs.
     */
    assert(
        wall_selection_resolve(
            &selection,
            &wall
        ) == NULL
    );
}


static void test_bottom_plate_resolves(void)
{
    Wall wall = {0};

    wall.framing.bottomplate = (Timber){
        .length = 4200,
        .depth = 90,
        .width = 35,

        .position = {
            .x = 0,
            .y = 0
        },

        .type = TIMBER_PLATE
    };

    WallSelection selection;

    wall_selection_init(
        &selection
    );

    wall_selection_set(
        &selection,
        WALL_MEMBER_BOTTOM_PLATE,
        &wall.framing.bottomplate
    );

    assert(
        wall_selection_resolve(
            &selection,
            &wall
        ) ==
        &wall.framing.bottomplate
    );
}


static void test_functions_accept_null_selection(void)
{
    Timber timber =
        make_common_stud(
            600
        );

    Wall wall = {0};

    wall_selection_init(
        NULL
    );

    wall_selection_clear(
        NULL
    );

    wall_selection_set(
        NULL,
        WALL_MEMBER_STUD,
        &timber
    );

    wall_selection_reconcile(
        NULL,
        &wall
    );

    assert(
        wall_selection_is_empty(
            NULL
        )
    );

    assert(
        wall_selection_resolve(
            NULL,
            &wall
        ) == NULL
    );

    assert(
        wall_selection_resolve(
            NULL,
            NULL
        ) == NULL
    );
}


int main(void)
{
    test_initialises_empty();
    test_clear_makes_selection_empty();
    test_set_stores_value_not_pointer();
    test_setting_null_timber_clears_selection();
    test_setting_none_kind_clears_selection();

    test_resolves_selected_stud();
    test_selection_resolves_after_framing_is_replaced();
    test_resolve_returns_null_when_member_no_longer_exists();
    test_reconcile_clears_missing_member();
    test_kind_prevents_cross_category_resolution();
    test_bottom_plate_resolves();

    test_functions_accept_null_selection();

    printf(
        "All wall selection tests passed.\n"
    );

    return 0;
}