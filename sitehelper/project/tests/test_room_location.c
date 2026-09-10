#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "test_support.h"

static void assert_location(const SiteHelperProject *project, DomainId id,
    bool placed, PlanPosition position)
{
    const Room *room = build_find_room_by_id_const(&project->storeys[0].structure, id);
    assert(room && room->id == id && room->has_location == placed);
    if (placed) {
        assert(room->location.x == position.x && room->location.y == position.y);
    }
}

int main(void)
{
    SiteHelperProject project, before;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    DomainId first = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId second = sitehelper_project_add_room(&project, project.storeys[0].id);
    DomainId next = project.domain_ids.next;
    assert_location(&project, first, false, (PlanPosition){0});
    assert_location(&project, second, false, (PlanPosition){0});
    PlanPosition positions[] = {{0, 0}, {-12345, 67890}, {INT_MIN, INT_MAX}};
    for (size_t i = 0; i < sizeof positions / sizeof positions[0]; i++) {
        assert(sitehelper_project_set_room_location(&project, first, positions[i]));
        assert_location(&project, first, true, positions[i]);
        assert_location(&project, second, false, (PlanPosition){0});
        assert(project.storeys[0].structure.wall_count == 0);
        assert(project.domain_ids.next == next);
        assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    }
    test_clone_project_authoritative(&project, &before);
    assert(!sitehelper_project_set_room_location(NULL, first, positions[0]));
    assert(!sitehelper_project_clear_room_location(NULL, first));
    DomainId invalid[] = {DOMAIN_ID_INVALID, next + 100};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(!sitehelper_project_set_room_location(&project, invalid[i], positions[0]));
        assert(!sitehelper_project_clear_room_location(&project, invalid[i]));
        test_assert_project_authoritative_equal(&before, &project);
    }
    sitehelper_project_destroy(&before);
    assert(sitehelper_project_clear_room_location(&project, first));
    assert(sitehelper_project_clear_room_location(&project, first));
    assert_location(&project, first, false, (PlanPosition){0});
    assert(project.domain_ids.next == next);

    DomainId wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id,
        (WallPlanSegment){{0, 0}, {4200, 0}});
    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);
    assert(wall && wall_generate(wall, &project.settings));
    next = project.domain_ids.next;
    assert(sitehelper_project_set_room_location(&project, first, positions[2]));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    test_clone_project_authoritative(&project, &before);
    assert(wall_generate(&before.storeys[0].structure.walls[0], &before.settings));
    assert(sitehelper_project_set_room_location(&project, second, positions[1]));
    assert_location(&project, first, true, positions[2]);
    assert_location(&project, second, true, positions[1]);
    test_assert_wall_definition_equal(&before.storeys[0].structure.walls[0], wall);
    test_assert_framing_semantically_equal(&before.storeys[0].structure.walls[0].framing, &wall->framing);
    assert(wall->framing.stud_count > 0 && project.domain_ids.next == next);
    assert(!sitehelper_project_set_room_location(&project, wall_id, positions[0]));
    assert(!sitehelper_project_clear_room_location(&project, wall_id));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&before);
    sitehelper_project_destroy(&project);
    puts("room location domain tests passed");
    return 0;
}
