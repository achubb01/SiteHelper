#include <stdlib.h>

#include <assert.h>
#include <stdio.h>

#include "opening_command.h"

static const DomainId TEST_ROOM_ID = 10;
static const DomainId TEST_WALL_ID = 20;

static Wall *add_test_wall(
    SiteHelperProject *project,
    int length,
    DomainId *room_id_out,
    DomainId *wall_id_out
)
{
    DomainId room_id =
        sitehelper_project_add_room(
            project
        );

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(
            project,
            room_id
        );

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project->structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall =
        room_find_wall_by_id(
            room,
            wall_id
        );

    assert(wall != NULL);

    assert(
        wall_set_length(
            wall,
            length
        )
    );

    assert(
        wall_generate(
            wall,
            &project->settings
        )
    );

    if (room_id_out != NULL) {
        *room_id_out = room_id;
    }

    if (wall_id_out != NULL) {
        *wall_id_out = wall_id;
    }

    return wall;
}

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

    OpeningCommand command;

    assert(
        opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            600,
            900,
            1200,
            1200,
            &command
        )
    );

    assert(command.room_id == TEST_ROOM_ID);
    assert(command.wall_id == TEST_WALL_ID);
    assert(command.type == OPENING_WINDOW);
    assert(command.frame_position == 600);
    assert(command.frame_bottom == 900);
    assert(command.width == 1200);
    assert(command.height == 1200);
}

static void test_create_rejects_invalid_dimensions(void)
{
    OpeningCommand command;

    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            600,
            900,
            0,
            1200,
            &command
        )
    );

    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            600,
            900,
            1200,
            0,
            &command
        )
    );
}

static void test_create_rejects_invalid_position(void)
{
    OpeningCommand command;

    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            -1,
            900,
            1200,
            1200,
            &command
        )
    );

    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            600,
            -1,
            1200,
            1200,
            &command
        )
    );
}

static void test_create_rejects_null_command(void)
{
    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            NULL
        )
    );
}

static void test_create_rejects_invalid_type(void)
{
    OpeningCommand command;

    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            TEST_WALL_ID,
            (OpeningType)999,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );
}

static void test_execute_adds_opening_to_wall(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    OpeningPlacement placement = {
        .valid = 1,
        .left = 1200.0,
        .bottom = 900.0,
        .width = 1200,
        .height = 1200
    };

    OpeningCommand command;

    DomainIdGenerator expected_ids =
        project.domain_ids;

    DomainId expected_opening_id =
        domain_id_generate(
            &expected_ids
        );

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    size_t opening_count_before =
        wall->definition.opening_count;

    DomainId created_opening_id =
        DOMAIN_ID_INVALID;

    assert(
        opening_command_execute(
            &project,
            &command,
            &created_opening_id
        )
    );

    assert(
        created_opening_id
        == expected_opening_id
    );

    assert(
        project.domain_ids.next
        == expected_ids.next
    );

    const Opening *opening =
        wall_find_opening_by_id_const(
            wall,
            expected_opening_id
        );

    assert(opening != NULL);

    assert(
        opening->frame_position
        == 1200
    );

    assert(
        opening->width
        == 1200
    );

    assert(
        wall->definition.opening_count
        == opening_count_before + 1
    );

    assert(
        wall->definition.openings[
            wall->definition.opening_count - 1
        ].frame_position
        == 1200
    );

    assert(
        wall->definition.openings[
            wall->definition.opening_count - 1
        ].width
        == 1200
    );

    int found_king = 0;
    int found_trimmer = 0;

    for (
        size_t i = 0;
        i < wall->framing.stud_count;
        i++
    ) {
        if (
            wall->framing.studs[i].details.stud.type
            == STUD_KING
        ) {
            found_king = 1;
        }

        if (
            wall->framing.studs[i].details.stud.type
            == STUD_TRIMMER
        ) {
            found_trimmer = 1;
        }
    }

    assert(found_king);
    assert(found_trimmer);

    sitehelper_project_destroy(
        &project
    );
}

static void test_execute_failure_preserves_wall_state(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    size_t opening_count_before =
        wall->definition.opening_count;

    size_t stud_count_before =
        wall->framing.stud_count;

    size_t nog_count_before =
        wall->framing.nog_count;

    OpeningPlacement placement = {
        .valid = 1,
        .left = 1200.0,
        .bottom = 900.0,
        .width = 1200,
        .height = 1200
    };

    OpeningCommand command;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    /*
     * Mutation can succeed, but generation
     * will fail.
     */
    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    DomainId created_opening_id = 999;

    assert(
        !opening_command_execute(
            &project,
            &command,
            &created_opening_id
        )
    );

    assert(
        created_opening_id
        == DOMAIN_ID_INVALID
    );

    assert(
        wall->definition.opening_count
        == opening_count_before
    );

    assert(
        wall->framing.stud_count
        == stud_count_before
    );

    assert(
        wall->framing.nog_count
        == nog_count_before
    );

    sitehelper_project_destroy(
        &project
    );
}

static void test_execute_preserves_existing_openings(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            6000,
            &room_id,
            &wall_id
        );

    DomainId existing_opening_id =
        domain_id_generate(
            &project.domain_ids
        );

    DomainIdGenerator expected_ids =
        project.domain_ids;

    DomainId new_opening_id =
        domain_id_generate(
            &expected_ids
        );

    assert(
        wall_add_opening(
            wall,
            &project.settings,
            existing_opening_id,
            OPENING_WINDOW,
            600,
            900,
            900,
            1200
        )
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    const Opening *existing_before =
        wall_find_opening_by_id_const(
            wall,
            existing_opening_id
        );

    assert(existing_before != NULL);

    assert(
        existing_before->frame_position
        == 600
    );

    assert(
        existing_before->width
        == 900
    );

    OpeningCommand command;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            3000,   /* frame_position */
            900,    /* frame_bottom */
            1200,   /* width */
            1200,   /* height */
            &command
        )
    );

    DomainId created_opening_id =
        DOMAIN_ID_INVALID;

    assert(
        opening_command_execute(
            &project,
            &command,
            &created_opening_id
        )
    );

    assert(
        created_opening_id
        == new_opening_id
    );

    assert(
        wall->definition.opening_count
        == 2
    );

    const Opening *existing_after =
        wall_find_opening_by_id_const(
            wall,
            existing_opening_id
        );

    const Opening *new_opening =
        wall_find_opening_by_id_const(
            wall,
            new_opening_id
        );

    assert(existing_after != NULL);
    assert(new_opening != NULL);

    assert(
        existing_after->id
        == existing_opening_id
    );

    assert(
        existing_after->type
        == OPENING_WINDOW
    );

    assert(
        existing_after->frame_position
        == 600
    );

    assert(
        existing_after->frame_bottom
        == 900
    );

    assert(
        existing_after->width
        == 900
    );

    assert(
        existing_after->height
        == 1200
    );

    assert(
        new_opening->frame_position
        == 3000
    );

    assert(
        new_opening->width
        == 1200
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_create_rejects_invalid_target_ids(void)
{
    OpeningCommand command;

    assert(
        !opening_command_create(
            DOMAIN_ID_INVALID,
            TEST_WALL_ID,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        ));

    assert(
        !opening_command_create(
            TEST_ROOM_ID,
            DOMAIN_ID_INVALID,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        ));
}

static void relocate_room_wall_storage(
    Room *room
)
{
    assert(room != NULL);
    assert(room->walls != NULL);
    assert(room->wall_capacity > 0);

    Wall *old_walls =
        room->walls;

    Wall *new_walls =
        malloc(
            room->wall_capacity
            * sizeof *new_walls
        );

    assert(new_walls != NULL);

    for (
        size_t i = 0;
        i < room->wall_count;
        i++
    ) {
        new_walls[i] =
            old_walls[i];
    }

    /*
     * Do not destroy individual Walls here.
     *
     * Their owned pointers have been moved
     * into the copied Wall structs.
     */
    free(
        old_walls
    );

    room->walls =
        new_walls;
}

static void
test_execute_resolves_wall_after_storage_relocation(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *original_wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    OpeningCommand command;

    DomainIdGenerator expected_ids =
        project.domain_ids;

    DomainId expected_opening_id =
        domain_id_generate(
            &expected_ids
        );

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    Room *room =
        build_find_room_by_id(
            &project.structure,
            room_id
        );

    assert(room != NULL);

    Wall *old_address =
        original_wall;

    relocate_room_wall_storage(
        room
    );

    Wall *relocated_wall =
        room_find_wall_by_id(
            room,
            wall_id
        );

    assert(relocated_wall != NULL);

    assert(
        relocated_wall
        != old_address
    );

    /*
     * The command was created before the
     * Wall moved, but contains only IDs.
     *
     * Execution must resolve the current
     * Wall from the project.
     */
    DomainId created_opening_id =
        DOMAIN_ID_INVALID;

    assert(
        opening_command_execute(
            &project,
            &command,
            &created_opening_id
        )
    );

    assert(
        created_opening_id
        == expected_opening_id
    );

    assert(
        project.domain_ids.next
        == expected_ids.next
    );

    /*
    * Resolve again after execution.
    *
    * The command itself must not depend on
    * any Wall pointer captured previously.
    */
    relocated_wall =
        room_find_wall_by_id(
            room,
            wall_id
        );

    assert(relocated_wall != NULL);

    assert(
        relocated_wall->definition.opening_count
        == 1
    );

    const Opening *opening =
        wall_find_opening_by_id_const(
            relocated_wall,
            expected_opening_id
        );

    assert(opening != NULL);

    assert(
        opening->frame_position
        == 1200
    );

    assert(
        opening->width
        == 1200
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_failed_execute_does_not_consume_domain_id(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    assert(wall != NULL);

    OpeningCommand command;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    DomainId next_before =
        project.domain_ids.next;

    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    DomainId created_opening_id = 999;

    assert(
        !opening_command_execute(
            &project,
            &command,
            &created_opening_id
        )
    );

    assert(
        created_opening_id
        == DOMAIN_ID_INVALID
    );

    /*
     * Failed command:
     * no identity allocation committed.
     */
    assert(
        project.domain_ids.next
        == next_before
    );

    /*
     * Failed command:
     * no authoritative opening committed.
     */
    assert(
        wall->definition.opening_count
        == 0
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_execute_rejects_null_opening_id_output(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    assert(wall != NULL);

    OpeningCommand command;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    assert(
        !opening_command_execute(
            &project,
            &command,
            NULL
        )
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_undo_removes_created_opening(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    assert(wall != NULL);

    OpeningCommand command;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    DomainId opening_id =
        DOMAIN_ID_INVALID;

    assert(
        opening_command_execute(
            &project,
            &command,
            &opening_id
        )
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) != NULL
    );

    DomainId next_before_undo =
        project.domain_ids.next;

    assert(
        opening_command_undo(
            &project,
            &command,
            opening_id
        )
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) == NULL
    );

    assert(
        wall->definition.opening_count
        == 0
    );

    /*
     * Undo must not recycle identity.
     */
    assert(
        project.domain_ids.next
        == next_before_undo
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_undo_failure_preserves_wall_state(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id;
    DomainId wall_id;

    Wall *wall =
        add_test_wall(
            &project,
            4200,
            &room_id,
            &wall_id
        );

    assert(wall != NULL);

    OpeningCommand command;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &command
        )
    );

    DomainId opening_id =
        DOMAIN_ID_INVALID;

    assert(
        opening_command_execute(
            &project,
            &command,
            &opening_id
        )
    );

    size_t opening_count_before =
        wall->definition.opening_count;

    Timber *studs_before =
        wall->framing.studs;

    size_t stud_count_before =
        wall->framing.stud_count;

    DomainId next_before =
        project.domain_ids.next;

    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    assert(
        !opening_command_undo(
            &project,
            &command,
            opening_id
        )
    );

    assert(
        wall->definition.opening_count
        == opening_count_before
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) != NULL
    );

    assert(
        wall->framing.studs
        == studs_before
    );

    assert(
        wall->framing.stud_count
        == stud_count_before
    );

    assert(
        project.domain_ids.next
        == next_before
    );

    sitehelper_project_destroy(
        &project
    );
}

int main(void)
{
    test_create_builds_opening_from_placement();
    test_create_rejects_invalid_dimensions();
    test_create_rejects_invalid_position();
    test_create_rejects_null_command();
    test_create_rejects_invalid_type();
    test_execute_adds_opening_to_wall();
    test_execute_failure_preserves_wall_state();
    test_execute_preserves_existing_openings();
    test_create_rejects_invalid_target_ids();
    test_execute_resolves_wall_after_storage_relocation();
    test_failed_execute_does_not_consume_domain_id();
    test_execute_rejects_null_opening_id_output();
    test_undo_removes_created_opening();
    test_undo_failure_preserves_wall_state();

    printf(
        "All opening command tests passed.\n"
    );

    return 0;
}