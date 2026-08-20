#include <assert.h>
#include <stdio.h>

#include "sitehelper_command.h"

static void
test_execute_dispatches_opening_command(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id =
        sitehelper_project_add_room(
            &project
        );

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(
            &project,
            room_id
        );

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.structure,
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
            4200
        )
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    OpeningCommand opening;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &opening
        )
    );

    SiteHelperCommand command;

    assert(
        sitehelper_command_from_opening(
            &opening,
            &command
        )
    );

    DomainIdGenerator expected_ids =
        project.domain_ids;

    DomainId expected_opening_id =
        domain_id_generate(
            &expected_ids
        );

    SiteHelperCommandResult result;

    assert(
        sitehelper_command_execute(
            &project,
            &command,
            &result
        )
    );

    assert(
        result.type
        == SITEHELPER_COMMAND_ADD_OPENING
    );

    assert(
        result.data.add_opening.room_id
        == room_id
    );

    assert(
        result.data.add_opening.wall_id
        == wall_id
    );

    assert(
        result.data.add_opening.opening_id
        == expected_opening_id
    );

    const Opening *created =
        wall_find_opening_by_id_const(
            wall,
            result.data.add_opening.opening_id
        );

    assert(created != NULL);

    assert(
        created->frame_position == 1200
    );

    assert(
        project.domain_ids.next
        == expected_ids.next
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_execute_rejects_unknown_command(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId next_before =
        project.domain_ids.next;

    SiteHelperCommand command = {
        .type =
            (SiteHelperCommandType)999
    };

    SiteHelperCommandResult result = {
        .type =
            SITEHELPER_COMMAND_ADD_OPENING,

        .data.add_opening = {
            .room_id = 100,
            .wall_id = 200,
            .opening_id = 300
        }
    };

    assert(
        !sitehelper_command_execute(
            &project,
            &command,
            &result
        )
    );

    assert(
        result.type
        == SITEHELPER_COMMAND_NONE
    );

    assert(
        project.domain_ids.next
        == next_before
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_failed_command_produces_no_result(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id =
        sitehelper_project_add_room(
            &project
        );

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(
            &project,
            room_id
        );

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.structure,
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
            4200
        )
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    OpeningCommand opening;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &opening
        )
    );

    SiteHelperCommand command;

    assert(
        sitehelper_command_from_opening(
            &opening,
            &command
        )
    );

    SiteHelperCommandResult result = {
        .type =
            SITEHELPER_COMMAND_ADD_OPENING,

        .data.add_opening = {
            .opening_id = 999
        }
    };

    DomainId next_before =
        project.domain_ids.next;

    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    assert(
        !sitehelper_command_execute(
            &project,
            &command,
            &result
        )
    );

    assert(
        result.type
        == SITEHELPER_COMMAND_NONE
    );

    assert(
        project.domain_ids.next
        == next_before
    );

    assert(
        wall->definition.opening_count
        == 0
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_undo_dispatches_opening_command(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id =
        sitehelper_project_add_room(
            &project
        );

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(
            &project,
            room_id
        );

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.structure,
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
            4200
        )
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    OpeningCommand opening;

    assert(
        opening_command_create(
            room_id,
            wall_id,
            OPENING_WINDOW,
            1200,
            900,
            1200,
            1200,
            &opening
        )
    );

    SiteHelperCommand command;

    assert(
        sitehelper_command_from_opening(
            &opening,
            &command
        )
    );

    SiteHelperCommandResult result;

    assert(
        sitehelper_command_execute(
            &project,
            &command,
            &result
        )
    );

    DomainId next_before_undo =
        project.domain_ids.next;

    assert(
        sitehelper_command_undo(
            &project,
            &command,
            &result
        )
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            result.data.add_opening.opening_id
        ) == NULL
    );

    assert(
        project.domain_ids.next
        == next_before_undo
    );

    sitehelper_project_destroy(
        &project
    );
}

int main(void)
{
    test_execute_dispatches_opening_command();
    test_execute_rejects_unknown_command();
    test_failed_command_produces_no_result();
    test_undo_dispatches_opening_command();

    printf(
        "All SiteHelper command tests passed.\n"
    );

    return 0;
}