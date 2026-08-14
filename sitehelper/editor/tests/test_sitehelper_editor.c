#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"

static Timber make_test_stud(void)
{
    Timber stud = {
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

    return stud;
}

static void test_editor_init_has_no_current_room(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        editor.current_room_id ==
        DOMAIN_ID_INVALID
    );
}

static void test_editor_init_has_no_current_wall(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        editor.current_wall_id ==
        DOMAIN_ID_INVALID
    );
}

static void test_editor_init_defaults_to_select_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        sitehelper_editor_get_active_tool(
            &editor
        )
        == EDITOR_TOOL_SELECT
    );
}

static void test_editor_can_change_active_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    int result =
        sitehelper_editor_set_active_tool(
            &editor,
            EDITOR_TOOL_OPENING
        );

    assert(result == 1);

    assert(
        sitehelper_editor_get_active_tool(
            &editor
        )
        == EDITOR_TOOL_OPENING
    );
}

static void test_editor_rejects_invalid_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    int result =
        sitehelper_editor_set_active_tool(
            &editor,
            EDITOR_TOOL_COUNT
        );

    assert(result == 0);

    assert(
        sitehelper_editor_get_active_tool(
            &editor
        )
        == EDITOR_TOOL_SELECT
    );
}

static void test_editor_initialises_without_selection(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    const EditorSelection *selection =
        sitehelper_editor_get_selection(
            &editor
        );

    assert(selection != NULL);

    assert(
        editor_selection_is_empty(
            selection
        )
    );
}

static void test_editor_selects_wall_member_at_position(void)
{
    Timber stud =
        make_test_stud();

    Wall wall = {
        .id = 10,

        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_select_wall_member_at_position(
        &editor,
        &wall,
        (Position){
            .x = 120,
            .y = 1000
        }
    );

    const EditorSelection *selection =
        sitehelper_editor_get_selection(
            &editor
        );

    const WallSelection *wall_selection =
        editor_selection_get_wall_member(
            selection,
            wall.id
        );

    assert(
        wall_selection != NULL
    );

    assert(
        !wall_selection_is_empty(
            wall_selection
        )
    );

    assert(
        wall_selection_resolve(
            wall_selection,
            &wall
        ) == &stud
    );
}

static void test_editor_clicking_empty_space_clears_selection(void)
{
    Timber stud =
        make_test_stud();

    Wall wall = {
        .id = 10,

        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_select_wall_member_at_position(
        &editor,
        &wall,
        (Position){
            .x = 120,
            .y = 1000
        }
    );

    assert(
        !editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );

    sitehelper_editor_select_wall_member_at_position(
        &editor,
        &wall,
        (Position){
            .x = 500,
            .y = 1000
        }
    );

    assert(
        editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );
}

static void test_editor_clear_selection_clears_selection(void)
{
    Timber stud =
        make_test_stud();

    Wall wall = {
        .id = 10,

        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_select_wall_member_at_position(
        &editor,
        &wall,
        (Position){
            .x = 120,
            .y = 1000
        }
    );

    assert(
        !editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );

    sitehelper_editor_clear_selection(
        &editor
    );

    assert(
        editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );
}

static void test_editor_selecting_null_wall_preserves_selection(void)
{
    Timber stud =
        make_test_stud();

    Wall wall = {
        .id = 10,

        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_select_wall_member_at_position(
        &editor,
        &wall,
        (Position){
            .x = 120,
            .y = 1000
        }
    );

    assert(
        !editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );

    sitehelper_editor_select_wall_member_at_position(
        &editor,
        NULL,
        (Position){
            .x = 0,
            .y = 0
        }
    );

    assert(
        !editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );
}

int main(void)
{
    test_editor_init_has_no_current_room();
    test_editor_init_has_no_current_wall();
    test_editor_init_defaults_to_select_tool();
    test_editor_can_change_active_tool();
    test_editor_rejects_invalid_tool();
    test_editor_initialises_without_selection();
    test_editor_selects_wall_member_at_position();
    test_editor_clicking_empty_space_clears_selection();
    test_editor_clear_selection_clears_selection();
    test_editor_selecting_null_wall_preserves_selection();
    

    printf(
        "All SiteHelper editor tests passed.\n"
    );

    return 0;
}