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

    assert(project.structure.rooms == NULL);
    assert(project.structure.room_count == 0);
    assert(project.structure.room_capacity == 0);
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

static void test_project_destroy_releases_structure(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

    DomainId room_id =
        domain_id_generate(
            &project.domain_ids
        );

    assert(
        build_add_room(
            &project.structure,
            room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &project.structure,
            room_id
        );

    assert(room != NULL);

    DomainId wall_id =
        domain_id_generate(
            &project.domain_ids
        );

    assert(
        room_add_wall(
            room,
            wall_id
        )
    );

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

    assert(project.structure.rooms == NULL);
    assert(project.structure.room_count == 0);
    assert(project.structure.room_capacity == 0);
}

static void test_project_destroy_handles_empty_project(void)
{
    SiteHelperProject project;

    sitehelper_project_init(
        &project
    );

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
    test_project_init_sets_defaults();

    test_project_init_initialises_domain_ids();

    test_project_destroy_releases_structure();

    test_project_destroy_handles_empty_project();

    printf(
        "All SiteHelper project tests passed.\n"
    );

    return 0;
}