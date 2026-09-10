#include <assert.h>
#include <stdio.h>

#include "command_history.h"
#include "wall.h"

/* Remaining fixtures use a local project named project. */
static Wall *find_project_wall(
    SiteHelperProject *project,
    DomainId wall_id
)
{
    return build_find_wall_by_id(&project->storeys[0].structure, wall_id);
}


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
        sitehelper_project_add_room(project, project->storeys[0].id);

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(project, project->storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project->storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = build_find_wall_by_id(&project->storeys[0].structure, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = length } })
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
    DomainId wall_id
)
{
    OpeningCommand opening;

    assert(
        opening_command_create(
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
    assert(sitehelper_project_add_storey(&project, 0));

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
        history.cursor == 1
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
test_undo_moves_history_cursor_back(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

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

    /*
    * Undo does not destroy history.
    * It only moves the applied-history cursor.
    */
    assert(
        history.count == 1
    );

    assert(
        history.cursor == 0
    );

    /*
    * The entry must remain intact because
    * redo will need both the command and
    * its original execution result.
    */
    assert(
        history.entries[0].command.type
        == SITEHELPER_COMMAND_ADD_OPENING
    );

    assert(
        history.entries[0]
            .result
            .data
            .add_opening
            .opening_id
        == opening_id
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
    assert(sitehelper_project_add_storey(&project, 0));

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
        history.cursor == 0
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
    assert(sitehelper_project_add_storey(&project, 0));

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
        history.cursor == 1
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

    assert(
        history.cursor == 1
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

static void
test_redo_moves_history_cursor_forward(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id
        != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id
        != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = find_project_wall(&project, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = 4200 } })
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
        sitehelper_command_history_execute(
            &history,
            &project,
            &command,
            &result
        )
    );

    DomainId opening_id =
        result.data.add_opening.opening_id;

    DomainId next_after_execute =
        project.domain_ids.next;

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 1
    );

    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 0
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) == NULL
    );

    assert(
        sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 1
    );

    const Opening *restored =
        wall_find_opening_by_id_const(
            wall,
            opening_id
        );

    assert(restored != NULL);

    assert(
        restored->id
        == opening_id
    );

    assert(
        project.domain_ids.next
        == next_after_execute
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_failed_redo_preserves_history_position(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id
        != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id
        != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = find_project_wall(&project, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = 4200 } })
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
        sitehelper_command_history_execute(
            &history,
            &project,
            &command,
            &result
        )
    );

    DomainId opening_id =
        result.data.add_opening.opening_id;

    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 0
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) == NULL
    );

    Timber *studs_before =
        wall->framing.studs;

    size_t stud_count_before =
        wall->framing.stud_count;

    DomainId next_before =
        project.domain_ids.next;

    /*
     * Force regeneration during redo
     * to fail.
     */
    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    assert(
        !sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    /*
     * Redo failure must not move history.
     */
    assert(
        history.count == 1
    );

    assert(
        history.cursor == 0
    );

    /*
     * The stored entry must remain available
     * for a future successful redo.
     */
    assert(
        history.entries[0].command.type
        == SITEHELPER_COMMAND_ADD_OPENING
    );

    assert(
        history.entries[0]
            .result
            .data
            .add_opening
            .opening_id
        == opening_id
    );

    /*
     * Authoritative project state must also
     * remain unchanged.
     */
    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) == NULL
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

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_execute_after_undo_discards_redo_branch(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id
        != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id
        != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = find_project_wall(&project, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = 6000 } })
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    OpeningCommand opening_a;

    assert(
        opening_command_create(
            wall_id,
            OPENING_WINDOW,
            600,
            900,
            1200,
            1200,
            &opening_a
        )
    );

    SiteHelperCommand command_a;

    assert(
        sitehelper_command_from_opening(
            &opening_a,
            &command_a
        )
    );

    SiteHelperCommandResult result_a;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command_a,
            &result_a
        )
    );

    DomainId opening_a_id =
        result_a.data.add_opening.opening_id;

    OpeningCommand opening_b;

    assert(
        opening_command_create(
            wall_id,
            OPENING_WINDOW,
            2400,
            900,
            1200,
            1200,
            &opening_b
        )
    );

    SiteHelperCommand command_b;

    assert(
        sitehelper_command_from_opening(
            &opening_b,
            &command_b
        )
    );

    SiteHelperCommandResult result_b;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command_b,
            &result_b
        )
    );

    DomainId opening_b_id =
        result_b.data.add_opening.opening_id;

    assert(
        history.count == 2
    );

    assert(
        history.cursor == 2
    );

    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 2
    );

    assert(
        history.cursor == 1
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_a_id
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_b_id
        ) == NULL
    );

    OpeningCommand opening_c;

    assert(
        opening_command_create(
            wall_id,
            OPENING_WINDOW,
            3600,
            900,
            1200,
            1200,
            &opening_c
        )
    );

    SiteHelperCommand command_c;

    assert(
        sitehelper_command_from_opening(
            &opening_c,
            &command_c
        )
    );

    SiteHelperCommandResult result_c;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command_c,
            &result_c
        )
    );

    DomainId opening_c_id =
        result_c.data.add_opening.opening_id;

    assert(
        history.count == 2
    );

    assert(
        history.cursor == 2
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_a_id
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_b_id
        ) == NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_c_id
        ) != NULL
    );

    /*
     * B's history slot must now contain C.
     */
    assert(
        history.entries[1]
            .result
            .data
            .add_opening
            .opening_id
        == opening_c_id
    );

    /*
     * There is no redo branch left.
     */
    assert(
        !sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_failed_execute_after_undo_preserves_redo_branch(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id
        != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id
        != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = find_project_wall(&project, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = 6000 } })
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    OpeningCommand opening_a;

    assert(
        opening_command_create(
            wall_id,
            OPENING_WINDOW,
            600,
            900,
            1200,
            1200,
            &opening_a
        )
    );

    SiteHelperCommand command_a;

    assert(
        sitehelper_command_from_opening(
            &opening_a,
            &command_a
        )
    );

    SiteHelperCommandResult result_a;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command_a,
            &result_a
        )
    );

    OpeningCommand opening_b;

    assert(
        opening_command_create(
            wall_id,
            OPENING_WINDOW,
            2400,
            900,
            1200,
            1200,
            &opening_b
        )
    );

    SiteHelperCommand command_b;

    assert(
        sitehelper_command_from_opening(
            &opening_b,
            &command_b
        )
    );

    SiteHelperCommandResult result_b;

    assert(
        sitehelper_command_history_execute(
            &history,
            &project,
            &command_b,
            &result_b
        )
    );

    DomainId opening_b_id =
        result_b.data.add_opening.opening_id;

    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 2
    );

    assert(
        history.cursor == 1
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_b_id
        ) == NULL
    );

    /*
     * Keep a copy of B's recorded identity so
     * we can prove its redo entry survives.
     */
    DomainId stored_redo_id =
        history.entries[1]
            .result
            .data
            .add_opening
            .opening_id;

    assert(
        stored_redo_id
        == opening_b_id
    );

    /*
     * Force the new command execution to fail
     * during wall regeneration.
     */
    project.settings.stud_spacing_mode =
        (StudSpacingMode)999;

    OpeningCommand opening_c;

    assert(
        opening_command_create(
            wall_id,
            OPENING_WINDOW,
            3600,
            900,
            1200,
            1200,
            &opening_c
        )
    );

    SiteHelperCommand command_c;

    assert(
        sitehelper_command_from_opening(
            &opening_c,
            &command_c
        )
    );

    SiteHelperCommandResult result_c;

    assert(
        !sitehelper_command_history_execute(
            &history,
            &project,
            &command_c,
            &result_c
        )
    );

    /*
     * Failed execution must not truncate
     * the redo branch.
     */
    assert(
        history.count == 2
    );

    assert(
        history.cursor == 1
    );

    assert(
        history.entries[1]
            .result
            .data
            .add_opening
            .opening_id
        == opening_b_id
    );

    /*
     * Restore valid settings and prove the
     * original B entry is still redoable.
     */
    project.settings.stud_spacing_mode =
        STUD_SPACING_MAXIMISE;

    assert(
        sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.count == 2
    );

    assert(
        history.cursor == 2
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_b_id
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
test_multiple_commands_can_be_undone_and_redone_in_order(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id
        != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id
        != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = find_project_wall(&project, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = 6000 } })
    );

    assert(
        wall_generate(
            wall,
            &project.settings
        )
    );

    const double positions[] = {
        600,
        2400,
        4200
    };

    DomainId opening_ids[3];

    for (size_t i = 0; i < 3; i++) {

        OpeningCommand opening;

        assert(
            opening_command_create(
                wall_id,
                OPENING_WINDOW,
                positions[i],
                900,
                900,
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
            sitehelper_command_history_execute(
                &history,
                &project,
                &command,
                &result
            )
        );

        opening_ids[i] =
            result.data.add_opening.opening_id;
    }

    assert(
        history.count == 3
    );

    assert(
        history.cursor == 3
    );

    /*
     * All three openings are applied.
     */
    for (size_t i = 0; i < 3; i++) {
        assert(
            wall_find_opening_by_id_const(
                wall,
                opening_ids[i]
            ) != NULL
        );
    }

    /*
     * Undo C.
     */
    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.cursor == 2
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[0]
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[1]
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[2]
        ) == NULL
    );

    /*
     * Undo B.
     */
    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.cursor == 1
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[0]
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[1]
        ) == NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[2]
        ) == NULL
    );

    /*
     * Undo A.
     */
    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.cursor == 0
    );

    for (size_t i = 0; i < 3; i++) {
        assert(
            wall_find_opening_by_id_const(
                wall,
                opening_ids[i]
            ) == NULL
        );
    }

    /*
     * History remains stored even though
     * nothing is currently applied.
     */
    assert(
        history.count == 3
    );

    /*
     * Redo A.
     */
    assert(
        sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.cursor == 1
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[0]
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[1]
        ) == NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[2]
        ) == NULL
    );

    /*
     * Redo B.
     */
    assert(
        sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.cursor == 2
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[0]
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[1]
        ) != NULL
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_ids[2]
        ) == NULL
    );

    /*
     * Redo C.
     */
    assert(
        sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.cursor == 3
    );

    assert(
        history.count == 3
    );

    for (size_t i = 0; i < 3; i++) {
        const Opening *opening =
            wall_find_opening_by_id_const(
                wall,
                opening_ids[i]
            );

        assert(opening != NULL);

        /*
         * Redo must restore the exact
         * original identities.
         */
        assert(
            opening->id
            == opening_ids[i]
        );
    }

    /*
     * Nothing remains to redo.
     */
    assert(
        !sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    sitehelper_command_history_destroy(
        &history
    );

    sitehelper_project_destroy(
        &project
    );
}

static void
test_undo_and_redo_reject_history_boundaries(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    SiteHelperCommandHistory history;

    sitehelper_command_history_init(
        &history
    );

    /*
     * Empty history:
     *
     * count  = 0
     * cursor = 0
     *
     * Neither undo nor redo is valid.
     */
    assert(
        !sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        !sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.count == 0
    );

    assert(
        history.cursor == 0
    );

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id
        != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id
        != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = find_project_wall(&project, wall_id);

    assert(wall != NULL);

    assert(
        wall_set_plan_segment(wall, (WallPlanSegment){ .end = { .x = 4200 } })
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
        history.cursor == 1
    );

    /*
     * cursor == count:
     * nothing remains to redo.
     */
    assert(
        !sitehelper_command_history_redo(
            &history,
            &project
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 1
    );

    assert(
        sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 0
    );

    /*
     * cursor == 0:
     * nothing remains to undo.
     */
    assert(
        !sitehelper_command_history_undo(
            &history,
            &project
        )
    );

    assert(
        history.count == 1
    );

    assert(
        history.cursor == 0
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
    test_undo_moves_history_cursor_back();
    test_redo_moves_history_cursor_forward();
    test_failed_redo_preserves_history_position();
    test_execute_after_undo_discards_redo_branch();
    test_failed_execute_after_undo_preserves_redo_branch();
    test_failed_execute_is_not_recorded();
    test_failed_undo_preserves_history_entry();
    test_multiple_commands_can_be_undone_and_redone_in_order();
    test_undo_and_redo_reject_history_boundaries();

    printf(
        "All command history tests passed.\n"
    );

    return 0;
}
