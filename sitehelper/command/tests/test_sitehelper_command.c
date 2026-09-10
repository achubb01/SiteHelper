#include <assert.h>
#include <stdio.h>

#include "sitehelper_command.h"
#include "wall.h"

static void
test_execute_dispatches_opening_command(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);

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
    assert(sitehelper_project_add_storey(&project, 0));

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
    assert(sitehelper_project_add_storey(&project, 0));

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);

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
    assert(sitehelper_project_add_storey(&project, 0));

    DomainId room_id =
        sitehelper_project_add_room(&project, project.storeys[0].id);

    assert(
        room_id != DOMAIN_ID_INVALID
    );

    DomainId wall_id =
        sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){ .end = { .x = 4200 } });

    assert(
        wall_id != DOMAIN_ID_INVALID
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);

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

static void
test_redo_dispatches_opening_command(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

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

    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);

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
        sitehelper_command_execute(
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
        sitehelper_command_undo(
            &project,
            &command,
            &result
        )
    );

    assert(
        wall_find_opening_by_id_const(
            wall,
            opening_id
        ) == NULL
    );

    assert(
        sitehelper_command_redo(
            &project,
            &command,
            &result
        )
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
    test_redo_dispatches_opening_command();

    printf(
        "All SiteHelper command tests passed.\n"
    );

    return 0;
}
