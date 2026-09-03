#include <assert.h>
#include <stdio.h>

#include "wall.h"

static BuildSettings test_settings(void)
{
    return (BuildSettings){
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,
        .stud_spacing = 600,
        .nog_spacing = 1200,
        .opening_width_allowance = 10,
        .opening_height_allowance = 10
    };
}

static WallOpeningProposal valid_proposal(void)
{
    return (WallOpeningProposal){
        .type = OPENING_WINDOW,
        .frame_position = 1600,
        .frame_bottom = 800,
        .width = 800,
        .height = 1000
    };
}

static void test_accepts_a_valid_opening(void)
{
    Wall wall = {
        .definition.length = 4000
    };
    BuildSettings settings = test_settings();

    WallOpeningValidation validation = wall_validate_opening(
        &wall,
        &settings,
        &(WallOpeningProposal){
            .type = OPENING_WINDOW,
            .frame_position = 1600,
            .frame_bottom = 800,
            .width = 800,
            .height = 1000
        }
    );

    assert(validation.code == WALL_OPENING_VALID);
    assert(validation.conflicting_opening_id == DOMAIN_ID_INVALID);
}

static void test_rejects_invalid_arguments_and_proposals(void)
{
    Wall wall = {
        .definition.length = 4000
    };
    BuildSettings settings = test_settings();
    WallOpeningProposal proposal = valid_proposal();

    assert(wall_validate_opening(NULL, &settings, &proposal).code ==
        WALL_OPENING_INVALID_ARGUMENT);
    assert(wall_validate_opening(&wall, NULL, &proposal).code ==
        WALL_OPENING_INVALID_ARGUMENT);
    assert(wall_validate_opening(&wall, &settings, NULL).code ==
        WALL_OPENING_INVALID_ARGUMENT);

    proposal.type = (OpeningType)99;
    assert(wall_validate_opening(&wall, &settings, &proposal).code ==
        WALL_OPENING_INVALID_TYPE);

    proposal = valid_proposal();
    proposal.width = 0;
    assert(wall_validate_opening(&wall, &settings, &proposal).code ==
        WALL_OPENING_INVALID_DIMENSIONS);
}

static void test_rejects_opening_that_exceeds_wall_height(void)
{
    Wall wall = {
        .definition.length = 4000
    };
    BuildSettings settings = test_settings();
    WallOpeningProposal proposal = valid_proposal();

    proposal.frame_bottom = 1400;

    assert(wall_validate_opening(&wall, &settings, &proposal).code ==
        WALL_OPENING_INVALID_HEIGHT);
}

static void test_rejects_openings_too_close_to_wall_ends(void)
{
    Wall wall = {
        .definition.length = 4000
    };
    BuildSettings settings = test_settings();
    WallOpeningProposal proposal = valid_proposal();

    proposal.frame_position = 100;
    assert(wall_validate_opening(&wall, &settings, &proposal).code ==
        WALL_OPENING_TOO_CLOSE_TO_LEFT_END);

    proposal.frame_position = 3100;
    assert(wall_validate_opening(&wall, &settings, &proposal).code ==
        WALL_OPENING_TOO_CLOSE_TO_RIGHT_END);
}

static void test_reports_conflicting_opening_identity(void)
{
    Opening openings[] = {
        {
            .id = 42,
            .type = OPENING_WINDOW,
            .frame_position = 500,
            .frame_bottom = 800,
            .width = 800,
            .height = 1000
        }
    };
    Wall wall = {
        .definition = {
            .length = 4000,
            .openings = openings,
            .opening_count = 1
        }
    };
    BuildSettings settings = test_settings();
    WallOpeningProposal proposal = valid_proposal();

    proposal.frame_position = 1250;

    WallOpeningValidation validation = wall_validate_opening(
        &wall,
        &settings,
        &proposal
    );

    assert(validation.code == WALL_OPENING_OVERLAPS_OPENING);
    assert(validation.conflicting_opening_id == 42);
}

static void test_add_opening_definition_preserves_custom_allowances(void)
{
    Wall wall = {
        .definition.length = 4000
    };
    BuildSettings settings = test_settings();
    Opening opening = {
        .id = 42,
        .type = OPENING_WINDOW,
        .frame_position = 1600,
        .frame_bottom = 800,
        .width = 800,
        .height = 1000,
        .width_allowance = 25,
        .height_allowance = 15,
        .custom_allowance = true
    };

    assert(wall_add_opening_definition(
        &wall,
        &settings,
        &opening
    ));

    assert(wall.definition.opening_count == 1);
    assert(wall.definition.openings[0].id == 42);
    assert(wall.definition.openings[0].custom_allowance);
    assert(wall.definition.openings[0].width_allowance == 25);
    assert(wall.definition.openings[0].height_allowance == 15);

    wall_destroy(&wall);
}

int main(void)
{
    test_accepts_a_valid_opening();
    test_rejects_invalid_arguments_and_proposals();
    test_rejects_opening_that_exceeds_wall_height();
    test_rejects_openings_too_close_to_wall_ends();
    test_reports_conflicting_opening_identity();
    test_add_opening_definition_preserves_custom_allowances();

    puts("wall opening validation tests passed");
    return 0;
}
