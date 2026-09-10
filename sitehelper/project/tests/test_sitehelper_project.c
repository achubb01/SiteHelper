#include <assert.h>
#include <stdio.h>

#include "sitehelper_project.h"
#include "wall.h"

static void test_project_init_sets_defaults(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    assert(project.settings.stud_height == 2400);
    assert(project.settings.stud_depth == 90);
    assert(project.settings.stud_width == 35);

    assert(project.settings.stud_spacing == 600);
    assert(project.settings.nog_spacing == 1200);

    assert(
        project.settings.opening_width_allowance == 0
    );

    assert(
        project.settings.opening_height_allowance == 0
    );

    assert(
        project.settings.stud_spacing_mode ==
        STUD_SPACING_MAXIMISE
    );

    assert(project.storeys == NULL);
    assert(project.storey_count == 0);
    assert(project.storey_capacity == 0);
}

static void test_project_init_initialises_domain_ids(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    assert(
        domain_id_generate(
            &project.domain_ids
        ) == 1
    );

    assert(
        domain_id_generate(
            &project.domain_ids
        ) == 2
    );
}

static void test_add_wall_without_room(void)
{
    SiteHelperProject project;

    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));

    DomainId next_before = project.domain_ids.next;

    DomainId wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){ .end = { .x = 4200 } });
    assert(wall_id == next_before);
    assert(project.domain_ids.next == next_before + 1);
    assert(project.storeys[0].structure.room_count == 0);
    assert(project.storeys[0].structure.wall_count == 1);
    assert(build_find_wall_by_id(&project.storeys[0].structure, wall_id));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);

    sitehelper_project_destroy(&project);
}

static void test_add_wall_requires_valid_ordered_geometry(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId room_id = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(room_id != DOMAIN_ID_INVALID);
    DomainId next = project.domain_ids.next;
    assert(sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){0}) ==
        DOMAIN_ID_INVALID);
    assert(project.domain_ids.next == next);
    assert(project.storeys[0].structure.wall_count == 0);

    WallPlanSegment segment = { .start = {5000, 5000}, .end = {1000, 2000} };
    assert(sitehelper_project_add_wall(&project, project.storeys[0].id, segment) == next);
    const Wall *wall = build_find_wall_by_id_const(&project.storeys[0].structure, next);
    assert(wall != NULL);
    assert(wall->definition.segment.start.x == 5000);
    assert(wall->definition.segment.start.y == 5000);
    assert(wall->definition.segment.end.x == 1000);
    assert(wall->definition.segment.end.y == 2000);
    assert(build_find_room_by_id(&project.storeys[0].structure, room_id));
    sitehelper_project_destroy(&project);
}

static void test_project_destroy_releases_structure(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    DomainId room_id =
        domain_id_generate(
            &project.domain_ids
        );

    assert(
        build_add_room(
            &project.storeys[0].structure,
            room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &project.storeys[0].structure,
            room_id
        );

    assert(room != NULL);

    DomainId wall_id =
        domain_id_generate(
            &project.domain_ids
        );

    Wall candidate = { .id = wall_id };

    assert(build_append_wall(&project.storeys[0].structure, &candidate));

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

    /*
     * Make sure we actually created dynamically
     * owned framing before testing destruction.
     */
    assert(
        wall->framing.studs != NULL
    );

    sitehelper_project_destroy(
        &project
    );

    assert(project.storeys == NULL);
    assert(project.storey_count == 0);
    assert(project.storey_capacity == 0);
}

static void test_project_destroy_handles_empty_project(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );
    assert(sitehelper_project_add_storey(&project, 0));

    sitehelper_project_destroy(
        &project
    );

    /*
     * Destruction should remain safe after
     * ownership has already been released.
     */
    sitehelper_project_destroy(
        &project
    );

    sitehelper_project_destroy(NULL);
}

int main(void)
{
    test_add_wall_requires_valid_ordered_geometry();
    test_project_init_sets_defaults();

    test_project_init_initialises_domain_ids();

    test_add_wall_without_room();

    test_project_destroy_releases_structure();

    test_project_destroy_handles_empty_project();

    printf(
        "All SiteHelper project tests passed.\n"
    );

    return 0;
}
