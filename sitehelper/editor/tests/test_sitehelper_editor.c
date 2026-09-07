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
            .u = 100,
            .z = 0
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = STUD_COMMON
        }
    };

    return stud;
}

static BuildSettings opening_test_settings(void)
{
    return (BuildSettings){
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .opening_width_allowance = 0,
        .opening_height_allowance = 0
    };
}

static OpeningPlacement valid_opening_placement(void)
{
    return (OpeningPlacement){
        .has_candidate = 1,
        .left = 600.0,
        .bottom = 900.0,
        .width = 1200,
        .height = 1200,
        .validation = {
            .code = WALL_OPENING_VALID
        }
    };
}

static SiteHelperEditor opening_test_editor(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(&editor);
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    editor.current_room_id = 10;
    editor.current_wall_id = 20;

    assert(sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    ));

    return editor;
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
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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
        (WallLocalPosition){
            .u = 120,
            .z = 1000
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
        (WallLocalPosition){
            .u = 120,
            .z = 1000
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
        (WallLocalPosition){
            .u = 500,
            .z = 1000
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
        (WallLocalPosition){
            .u = 120,
            .z = 1000
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
        (WallLocalPosition){
            .u = 120,
            .z = 1000
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
        (WallLocalPosition){
            .u = 0,
            .z = 0
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
            .u = 600,
            .z = 0
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
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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

    assert(!editor.opening_placement.has_candidate);
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
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_SELECT
    );

    assert(!editor.opening_tool.active);
    assert(!editor.opening_placement.has_candidate);
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
        .definition.segment.end.x = 4000,
        .framing.studs = studs,
        .framing.stud_count = 4
    };
    BuildSettings settings = opening_test_settings();

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
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
        .definition.segment.end.x = 4000,
        .framing.studs = studs,
        .framing.stud_count = 4
    };
    BuildSettings settings = opening_test_settings();

    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_tool.width = 1200;

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
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
    assert(placement->has_candidate);
    assert(placement->validation.code == WALL_OPENING_VALID);
    assert(opening_placement_is_valid(placement));

    assert(placement->left == 300.0);
    assert(placement->bottom == 900.0);

    assert(placement->width == 1200);
    assert(placement->height == 1200);

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
    assert(!placement->has_candidate);
    assert(!opening_placement_is_valid(placement));
}

static void test_editor_creates_opening_command(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    editor.current_room_id = 10;
    editor.current_wall_id = 20;

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    editor.opening_tool.type =
        OPENING_WINDOW;

    OpeningCommand command;

    assert(
        sitehelper_editor_create_opening_command(
            &editor,
            &command
        )
    );

    assert(command.room_id == 10);
    assert(command.wall_id == 20);

    assert(command.type == OPENING_WINDOW);
    assert(command.frame_position == 600);
    assert(command.frame_bottom == 900);
    assert(command.width == 1200);
    assert(command.height == 1200);
}

static void test_editor_does_not_create_opening_command_in_select_mode(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    editor.current_room_id = 10;
    editor.current_wall_id = 20;

    editor.opening_placement = valid_opening_placement();

    OpeningCommand command;

    assert(
        !sitehelper_editor_create_opening_command(
            &editor,
            &command
        )
    );
}

static void test_editor_completing_opening_command_clears_placement(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    sitehelper_editor_complete_opening_command(
        &editor
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(
            &editor
        );

    assert(placement != NULL);
    assert(!placement->has_candidate);
}

static void test_editor_complete_opening_command_accepts_null(void)
{
    sitehelper_editor_complete_opening_command(
        NULL
    );
}

static void test_select_primary_action_selects_wall_member(void)
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
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    EditorAction action;

    assert(
        sitehelper_editor_primary_action(
            &editor,
            &wall,
            (Vec2){
                .x = 120.0,
                .y = 1000.0
            },
            &action
        )
    );

    assert(
        action.kind
        == EDITOR_ACTION_NONE
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

    assert(wall_selection != NULL);

    assert(
        wall_selection_resolve(
            wall_selection,
            &wall
        ) == &stud
    );
}

static void test_select_primary_action_with_null_wall_preserves_selection(void)
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
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    EditorAction action;

    assert(
        sitehelper_editor_primary_action(
            &editor,
            &wall,
            (Vec2){
                .x = 120.0,
                .y = 1000.0
            },
            &action
        )
    );

    assert(
        action.kind
        == EDITOR_ACTION_NONE
    );

    assert(
        !editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );

    assert(
        sitehelper_editor_primary_action(
            &editor,
            NULL,
            (Vec2){
                .x = 0.0,
                .y = 0.0
            },
            &action
        )
    );

    assert(
        action.kind
        == EDITOR_ACTION_NONE
    );

    assert(
        !editor_selection_is_empty(
            sitehelper_editor_get_selection(
                &editor
            )
        )
    );
}

static void test_opening_primary_action_produces_command(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    editor.current_room_id = 10;
    editor.current_wall_id = 20;

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    editor.opening_tool.type =
        OPENING_WINDOW;

    EditorAction action;

    assert(
        sitehelper_editor_primary_action(
            &editor,
            NULL,
            (Vec2){0.0, 0.0},
            &action
        )
    );

    assert(
        action.kind
        == EDITOR_ACTION_COMMAND
    );

    assert(
        action.command.type
        == SITEHELPER_COMMAND_ADD_OPENING
    );

    const OpeningCommand *opening =
        &action.command.data.opening;

    assert(opening->room_id == 10);
    assert(opening->wall_id == 20);

    assert(
        opening->type
        == OPENING_WINDOW
    );

    assert(
        opening->frame_position
        == 600
    );

    assert(
        opening->frame_bottom
        == 900
    );

    assert(
        opening->width
        == 1200
    );

    assert(
        opening->height
        == 1200
    );
}

static void test_editor_has_opening_preview_when_opening_placement_valid(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    assert(
        sitehelper_editor_has_opening_preview(
            &editor
        )
    );
}

static void test_editor_has_no_opening_preview_in_select_mode(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );

    editor.opening_placement = valid_opening_placement();

    assert(
        !sitehelper_editor_has_opening_preview(
            &editor
        )
    );
}

static void test_editor_returns_opening_preview_rect(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    Rect2 rect;

    assert(
        sitehelper_editor_get_opening_preview_rect(
            &editor,
            &rect
        )
    );

    assert(rect.position.x == 600.0);
    assert(rect.position.y == 900.0);
    assert(rect.width == 1200.0);
    assert(rect.height == 1200.0);
}

static void test_editor_does_not_return_opening_preview_rect_when_inactive(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    Rect2 rect;

    assert(
        !sitehelper_editor_get_opening_preview_rect(
            &editor,
            &rect
        )
    );
}

static void
test_editor_rejects_opening_command_without_target(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_active_tool(
        &editor,
        EDITOR_TOOL_OPENING
    );

    editor.opening_placement = valid_opening_placement();

    OpeningCommand command;

    assert(
        !sitehelper_editor_create_opening_command(
            &editor,
            &command
        )
    );
}

static void
test_editor_complete_action_clears_opening_placement(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    editor.opening_placement = valid_opening_placement();

    EditorAction action = {
        .kind =
            EDITOR_ACTION_COMMAND,

        .command = {
            .type =
                SITEHELPER_COMMAND_ADD_OPENING
        }
    };
    SiteHelperCommandResult result = {
        .type = SITEHELPER_COMMAND_ADD_OPENING
    };

    sitehelper_editor_complete_action(
        &editor,
        &action,
        &result
    );

    assert(
        !editor.opening_placement.has_candidate
    );
}

static void
test_editor_complete_action_accepts_null(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_complete_action(
        NULL,
        NULL,
        NULL
    );

    sitehelper_editor_complete_action(
        &editor,
        NULL,
        NULL
    );
}

static void
test_editor_invalidate_transient_state_clears_snap_and_opening_placement(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    sitehelper_editor_set_snap_result(
        &editor,
        (SnapResult){
            .position = {
                .x = 1200.0,
                .y = 900.0
            },
            .type = SNAP_ENDPOINT
        }
    );

    editor.opening_placement = valid_opening_placement();

    assert(
        sitehelper_editor_has_snap(
            &editor
        )
    );

    assert(
        editor.opening_placement.has_candidate
    );

    sitehelper_editor_invalidate_transient_state(
        &editor
    );

    assert(
        !sitehelper_editor_has_snap(
            &editor
        )
    );

    assert(
        !editor.opening_placement.has_candidate
    );
}

static void test_editor_keeps_validated_candidate_geometry(void)
{
    Wall wall = {
        .definition.segment.end.x = 4200
    };
    BuildSettings settings = opening_test_settings();
    SiteHelperEditor editor = opening_test_editor();

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = 600.0, .y = 1000.0}
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    OpeningCommand command;

    assert(placement->has_candidate);
    assert(placement->validation.code == WALL_OPENING_VALID);
    assert(opening_placement_is_valid(placement));
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(sitehelper_editor_create_opening_command(&editor, &command));
}

static void test_editor_keeps_left_end_rejected_candidate(void)
{
    Wall wall = {
        .definition.segment.end.x = 4200
    };
    BuildSettings settings = opening_test_settings();
    SiteHelperEditor editor = opening_test_editor();

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = 100.0, .y = 1000.0}
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    OpeningCommand command;

    assert(placement->has_candidate);
    assert(placement->validation.code ==
        WALL_OPENING_TOO_CLOSE_TO_LEFT_END);
    assert(!opening_placement_is_valid(placement));
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_create_opening_command(&editor, &command));
}

static void test_editor_keeps_right_end_rejected_candidate(void)
{
    Wall wall = {
        .definition.segment.end.x = 4200
    };
    BuildSettings settings = opening_test_settings();
    SiteHelperEditor editor = opening_test_editor();

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = 3000.0, .y = 1000.0}
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    OpeningCommand command;

    assert(placement->has_candidate);
    assert(placement->validation.code ==
        WALL_OPENING_TOO_CLOSE_TO_RIGHT_END);
    assert(!opening_placement_is_valid(placement));
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_create_opening_command(&editor, &command));
}

static void test_editor_keeps_height_rejected_candidate(void)
{
    Wall wall = {
        .definition.segment.end.x = 4200
    };
    BuildSettings settings = opening_test_settings();
    SiteHelperEditor editor = opening_test_editor();
    editor.opening_tool.bottom = 1300;

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = 600.0, .y = 1000.0}
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    OpeningCommand command;

    assert(placement->has_candidate);
    assert(placement->validation.code == WALL_OPENING_INVALID_HEIGHT);
    assert(!opening_placement_is_valid(placement));
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_create_opening_command(&editor, &command));
}

static void test_editor_keeps_overlapping_candidate(void)
{
    Opening openings[] = {
        {
            .id = 77,
            .type = OPENING_WINDOW,
            .frame_position = 1200,
            .frame_bottom = 900,
            .width = 1200,
            .height = 1200
        }
    };
    Wall wall = {
        .definition = {
            .segment.end.x = 4200,
            .openings = openings,
            .opening_count = 1
        }
    };
    BuildSettings settings = opening_test_settings();
    SiteHelperEditor editor = opening_test_editor();

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = 1400.0, .y = 1000.0}
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);
    OpeningCommand command;

    assert(placement->has_candidate);
    assert(placement->validation.code == WALL_OPENING_OVERLAPS_OPENING);
    assert(placement->validation.conflicting_opening_id == 77);
    assert(!opening_placement_is_valid(placement));
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_create_opening_command(&editor, &command));
}

static void test_editor_applies_opening_width_allowance_during_validation(void)
{
    BuildSettings settings = opening_test_settings();
    const int wall_length = 4305;
    const int nominal_width = 1200;
    const int nominal_left =
        wall_length -
        (3 * settings.stud_width) -
        nominal_width;
    Wall wall = {
        .definition.segment.end.x = wall_length
    };
    SiteHelperEditor editor = opening_test_editor();

    editor.opening_tool.width = nominal_width;

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = (double)nominal_left, .y = 1000.0}
    );

    const OpeningPlacement *placement =
        sitehelper_editor_get_opening_placement(&editor);

    assert(placement->has_candidate);
    assert(placement->left == (double)nominal_left);
    assert(placement->bottom == 900.0);
    assert(placement->width == nominal_width);
    assert(placement->height == 1200);
    assert(placement->validation.code == WALL_OPENING_VALID);

    settings.opening_width_allowance = 10;

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){.x = (double)nominal_left, .y = 1000.0}
    );

    placement = sitehelper_editor_get_opening_placement(&editor);
    OpeningCommand command;

    assert(placement->has_candidate);
    assert(placement->left == (double)nominal_left);
    assert(placement->bottom == 900.0);
    assert(placement->width == nominal_width);
    assert(placement->height == 1200);
    assert(placement->validation.code ==
        WALL_OPENING_TOO_CLOSE_TO_RIGHT_END);
    assert(sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_create_opening_command(&editor, &command));
}

static void test_opening_tool_does_not_mutate_wall_before_command_execution(void)
{
    Wall wall = {
        .id = 20,
        .definition = {
            .segment = {
                .start = { .x = 5000, .y = 3000 },
                .end = { .x = 9200, .y = 3000 }
            }
        }
    };
    BuildSettings settings = opening_test_settings();
    SiteHelperEditor editor = opening_test_editor();
    EditorAction action;

    sitehelper_editor_pointer_move(
        &editor,
        &wall,
        &settings,
        (Vec2){ .x = 600.0, .y = 1000.0 }
    );

    assert(sitehelper_editor_primary_action(
        &editor,
        &wall,
        (Vec2){ .x = 600.0, .y = 1000.0 },
        &action
    ));

    assert(action.kind == EDITOR_ACTION_COMMAND);
    assert(action.command.type == SITEHELPER_COMMAND_ADD_OPENING);

    /* Previewing and producing a command are editor-only operations. */
    assert(wall.id == 20);
    assert(wall.definition.segment.start.x == 5000);
    assert(wall.definition.segment.start.y == 3000);
    assert(wall_length_mm(&wall) == 4200);
    assert(wall.definition.openings == NULL);
    assert(wall.definition.opening_count == 0);
    assert(wall.framing.studs == NULL);
    assert(wall.framing.stud_count == 0);
}

static void test_editor_rejects_command_without_candidate(void)
{
    SiteHelperEditor editor = opening_test_editor();
    OpeningCommand command;

    assert(!editor.opening_placement.has_candidate);
    assert(!opening_placement_is_valid(&editor.opening_placement));
    assert(!sitehelper_editor_has_opening_preview(&editor));
    assert(!sitehelper_editor_create_opening_command(&editor, &command));
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
    test_editor_completing_opening_command_clears_placement();
    test_editor_complete_opening_command_accepts_null();
    test_select_primary_action_selects_wall_member();
    test_select_primary_action_with_null_wall_preserves_selection();
    test_opening_primary_action_produces_command();
    test_editor_has_opening_preview_when_opening_placement_valid();
    test_editor_has_no_opening_preview_in_select_mode();
    test_editor_returns_opening_preview_rect();
    test_editor_does_not_return_opening_preview_rect_when_inactive();
    test_editor_rejects_opening_command_without_target();
    test_editor_complete_action_clears_opening_placement();
    test_editor_complete_action_accepts_null();
    test_editor_invalidate_transient_state_clears_snap_and_opening_placement();
    test_editor_keeps_validated_candidate_geometry();
    test_editor_keeps_left_end_rejected_candidate();
    test_editor_keeps_right_end_rejected_candidate();
    test_editor_keeps_height_rejected_candidate();
    test_editor_keeps_overlapping_candidate();
    test_editor_applies_opening_width_allowance_during_validation();
    test_opening_tool_does_not_mutate_wall_before_command_execution();
    test_editor_rejects_command_without_candidate();

    printf(
        "All SiteHelper editor tests passed.\n"
    );

    return 0;
}
