#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "test_support.h"

static void assert_segment(const RoomSeparator *separator, DomainId id, PlanSegment segment)
{
    assert(separator && separator->id == id);
    assert(separator->segment.start.x == segment.start.x && separator->segment.start.y == segment.start.y);
    assert(separator->segment.end.x == segment.end.x && separator->segment.end.y == segment.end.y);
}

int main(void)
{
    SiteHelperProject project, before;
    sitehelper_project_init(&project);
    assert(sitehelper_project_add_storey(&project, 0));
    PlanSegment reversed = {{5000, 6000}, {1000, 2000}};
    PlanSegment wide = {{INT_MIN, INT_MAX}, {INT_MAX, INT_MIN}};
    DomainId next = project.domain_ids.next;
    DomainId first = sitehelper_project_add_room_separator(&project, project.storeys[0].id, reversed);
    assert(first == next && project.domain_ids.next == next + 1);
    assert(project.storeys[0].structure.room_count == 0 && project.storeys[0].structure.wall_count == 0);
    assert(!build_find_wall_by_id(&project.storeys[0].structure, first));
    assert_segment(build_find_room_separator_by_id(&project.storeys[0].structure, first), first, reversed);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    DomainId second = sitehelper_project_add_room_separator(&project, project.storeys[0].id, wide);
    assert_segment(build_find_room_separator_by_id_const(&project.storeys[0].structure, second), second, wide);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(sitehelper_project_set_room_separator_segment(&project, first, wide));
    assert_segment(build_find_room_separator_by_id_const(&project.storeys[0].structure, first), first, wide);
    assert(sitehelper_project_set_room_separator_segment(&project, first, reversed));

    DomainId room = sitehelper_project_add_room(&project, project.storeys[0].id);
    assert(sitehelper_project_set_room_location(&project, room, (PlanPosition){-2, 3}));
    assert(sitehelper_project_add_room(&project, project.storeys[0].id)); /* Unplaced room also remains independent. */
    DomainId wall_id = sitehelper_project_add_wall(&project, project.storeys[0].id, (WallPlanSegment){{0, 0}, {6000, 0}});
    Wall *wall = build_find_wall_by_id(&project.storeys[0].structure, wall_id);
    DomainId opening = domain_id_generate(&project.domain_ids);
    assert(wall_add_opening(wall, &project.settings, opening, OPENING_WINDOW, 1200, 900, 800, 1000));
    assert(wall_generate(wall, &project.settings));
    test_clone_project_authoritative(&project, &before);
    next = project.domain_ids.next;
    assert(sitehelper_project_add_room_separator(NULL, DOMAIN_ID_INVALID, reversed) == DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{1, 2}, {1, 2}}) == DOMAIN_ID_INVALID);
    assert(!sitehelper_project_set_room_separator_segment(&project, first, (PlanSegment){0}));
    assert(!sitehelper_project_set_room_separator_segment(NULL, first, reversed));
    DomainId invalid[] = {0, room, wall_id, opening, next + 100};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(!sitehelper_project_remove_room_separator_by_id(&project, invalid[i]));
        assert(!sitehelper_project_set_room_separator_segment(&project, invalid[i], wide));
    }
    assert(!sitehelper_project_remove_room_separator_by_id(NULL, first));
    test_assert_project_authoritative_equal(&before, &project);
    /* Reusing any entity's identity through restoration must be rejected. */
    DomainId occupied[] = {first, second, room, wall_id, opening};
    for (size_t i = 0; i < sizeof occupied / sizeof occupied[0]; i++) {
        RoomSeparator duplicate = {.id = occupied[i], .segment = reversed};
        assert(!build_insert_room_separator(&project.storeys[0].structure, &duplicate, 0));
        test_assert_project_authoritative_equal(&before, &project);
    }
    Wall collision = {.id = first};
    assert(!build_append_wall(&project.storeys[0].structure, &collision));
    assert(!build_add_room(&project.storeys[0].structure, first));
    assert(project.domain_ids.next == next);
    sitehelper_project_destroy(&before);

    /* Crossing/overlap and duplicate geometry impose no topology policy. */
    DomainId overlap = sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 0}, {6000, 0}});
    assert(overlap && sitehelper_project_add_room_separator(&project, project.storeys[0].id, (PlanSegment){{0, 0}, {6000, 0}}));
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    assert(build_find_room_by_id(&project.storeys[0].structure, room)->has_location);
    assert(build_find_room_by_id(&project.storeys[0].structure, room)->location.x == -2);
    assert(build_find_room_by_id(&project.storeys[0].structure, room)->location.y == 3);
    assert(project.storeys[0].structure.wall_count == 1 && wall->definition.opening_count == 1);
    assert(wall_find_opening_by_id_const(wall, opening));
    assert(wall->framing.stud_count > 0);
    test_clone_project_authoritative(&project, &before);
    test_assert_project_authoritative_equal(&before, &project);
    assert(sitehelper_project_remove_room_separator_by_id(&project, second));
    assert(project.storeys[0].structure.room_separators[0].id == first);
    assert(project.storeys[0].structure.room_separators[1].id == overlap);
    assert_segment(build_find_room_separator_by_id(&project.storeys[0].structure, first), first, reversed);
    test_assert_wall_definition_equal(&before.storeys[0].structure.walls[0], wall);
    sitehelper_project_destroy(&before);
    next = project.domain_ids.next;
    project.domain_ids.next = UINT64_MAX;
    assert(sitehelper_project_add_room_separator(&project, project.storeys[0].id, wide) == DOMAIN_ID_INVALID);
    assert(project.domain_ids.next == UINT64_MAX);
    project.domain_ids.next = next;
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&project);
    assert(project.storeys == NULL && project.storey_count == 0);
    puts("room separator domain tests passed");
    return 0;
}
