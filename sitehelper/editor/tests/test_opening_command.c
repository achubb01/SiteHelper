#include <assert.h>
#include <stdio.h>

#include "opening_command.h"

static void test_create_builds_opening_from_placement(void)
{
    OpeningPlacement placement = {
        .valid = 1,

        .start_bay_index = 0,
        .end_bay_index = 2,

        .left = 600.0,
        .bottom = 900.0,

        .width = 1200,
        .height = 1200
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    OpeningCommand command;

    int result =
        opening_command_create(
            &placement,
            &tool,
            &command
        );

    assert(result == 1);

    assert(
        command.opening.type
        == OPENING_WINDOW
    );

    assert(
        command.opening.frame_position
        == 600
    );

    assert(
        command.opening.frame_bottom
        == 900
    );

    assert(
        command.opening.width
        == 1200
    );

    assert(
        command.opening.height
        == 1200
    );

    assert(
        command.opening.custom_allowance
        == false
    );
}

static void test_create_rejects_invalid_placement(void)
{
    OpeningPlacement placement = {
        .valid = 0
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    OpeningCommand command;

    assert(
        !opening_command_create(
            &placement,
            &tool,
            &command
        )
    );
}

static void test_create_rejects_null_arguments(void)
{
    OpeningPlacement placement = {
        .valid = 1,
        .left = 600.0,
        .bottom = 900.0,
        .width = 1200,
        .height = 1200
    };

    OpeningTool tool;

    opening_tool_init(
        &tool
    );

    OpeningCommand command;

    assert(
        !opening_command_create(
            NULL,
            &tool,
            &command
        )
    );

    assert(
        !opening_command_create(
            &placement,
            NULL,
            &command
        )
    );

    assert(
        !opening_command_create(
            &placement,
            &tool,
            NULL
        )
    );
}

static void test_execute_adds_opening_to_wall(void)
{
    Wall wall = {0};

    BuildSettings settings = {
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .stud_spacing_mode = STUD_SPACING_MAXIMISE
    };

    assert(
        wall_set_length(
            &wall,
            4200
        )
    );

    assert(
        wall_generate(
            &wall,
            &settings
        )
    );

    OpeningPlacement placement = {
        .valid = 1,
        .left = 1200.0,
        .bottom = 900.0,
        .width = 1200,
        .height = 1200
    };

    OpeningTool tool;
    opening_tool_init(&tool);

    OpeningCommand command;

    assert(
        opening_command_create(
            &placement,
            &tool,
            &command
        )
    );

    size_t opening_count_before =
        wall.opening_count;

    assert(
        opening_command_execute(
            &wall,
            &settings,
            &command
        )
    );

    assert(
        wall.opening_count
        == opening_count_before + 1
    );

    assert(
        wall.openings[
            wall.opening_count - 1
        ].frame_position
        == 1200
    );

    assert(
        wall.openings[
            wall.opening_count - 1
        ].width
        == 1200
    );

    int found_king = 0;
    int found_trimmer = 0;

    for (
        size_t i = 0;
        i < wall.stud_count;
        i++
    ) {
        if (
            wall.studs[i].details.stud.type
            == STUD_KING
        ) {
            found_king = 1;
        }

        if (
            wall.studs[i].details.stud.type
            == STUD_TRIMMER
        ) {
            found_trimmer = 1;
        }
    }

    assert(found_king);
    assert(found_trimmer);

    wall_destroy(&wall);
}

int main(void)
{
    test_create_builds_opening_from_placement();
    test_create_rejects_invalid_placement();
    test_create_rejects_null_arguments();
    test_execute_adds_opening_to_wall();

    printf(
        "All opening command tests passed.\n"
    );

    return 0;
}