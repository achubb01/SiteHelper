#include <assert.h>
#include <stdio.h>

#include "wall.h"

static Wall make_wall(DomainId id)
{
    return (Wall){ .id = id };
}

static void test_global_wall_lookup_survives_reallocation(void)
{
    BuildStructure structure = {0};
    Wall first = make_wall(10);
    assert(build_append_wall(&structure, &first));

    for (DomainId id = 11; id < 20; id++) {
        Wall wall = make_wall(id);
        assert(build_append_wall(&structure, &wall));
    }

    assert(build_find_wall_by_id(&structure, 10) != NULL);
    assert(build_find_wall_by_id(&structure, 10)->id == 10);
    build_destroy(&structure);
}

static void test_room_membership_is_id_based_and_shared(void)
{
    BuildStructure structure = {0};
    Wall wall = make_wall(30);
    assert(build_append_wall(&structure, &wall));
    assert(build_add_room(&structure, 1));
    assert(build_add_room(&structure, 2));

    Room *first = build_find_room_by_id(&structure, 1);
    Room *second = build_find_room_by_id(&structure, 2);
    assert(room_add_wall_reference(first, 30));
    assert(room_add_wall_reference(second, 30));
    assert(!room_add_wall_reference(first, 30));
    assert(first->wall_count == 1 && second->wall_count == 1);
    assert(first->wall_ids[0] == 30 && second->wall_ids[0] == 30);
    assert(structure.wall_count == 1);
    assert(build_find_wall_by_id(&structure, first->wall_ids[0]) ==
        build_find_wall_by_id(&structure, second->wall_ids[0]));
    build_destroy(&structure);
}

static void test_room_membership_survives_room_reallocation(void)
{
    BuildStructure structure = {0};
    Wall wall = make_wall(40);
    assert(build_append_wall(&structure, &wall));
    assert(build_add_room(&structure, 1));
    assert(room_add_wall_reference(
        build_find_room_by_id(&structure, 1), 40));
    for (DomainId id = 2; id < 10; id++) {
        assert(build_add_room(&structure, id));
    }
    const Room *room = build_find_room_by_id_const(&structure, 1);
    assert(room != NULL && room_has_wall_id(room, 40));
    assert(build_find_wall_by_id(&structure, room->wall_ids[0])->id == 40);
    build_destroy(&structure);
}

static void test_removing_wall_clears_every_membership(void)
{
    BuildStructure structure = {0};
    Wall wall = make_wall(50);
    assert(build_append_wall(&structure, &wall));
    assert(build_add_room(&structure, 1));
    assert(build_add_room(&structure, 2));
    assert(room_add_wall_reference(build_find_room_by_id(&structure, 1), 50));
    assert(room_add_wall_reference(build_find_room_by_id(&structure, 2), 50));
    assert(build_remove_wall_by_id(&structure, 50));
    assert(build_find_wall_by_id(&structure, 50) == NULL);
    assert(!room_has_wall_id(build_find_room_by_id(&structure, 1), 50));
    assert(!room_has_wall_id(build_find_room_by_id(&structure, 2), 50));
    build_destroy(&structure);
}

static void test_destroying_room_does_not_destroy_physical_wall(void)
{
    BuildStructure structure = {0};
    Room room = { .id = 60 };
    Wall wall = make_wall(61);
    assert(build_append_wall(&structure, &wall));
    assert(room_add_wall_reference(&room, 61));
    room_destroy(&room);
    assert(build_find_wall_by_id(&structure, 61) != NULL);
    build_destroy(&structure);
}

static void test_global_entities_reject_duplicate_domain_ids(void)
{
    BuildStructure structure = {0};
    Wall wall = make_wall(70);
    assert(build_append_wall(&structure, &wall));
    assert(!build_add_room(&structure, 70));
    assert(build_add_room(&structure, 71));
    wall = make_wall(71);
    assert(!build_append_wall(&structure, &wall));
    build_destroy(&structure);
}

int main(void)
{
    test_global_wall_lookup_survives_reallocation();
    test_room_membership_is_id_based_and_shared();
    test_room_membership_survives_room_reallocation();
    test_removing_wall_clears_every_membership();
    test_destroying_room_does_not_destroy_physical_wall();
    test_global_entities_reject_duplicate_domain_ids();
    printf("All build structure tests passed.\n");
    return 0;
}
