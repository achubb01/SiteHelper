#include <assert.h>
#include <stdio.h>

#include "wall_editor.h"
#include "wall_selection.h"


static void test_wall_editor_initialises_without_selection(void)
{
    WallEditor editor;

    wall_editor_init(&editor);

    const WallSelection *selection =
        wall_editor_get_selection(&editor);

    assert(selection != NULL);
    assert(wall_selection_get(selection) == NULL);
}

static void test_wall_editor_selects_timber_at_position(void)
{
    Timber stud = {
        .position = {
            .x = 100,
            .y = 0
        },
        .width = 90,
        .length = 2400
    };

    Wall wall = {
        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    WallEditor editor;
    wall_editor_init(&editor);

    Position click = {
        .x = 120,
        .y = 1000
    };

    wall_editor_select_at_position(
        &editor,
        &wall,
        click
    );

    const WallSelection *selection =
        wall_editor_get_selection(&editor);

    assert(
        wall_selection_get(selection) == &stud
    );
}

static void test_wall_editor_clicking_empty_space_clears_selection(void)
{
    Timber stud = {
        .position = {
            .x = 100,
            .y = 0
        },
        .width = 90,
        .length = 2400
    };

    Wall wall = {
        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    WallEditor editor;
    wall_editor_init(&editor);

    wall_editor_select_at_position(
        &editor,
        &wall,
        (Position){120, 1000}
    );

    assert(
        wall_selection_get(
            wall_editor_get_selection(&editor)
        ) == &stud
    );

    wall_editor_select_at_position(
        &editor,
        &wall,
        (Position){500, 1000}
    );

    assert(
        wall_selection_get(
            wall_editor_get_selection(&editor)
        ) == NULL
    );
}

static void test_wall_editor_clear_selection_clears_selection(void)
{
    Timber stud = {
        .position = {
            .x = 100,
            .y = 0
        },
        .width = 90,
        .length = 2400
    };

    Wall wall = {
        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    WallEditor editor;
    wall_editor_init(&editor);

    wall_editor_select_at_position(
        &editor,
        &wall,
        (Position){120, 1000}
    );

    wall_editor_clear_selection(&editor);

    assert(
        wall_selection_get(
            wall_editor_get_selection(&editor)
        ) == NULL
    );
}

static void test_wall_editor_init_accepts_null(void)
{
    wall_editor_init(NULL);
}

static void test_wall_editor_clear_selection_accepts_null(void)
{
    wall_editor_clear_selection(NULL);
}

static void test_wall_editor_select_accepts_null_editor(void)
{
    Wall wall = {0};

    wall_editor_select_at_position(
        NULL,
        &wall,
        (Position){0, 0}
    );
}

static void test_wall_editor_get_selection_accepts_null(void)
{
    assert(
        wall_editor_get_selection(NULL) == NULL
    );
}

static void test_wall_editor_selecting_null_wall_preserves_selection(void)
{
    Timber stud = {
        .position = {
            .x = 100,
            .y = 0
        },
        .width = 90,
        .length = 2400
    };

    Wall wall = {
        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    WallEditor editor;
    wall_editor_init(&editor);

    wall_editor_select_at_position(
        &editor,
        &wall,
        (Position){120, 1000}
    );

    wall_editor_select_at_position(
        &editor,
        NULL,
        (Position){0, 0}
    );

    assert(
        wall_selection_get(
            wall_editor_get_selection(&editor)
        ) == &stud
    );
}

int main(void)
{
    test_wall_editor_initialises_without_selection();
    test_wall_editor_selects_timber_at_position();
    test_wall_editor_clicking_empty_space_clears_selection();
    test_wall_editor_clear_selection_clears_selection();

    test_wall_editor_init_accepts_null();
    test_wall_editor_clear_selection_accepts_null();
    test_wall_editor_select_accepts_null_editor();
    test_wall_editor_get_selection_accepts_null();
    test_wall_editor_selecting_null_wall_preserves_selection();

    printf("All wall editor tests passed.\n");

    return 0;
}

