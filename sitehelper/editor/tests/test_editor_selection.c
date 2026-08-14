#include <assert.h>
#include <stdio.h>

#include "editor_selection.h"

static Timber make_test_stud(void)
{
    return (Timber){
        .length = 2400,
        .depth = 90,
        .width = 90,

        .position = {
            .x = 100,
            .y = 0
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = STUD_COMMON
        }
    };
}

static void test_selection_initialises_empty(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    assert(
        editor_selection_is_empty(
            &selection
        )
    );
}

static void test_selection_can_store_wall_member(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        10,
        WALL_MEMBER_STUD,
        &stud
    );

    assert(
        !editor_selection_is_empty(
            &selection
        )
    );

    assert(
        editor_selection_get_wall_member(
            &selection,
            10
        ) != NULL
    );
}

static void test_wall_member_selection_is_scoped_to_wall(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        10,
        WALL_MEMBER_STUD,
        &stud
    );

    assert(
        editor_selection_get_wall_member(
            &selection,
            11
        ) == NULL
    );
}

static void test_selection_can_be_cleared(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        10,
        WALL_MEMBER_STUD,
        &stud
    );

    editor_selection_clear(
        &selection
    );

    assert(
        editor_selection_is_empty(
            &selection
        )
    );
}

static void test_invalid_wall_clears_selection(void)
{
    EditorSelection selection;

    editor_selection_init(
        &selection
    );

    Timber stud =
        make_test_stud();

    editor_selection_set_wall_member(
        &selection,
        DOMAIN_ID_INVALID,
        WALL_MEMBER_STUD,
        &stud
    );

    assert(
        editor_selection_is_empty(
            &selection
        )
    );
}

int main(void)
{
    test_selection_initialises_empty();
    test_selection_can_store_wall_member();
    test_wall_member_selection_is_scoped_to_wall();
    test_selection_can_be_cleared();
    test_invalid_wall_clears_selection();

    printf(
        "All editor selection tests passed.\n"
    );

    return 0;
}