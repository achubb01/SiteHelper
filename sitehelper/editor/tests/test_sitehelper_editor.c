#include <assert.h>
#include <stdio.h>

#include "sitehelper_editor.h"
#include "command_history.h"

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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
            EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
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
    editor.current_storey_id = 1;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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
    editor.current_storey_id = 1;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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
    editor.current_storey_id = 1;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;

    assert(!editor.opening_placement.has_candidate);
}

static void test_editor_initialises_opening_tool_inactive(void)
{
    SiteHelperEditor editor;

    sitehelper_editor_init(
        &editor
    );
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
            EDITOR_SELECTION_SCOPE_WALL_ELEVATION,
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;

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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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
    editor.current_storey_id = 1;
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


static void test_select_pointer_hover_tracks_member_then_opening(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    assert(sitehelper_editor_set_active_tool(&editor, EDITOR_TOOL_SELECT));

    Timber studs[] = {{
        .length=1000,.depth=90,.width=35,.position={100,100},.type=TIMBER_STUD,
        .details.stud={.type=STUD_COMMON}
    }};
    Opening openings[] = {{
        .id=77,.type=OPENING_WINDOW,.frame_position=500,.frame_bottom=500,
        .width=800,.height=900,.custom_allowance=true
    }};
    Wall wall = {
        .id=20,
        .definition={
            .segment={{0,0},{3000,0}},.openings=openings,.opening_count=1
        },
        .framing={.studs=studs,.stud_count=1}
    };
    BuildSettings settings = opening_test_settings();

    sitehelper_editor_pointer_move(&editor,&wall,&settings,(Vec2){110,200});
    const WallSelection *member = sitehelper_editor_get_hovered_wall_member(&editor,20);
    assert(member != NULL && member->kind == WALL_MEMBER_STUD);
    assert(sitehelper_editor_get_hovered_opening(&editor,20) == DOMAIN_ID_INVALID);

    sitehelper_editor_pointer_move(&editor,&wall,&settings,(Vec2){700,700});
    assert(sitehelper_editor_get_hovered_wall_member(&editor,20) == NULL);
    assert(sitehelper_editor_get_hovered_opening(&editor,20) == 77);

    sitehelper_editor_pointer_leave(&editor);
    assert(sitehelper_editor_get_hovered_wall_member(&editor,20) == NULL);
    assert(sitehelper_editor_get_hovered_opening(&editor,20) == DOMAIN_ID_INVALID);
    sitehelper_editor_destroy(&editor);
}

static void test_wall_elevation_hover_is_select_only_and_owner_scoped(void)
{
    SiteHelperEditor editor;
    sitehelper_editor_init(&editor);
    editor.current_storey_id = 1;
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));

    Timber stud = make_test_stud();
    Wall wall={.id=20,.definition={.segment={{0,0},{3000,0}}},
        .framing={.studs=&stud,.stud_count=1}};
    BuildSettings settings=opening_test_settings();
    sitehelper_editor_pointer_move(&editor,&wall,&settings,(Vec2){110,200});
    assert(sitehelper_editor_get_hovered_wall_member(&editor,20) != NULL);
    assert(sitehelper_editor_get_hovered_wall_member(&editor,21) == NULL);

    assert(sitehelper_editor_set_active_tool(&editor,EDITOR_TOOL_OPENING));
    assert(sitehelper_editor_get_hovered_wall_member(&editor,20) == NULL);
    sitehelper_editor_pointer_move(&editor,&wall,&settings,(Vec2){110,200});
    assert(sitehelper_editor_get_hovered_wall_member(&editor,20) == NULL);
    sitehelper_editor_destroy(&editor);
}


static void test_framing_opening_direct_edit_is_preview_then_one_command(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    sitehelper_command_history_init(&history);
    DomainId storey_id=sitehelper_project_add_storey(&project,0);
    DomainId wall_id=sitehelper_project_add_wall(&project,storey_id,
        (WallPlanSegment){{0,0},{5000,0}});
    assert(storey_id != DOMAIN_ID_INVALID && wall_id != DOMAIN_ID_INVALID);
    Wall *wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&project,storey_id,&settings));
    DomainId opening_id=domain_id_generate(&project.domain_ids);
    Opening opening={.id=opening_id,.type=OPENING_WINDOW,.frame_position=1000,
        .frame_bottom=700,.width=800,.height=1000,.custom_allowance=true};
    assert(wall_add_opening_definition(wall,&settings,&opening));
    assert(wall_generate(wall,&settings));

    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    editor.current_wall_id=wall_id;
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_FRAMING));
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    editor_selection_set_opening(&editor.selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,wall_id,opening_id);

    sitehelper_editor_update_opening_edit_hover_in_project(&editor,&project,
        (Vec2){1800,1200},25.0);
    WallOpeningEdit edit;
    assert(sitehelper_editor_get_opening_edit(&editor,wall_id,opening_id,&edit));
    assert(edit.hovered_handle == WALL_OPENING_EDIT_HANDLE_RIGHT && !edit.active);
    assert(sitehelper_editor_begin_opening_edit_in_project(&editor,&project,
        (Vec2){1800,1200},25.0));
    sitehelper_editor_update_opening_edit_in_project(&editor,&project,
        (Vec2){2000,1200});
    assert(sitehelper_editor_get_opening_edit(&editor,wall_id,opening_id,&edit));
    assert(edit.active && edit.active_handle == WALL_OPENING_EDIT_HANDLE_RIGHT);
    assert(edit.candidate.width == 1000 && edit.validation.code == WALL_OPENING_VALID);
    assert(wall_find_opening_by_id_const(wall,opening_id)->width == 800);

    EditorAction action={0};
    assert(sitehelper_editor_create_opening_edit_action(&editor,&action));
    assert(action.kind == EDITOR_ACTION_COMMAND &&
        action.command.type == SITEHELPER_COMMAND_EDIT_OPENING);
    assert(action.command.data.edit_opening.definition.width == 1000);

    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history,&project,&action.command,&result));
    sitehelper_editor_complete_action(&editor,&action,&result);
    wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    assert(history.count == 1 && history.cursor == 1);
    assert(wall_find_opening_by_id_const(wall,opening_id)->width == 1000);
    assert(!editor.wall_opening_edit.active);
    assert(sitehelper_command_history_undo(&history,&project));
    wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    assert(wall_find_opening_by_id_const(wall,opening_id)->width == 800);
    assert(sitehelper_command_history_redo(&history,&project));
    wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    assert(wall_find_opening_by_id_const(wall,opening_id)->width == 1000);

    sitehelper_command_history_destroy(&history);
    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}

static void test_framing_opening_direct_edit_rejects_invalid_and_escape_cancels(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    DomainId storey_id=sitehelper_project_add_storey(&project,0);
    DomainId wall_id=sitehelper_project_add_wall(&project,storey_id,
        (WallPlanSegment){{0,0},{5000,0}});
    Wall *wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&project,storey_id,&settings));
    DomainId opening_id=domain_id_generate(&project.domain_ids);
    Opening opening={.id=opening_id,.type=OPENING_WINDOW,.frame_position=1000,
        .frame_bottom=700,.width=800,.height=1000,.custom_allowance=true};
    assert(wall_add_opening_definition(wall,&settings,&opening));
    assert(wall_generate(wall,&settings));
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    editor.current_wall_id=wall_id;
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_FRAMING));
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    editor_selection_set_opening(&editor.selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,wall_id,opening_id);

    assert(sitehelper_editor_begin_opening_edit_in_project(&editor,&project,
        (Vec2){1000,1200},25.0));
    sitehelper_editor_update_opening_edit_in_project(&editor,&project,
        (Vec2){-500,1200});
    assert(editor.wall_opening_edit.validation.code != WALL_OPENING_VALID);
    EditorAction action={0};
    assert(!sitehelper_editor_create_opening_edit_action(&editor,&action));
    assert(wall_find_opening_by_id_const(wall,opening_id)->frame_position == 1000);
    assert(sitehelper_editor_cancel_tool_interaction(&editor));
    assert(!editor.wall_opening_edit.active);
    assert(wall_find_opening_by_id_const(wall,opening_id)->frame_position == 1000);

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
}


static void test_framing_opening_direct_edit_grip_semantics(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    DomainId storey_id=sitehelper_project_add_storey(&project,0);
    DomainId wall_id=sitehelper_project_add_wall(&project,storey_id,
        (WallPlanSegment){{0,0},{5000,0}});
    Wall *wall=sitehelper_project_find_wall_by_id(&project,wall_id);
    BuildSettings settings;
    assert(sitehelper_project_resolve_storey_build_settings(&project,storey_id,&settings));
    DomainId opening_id=domain_id_generate(&project.domain_ids);
    Opening opening={.id=opening_id,.type=OPENING_WINDOW,.frame_position=1000,
        .frame_bottom=700,.width=800,.height=1000,.custom_allowance=true};
    assert(wall_add_opening_definition(wall,&settings,&opening));
    assert(wall_generate(wall,&settings));
    assert(sitehelper_editor_set_current_storey(&editor,&project,storey_id));
    editor.current_wall_id=wall_id;
    assert(sitehelper_editor_set_active_workspace(&editor,EDITOR_WORKSPACE_FRAMING));
    assert(sitehelper_editor_set_active_view(&editor,EDITOR_VIEW_WALL_ELEVATION));
    editor_selection_set_opening(&editor.selection,
        EDITOR_SELECTION_SCOPE_WALL_ELEVATION,wall_id,opening_id);

    struct Case { Vec2 grip, moved; WallOpeningEditHandle handle;
        int position,bottom,width,height; } cases[] = {
        {{1400,1200},{1500,1250},WALL_OPENING_EDIT_HANDLE_MOVE,1100,750,800,1000},
        {{1000,1200},{1100,1200},WALL_OPENING_EDIT_HANDLE_LEFT,1100,700,700,1000},
        {{1800,1200},{1900,1200},WALL_OPENING_EDIT_HANDLE_RIGHT,1000,700,900,1000},
        {{1400,700},{1400,750},WALL_OPENING_EDIT_HANDLE_BOTTOM,1000,750,800,950},
        {{1400,1700},{1400,1800},WALL_OPENING_EDIT_HANDLE_TOP,1000,700,800,1100}
    };
    for (size_t i=0;i<sizeof cases/sizeof cases[0];i++) {
        assert(sitehelper_editor_begin_opening_edit_in_project(&editor,&project,
            cases[i].grip,20.0));
        assert(editor.wall_opening_edit.active_handle == cases[i].handle);
        sitehelper_editor_update_opening_edit_in_project(&editor,&project,cases[i].moved);
        assert(editor.wall_opening_edit.validation.code == WALL_OPENING_VALID);
        const Opening *candidate=&editor.wall_opening_edit.candidate;
        assert(candidate->frame_position == cases[i].position);
        assert(candidate->frame_bottom == cases[i].bottom);
        assert(candidate->width == cases[i].width);
        assert(candidate->height == cases[i].height);
        sitehelper_editor_cancel_opening_edit(&editor);
    }
    assert(wall_find_opening_by_id_const(wall,opening_id)->frame_position == 1000);
    assert(wall_find_opening_by_id_const(wall,opening_id)->width == 800);

    sitehelper_editor_destroy(&editor);
    sitehelper_project_destroy(&project);
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
    test_select_pointer_hover_tracks_member_then_opening();
    test_wall_elevation_hover_is_select_only_and_owner_scoped();
    test_framing_opening_direct_edit_is_preview_then_one_command();
    test_framing_opening_direct_edit_grip_semantics();
    test_framing_opening_direct_edit_rejects_invalid_and_escape_cancels();
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
