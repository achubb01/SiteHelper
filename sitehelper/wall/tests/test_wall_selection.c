#include <assert.h>
#include <stdio.h>

#include "wall_selection.h"


static void test_initialises_with_no_selection(void)
{
    WallSelection selection;

    wall_selection_init(&selection);

    assert(
        wall_selection_get(&selection) ==
        NULL
    );
}


static void test_sets_selected_timber(void)
{
    Timber timber = {
        .length = 2400,
        .width = 35,
        .depth = 90,
        .position = {
            .x = 450,
            .y = 0
        },
        .type = TIMBER_STUD
    };

    WallSelection selection;

    wall_selection_init(&selection);

    wall_selection_set(
        &selection,
        &timber
    );

    assert(
        wall_selection_get(&selection) ==
        &timber
    );
}


static void test_contains_selected_timber(void)
{
    Timber selected_timber = {
        .type = TIMBER_STUD
    };

    Timber other_timber = {
        .type = TIMBER_NOGGIN
    };

    WallSelection selection;

    wall_selection_init(&selection);

    wall_selection_set(
        &selection,
        &selected_timber
    );

    assert(
        wall_selection_contains(
            &selection,
            &selected_timber
        )
    );

    assert(
        !wall_selection_contains(
            &selection,
            &other_timber
        )
    );
}


static void test_clears_selected_timber(void)
{
    Timber timber = {
        .type = TIMBER_STUD
    };

    WallSelection selection;

    wall_selection_init(&selection);

    wall_selection_set(
        &selection,
        &timber
    );

    wall_selection_clear(&selection);

    assert(
        wall_selection_get(&selection) ==
        NULL
    );
}


static void test_setting_null_clears_selection(void)
{
    Timber timber = {
        .type = TIMBER_STUD
    };

    WallSelection selection;

    wall_selection_init(&selection);

    wall_selection_set(
        &selection,
        &timber
    );

    wall_selection_set(
        &selection,
        NULL
    );

    assert(
        wall_selection_get(&selection) ==
        NULL
    );
}


static void test_functions_accept_null_selection(void)
{
    Timber timber = {
        .type = TIMBER_STUD
    };

    wall_selection_init(NULL);
    wall_selection_clear(NULL);

    wall_selection_set(
        NULL,
        &timber
    );

    assert(
        wall_selection_get(NULL) ==
        NULL
    );

    assert(
        !wall_selection_contains(
            NULL,
            &timber
        )
    );
}


static void test_contains_rejects_null_timber(void)
{
    WallSelection selection;

    wall_selection_init(&selection);

    assert(
        !wall_selection_contains(
            &selection,
            NULL
        )
    );
}


int main(void)
{
    test_initialises_with_no_selection();
    test_sets_selected_timber();
    test_contains_selected_timber();
    test_clears_selected_timber();
    test_setting_null_clears_selection();
    test_functions_accept_null_selection();
    test_contains_rejects_null_timber();

    printf(
        "All wall selection tests passed.\n"
    );

    return 0;
}