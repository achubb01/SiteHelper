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

static void test_editor_initialises_without_snap(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        !sitehelper_editor_has_snap(
            &editor
        )
    );
}

static void test_editor_exposes_default_snap_settings(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    const SnapSettings *settings =
        sitehelper_editor_get_snap_settings(
            &editor
        );

    assert(settings != NULL);

    assert(settings->grid_enabled == 1);
    assert(settings->grid_spacing == 100.0);
}

static void test_editor_can_store_snap_result(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_snap_result(
        &editor,
        (SnapResult){
            .position = {
                .x = 300.0,
                .y = 500.0
            },

            .type = SNAP_ENDPOINT
        }
    );

    assert(
        sitehelper_editor_has_snap(
            &editor
        )
    );

    const SnapResult *result =
        sitehelper_editor_get_snap_result(
            &editor
        );

    assert(result != NULL);

    assert(result->position.x == 300.0);
    assert(result->position.y == 500.0);
    assert(result->type == SNAP_ENDPOINT);
}

static void test_editor_can_clear_snap(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_snap_result(
        &editor,
        (SnapResult){
            .position = {
                .x = 300.0,
                .y = 500.0
            },

            .type = SNAP_ENDPOINT
        }
    );

    sitehelper_editor_clear_snap(
        &editor
    );

    assert(
        !sitehelper_editor_has_snap(
            &editor
        )
    );
}

static void test_editor_updates_grid_snap_without_wall(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_update_snap(
        &editor,
        NULL,
        (Vec2){
            .x = 103.0,
            .y = 198.0
        }
    );

    const SnapResult *result =
        sitehelper_editor_get_snap_result(
            &editor
        );

    assert(result != NULL);
    assert(result->type == SNAP_GRID);

    assert(result->position.x == 100.0);
    assert(result->position.y == 200.0);
}

static void test_editor_updates_snap_from_wall_candidates(void)
{
    Timber stud = {
        .length = 2400,

        .position = {
            .x = 600,
            .y = 0
        },

        .type = TIMBER_STUD
    };

    Wall wall = {
        .framing.studs = &stud,
        .framing.stud_count = 1
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_update_snap(
        &editor,
        &wall,
        (Vec2){
            .x = 620.0,
            .y = 20.0
        }
    );

    const SnapResult *result =
        sitehelper_editor_get_snap_result(
            &editor
        );

    assert(result != NULL);

    assert(
        result->type
        == SNAP_ENDPOINT
    );

    assert(
        result->position.x
        == 600.0
    );

    assert(
        result->position.y
        == 0.0
    );
}

static void test_editor_snap_update_replaces_previous_result(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_snap_result(
        &editor,
        (SnapResult){
            .position = {
                .x = 999.0,
                .y = 999.0
            },
            .type = SNAP_ENDPOINT
        }
    );

    sitehelper_editor_update_snap(
        &editor,
        NULL,
        (Vec2){
            .x = 101.0,
            .y = 201.0
        }
    );

    const SnapResult *result =
        sitehelper_editor_get_snap_result(
            &editor
        );

    assert(result != NULL);

    assert(
        result->position.x != 999.0
        || result->position.y != 999.0
    );
}

static void test_editor_initialises_without_opening_placement(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        editor.opening_placement.valid == 0
    );
}

static void test_editor_initialises_opening_tool_inactive(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        editor.opening_tool.active == 0
    );
}

static void test_editor_activates_opening_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    assert(
        sitehelper_editor_set_active_tool(
            &editor,
            EDITOR_TOOL_OPENING
        )
    );

    assert(editor.opening_tool.active);
}

static void test_editor_switching_away_cancels_opening_tool(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement.valid = 1;

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_SELECT
    );

    assert(!editor.opening_tool.active);
    assert(!editor.opening_placement.valid);
}

static void test_opening_tool_pointer_move_updates_preview(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {600, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1200, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1800, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .framing.studs = studs,
        .framing.stud_count = 4
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        (Vec2){
            .x = 300.0,
            .y = 1000.0
        }
    );

    assert(
        editor.opening_tool.preview_position.x
        == 300.0
    );

    assert(
        editor.opening_tool.preview_position.y
        == 1000.0
    );
}

static void test_editor_pointer_move_updates_opening_placement(void)
{
    Timber studs[] = {
        {
            .position = {0, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {600, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1200, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        },
        {
            .position = {1800, 0},
            .width = 35,
            .length = 2400,
            .type = TIMBER_STUD
        }
    };

    Wall wall = {
        .framing.studs = studs,
        .framing.stud_count = 4
    };

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_tool.width = 1200;

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        (Vec2){
            .x = 300.0,
            .y = 1000.0
        }
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(
            &editor
        );

    assert(placement != NULL);
    assert(placement->valid == 1);

    assert(placement->left == 300.0);
    assert(placement->bottom == 900.0);

    assert(placement->width == 1200);
    assert(placement->height == 1200);

    assert(
        placement->start_bay_index == 0
    );

    assert(
        placement->end_bay_index == 2
    );
}

static void test_select_tool_pointer_move_has_no_opening_placement(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_pointer_move(
        &editor,
        NULL,
        (Vec2){
            .x = 100.0,
            .y = 200.0
        }
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(
            &editor
        );

    assert(placement != NULL);
    assert(!placement->valid);
}

static void test_editor_creates_opening_command(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement =
        (OpeningPlacement){
            .valid = 1,
            .left = 600.0,
            .bottom = 900.0,
            .width = 1200,
            .height = 1200
        };

    editor.opening_tool.type =
        OPENING_WINDOW;

    OpeningCommand command;

    assert(
        sitehelper_editor_create_opening_command(
            &editor,
            1,
            &command
        )
    );

    assert(command.opening.id == 1);
    assert(command.opening.type == OPENING_WINDOW);
    assert(command.opening.frame_position == 600);
    assert(command.opening.frame_bottom == 900);
    assert(command.opening.width == 1200);
    assert(command.opening.height == 1200);
}

static void test_editor_does_not_create_opening_command_in_select_mode(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    editor.opening_placement =
        (OpeningPlacement){
            .valid = 1,
            .left = 600.0,
            .bottom = 900.0,
            .width = 1200,
            .height = 1200
        };

    OpeningCommand command;

    assert(
        !sitehelper_editor_create_opening_command(
            &editor,
            1,
            &command
        )
    );
}

static void test_editor_rejects_invalid_opening_command_id(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement =
        (OpeningPlacement){
            .valid = 1,
            .left = 600.0,
            .bottom = 900.0,
            .width = 1200,
            .height = 1200
        };

    OpeningCommand command;

    assert(
        !sitehelper_editor_create_opening_command(
            &editor,
            DOMAIN_ID_INVALID,
            &command
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
    test_editor_initialises_without_snap();
    test_editor_exposes_default_snap_settings();
    test_editor_can_store_snap_result();
    test_editor_can_clear_snap();
    test_editor_updates_grid_snap_without_wall();
    test_editor_updates_snap_from_wall_candidates();
    test_editor_snap_update_replaces_previous_result();
    test_editor_initialises_without_opening_placement();
    test_editor_initialises_opening_tool_inactive();
    test_editor_activates_opening_tool();
    test_editor_switching_away_cancels_opening_tool();
    test_opening_tool_pointer_move_updates_preview();
    test_editor_pointer_move_updates_opening_placement();
    test_select_tool_pointer_move_has_no_opening_placement();
    test_editor_creates_opening_command();
    test_editor_does_not_create_opening_command_in_select_mode();
    test_editor_rejects_invalid_opening_command_id();

    printf(
        "All SiteHelper editor tests passed.\n"
    );

    return 0;
}