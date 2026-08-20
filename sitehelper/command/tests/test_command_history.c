#include <assert.h>
#include <stdio.h>

#include "command_history.h"


static Wall *
add_test_wall(
    SiteHelperProject *project,
    int length,
    DomainId *room_id_out,
    DomainId *wall_id_out
)
{
    assert(project != NULL);
    assert(room_id_out != NULL);
    assert(wall_id_out != NULL);

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

    *room_id_out =
        room_id;

    *wall_id_out =
        wall_id;

    return wall;
}


static SiteHelperCommand
make_test_opening_command(
    DomainId room_id,
    DomainId wall_id
)
{
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

    return command;
}


static void
test_execute_records_successful_command(void)
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

    SiteHelperCommand command =
        make_test_opening_command(
            room_id,
            wall_id
        );

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    SiteHelperCommandResult result;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command,
            &result
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.entries != NULL
    );

    assert(
        history.entries[0].command.type
        == SITEHELPER_COMMAND_ADD_OPENING
    );

    assert(
        history.entries[0].result.type
        == SITEHELPER_COMMAND_ADD_OPENING
    );

    assert(
        history.entries[0]
            .result
            .data
            .add_opening
            .opening_id
        ==
        result.data.add_opening.opening_id
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            result.data.add_opening.opening_id
        ) != NULL
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}


static void
test_undo_removes_latest_history_entry(void)
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

    SiteHelperCommand command =
        make_test_opening_command(
            room_id,
            wall_id
        );

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    SiteHelperCommandResult result;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command,
            &result
        )
    );

    assert(
        history.count == 1
    );

    DomainId opening_id =
        result.data.add_opening.opening_id;

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) != NULL
    );

    DomainId next_before_undo =
        project.domain_ids.next;

    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 0
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) == NULL
    );

    /*
     * Undo does not recycle domain IDs.
     */
    assert(
        project.domain_ids.next
        == next_before_undo
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}


static void
test_failed_execute_is_not_recorded(void)
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

    SiteHelperCommand command =
        make_test_opening_command(
            room_id,
            wall_id
        );

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
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
        !sitehelper_command_history_execute(
            &history,
            &project,
            &command,
            &result
        )
    );

    assert(
        history.count == 0
    );

    assert(
        result.type
        == SITEHELPER_COMMAND_NONE
    );

    assert(
        wall->definition.opening_count
        == 0
    );

    assert(
        project.domain_ids.next
        == next_before
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}


static void
test_failed_undo_preserves_history_entry(void)
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

    SiteHelperCommand command =
        make_test_opening_command(
            room_id,
            wall_id
        );

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    SiteHelperCommandResult result;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command,
            &result
        )
    );

    assert(
        history.count == 1
    );

    DomainId opening_id =
        result.data.add_opening.opening_id;

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) != NULL
    );

    DomainId next_before_undo =
        project.domain_ids.next;

    size_t opening_count_before =
        wall->definition.opening_count;

    Timber *studs_before =
        wall->framing.studs;

    size_t stud_count_before =
        wall->framing.stud_count;

    /*
     * Force regeneration during undo to fail.
     */
    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    assert(
        !sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    /*
     * Failed undo must remain available
     * for a later retry.
     */
    assert(
        history.count == 1
    );

    /*
     * Authoritative wall state must also
     * remain unchanged.
     */
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
        == next_before_undo
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}


int
main(void)
{
    test_execute_records_successful_command();
    test_undo_removes_latest_history_entry();
    test_failed_execute_is_not_recorded();
    test_failed_undo_preserves_history_entry();

    printf(
        "All command history tests passed.\n"
    );

    return 0;
}