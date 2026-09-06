#include <assert.h>
#include <stdio.h>

#include "sitehelper_command.h"
#include "test_support.h"

static Wall *add_generated_wall(
    SiteHelperProject *project,
    DomainId room_id,
    PlanPosition origin,
    int length,
    DomainId *wall_id_out
)
{
    DomainId wall_id = sitehelper_project_add_wall(project, room_id, (WallPlanSegment){ .end = { .x = 4200 } });
    assert(wall_id != DOMAIN_ID_INVALID);

    Wall *wall = build_find_wall_by_id(&project->structure, wall_id);
    assert(wall != NULL);
    assert(wall_set_plan_segment(wall, (WallPlanSegment){
        .start = origin, .end = { .x = origin.x + length, .y = origin.y }
    }));
    assert(wall_generate(wall, &project->settings));

    if (wall_id_out != NULL) {
        *wall_id_out = wall_id;
    }

    return wall;
}

static void setup_opening_project(
    SiteHelperProject *project,
    DomainId *room_id_out,
    DomainId *target_wall_id_out,
    DomainId *other_wall_id_out
)
{
    sitehelper_project_init(project);

    DomainId room_id = sitehelper_project_add_room(project);
    assert(room_id != DOMAIN_ID_INVALID);

    Wall *target = add_generated_wall(
        project,
        room_id,
        (PlanPosition){ .x = 0, .y = 0 },
        6000,
        target_wall_id_out
    );
    /* Opening snapshots and undo/redo must retain ordered diagonal geometry. */
    assert(wall_set_plan_segment(target, (WallPlanSegment){
        .start = {4600, 6800}, .end = {1000, 2000}
    }));
    assert(wall_generate(target, &project->settings));

    add_generated_wall(
        project,
        room_id,
        (PlanPosition){ .x = 7000, .y = 1500 },
        4200,
        other_wall_id_out
    );

    *room_id_out = room_id;
}

static SiteHelperCommand make_opening_command(
    DomainId room_id,
    DomainId wall_id,
    int frame_position
)
{
    OpeningCommand opening;
    SiteHelperCommand command;

    assert(opening_command_create(
        room_id,
        wall_id,
        OPENING_WINDOW,
        frame_position,
        700,
        820,
        1000,
        &opening
    ));

    assert(sitehelper_command_from_opening(&opening, &command));
    return command;
}

static void test_success_only_changes_intended_authoritative_state(void)
{
    SiteHelperProject project;
    DomainId room_id;
    DomainId target_wall_id;
    DomainId other_wall_id;

    setup_opening_project(
        &project,
        &room_id,
        &target_wall_id,
        &other_wall_id
    );

    SiteHelperProject before;
    test_clone_project_authoritative(&project, &before);

    DomainId next_before = project.domain_ids.next;
    SiteHelperCommand command = make_opening_command(
        room_id,
        target_wall_id,
        1200
    );
    SiteHelperCommandResult result;

    assert(sitehelper_command_execute(&project, &command, &result));
    assert(result.type == SITEHELPER_COMMAND_ADD_OPENING);
    assert(result.data.add_opening.room_id == room_id);
    assert(result.data.add_opening.wall_id == target_wall_id);
    assert(result.data.add_opening.opening_id == next_before);
    assert(project.domain_ids.next == next_before + 1);

    assert(project.structure.room_count == before.structure.room_count);
    assert(project.structure.wall_count == before.structure.wall_count);
    test_assert_build_settings_equal(&before.settings, &project.settings);

    const Room *before_room = build_find_room_by_id_const(
        &before.structure,
        room_id
    );
    const Room *after_room = build_find_room_by_id_const(
        &project.structure,
        room_id
    );

    assert(before_room != NULL && after_room != NULL);
    assert(before_room->wall_count == after_room->wall_count);
    assert(room_has_wall_id(after_room, target_wall_id));
    assert(room_has_wall_id(after_room, other_wall_id));

    const Wall *before_other = build_find_wall_by_id_const(
        &before.structure,
        other_wall_id
    );
    const Wall *after_other = build_find_wall_by_id_const(
        &project.structure,
        other_wall_id
    );

    test_assert_wall_definition_equal(before_other, after_other);

    const Wall *target = build_find_wall_by_id_const(
        &project.structure,
        target_wall_id
    );

    assert(target != NULL);
    assert(target->definition.segment.start.x == 4600);
    assert(target->definition.segment.start.y == 6800);
    assert(target->definition.segment.end.x == 1000);
    assert(target->definition.segment.end.y == 2000);
    assert(target->definition.opening_count == 1);
    assert(wall_find_opening_by_id_const(
        target,
        result.data.add_opening.opening_id
    ) != NULL);

    sitehelper_project_destroy(&before);
    sitehelper_project_destroy(&project);
}

static void test_failure_leaves_full_authoritative_state_unchanged(void)
{
    SiteHelperProject project;
    DomainId room_id;
    DomainId target_wall_id;
    DomainId other_wall_id;

    setup_opening_project(
        &project,
        &room_id,
        &target_wall_id,
        &other_wall_id
    );

    Wall *target = build_find_wall_by_id(
        &project.structure,
        target_wall_id
    );
    assert(target != NULL);

    DomainId existing_id = domain_id_generate(&project.domain_ids);
    assert(existing_id != DOMAIN_ID_INVALID);
    assert(wall_add_opening(
        target,
        &project.settings,
        existing_id,
        OPENING_WINDOW,
        1200,
        700,
        820,
        1000
    ));
    assert(wall_generate(target, &project.settings));

    SiteHelperProject before;
    test_clone_project_authoritative(&project, &before);

    SiteHelperCommand overlapping = make_opening_command(
        room_id,
        target_wall_id,
        1400
    );
    SiteHelperCommandResult result = {
        .type = SITEHELPER_COMMAND_ADD_WALL
    };

    assert(!sitehelper_command_execute(&project, &overlapping, &result));
    assert(result.type == SITEHELPER_COMMAND_NONE);
    test_assert_project_authoritative_equal(&before, &project);

    sitehelper_project_destroy(&before);
    sitehelper_project_destroy(&project);
}

static void test_undo_restores_and_redo_recreates_exact_authoritative_state(void)
{
    SiteHelperProject project;
    DomainId room_id;
    DomainId target_wall_id;
    DomainId other_wall_id;

    setup_opening_project(
        &project,
        &room_id,
        &target_wall_id,
        &other_wall_id
    );

    SiteHelperProject before;
    test_clone_project_authoritative(&project, &before);

    SiteHelperCommand command = make_opening_command(
        room_id,
        target_wall_id,
        1200
    );
    SiteHelperCommandResult result;

    assert(sitehelper_command_execute(&project, &command, &result));

    SiteHelperProject committed;
    test_clone_project_authoritative(&project, &committed);

    DomainId next_after_execute = project.domain_ids.next;

    assert(sitehelper_command_undo(&project, &command, &result));
    test_assert_project_model_equal(&before, &project);

    /*
     * Undo restores the domain model but deliberately does not recycle
     * a committed identity. The allocator watermark remains monotonic.
     */
    assert(project.domain_ids.next == next_after_execute);

    assert(sitehelper_command_redo(&project, &command, &result));
    test_assert_project_authoritative_equal(&committed, &project);

    const Wall *target = build_find_wall_by_id_const(
        &project.structure,
        target_wall_id
    );

    assert(target != NULL);
    assert(wall_find_opening_by_id_const(
        target,
        result.data.add_opening.opening_id
    ) != NULL);

    sitehelper_project_destroy(&committed);
    sitehelper_project_destroy(&before);
    sitehelper_project_destroy(&project);
}

int main(void)
{
    test_success_only_changes_intended_authoritative_state();
    test_failure_leaves_full_authoritative_state_unchanged();
    test_undo_restores_and_redo_recreates_exact_authoritative_state();

    puts("command invariant tests passed");
    return 0;
}
