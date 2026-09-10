#include <assert.h>
#include <stdio.h>

#include "wall.h"

static void test_global_wall_lookup_survives_independent_reallocation(void)
{
    BuildStructure structure = {0};
    for (DomainId id = 10; id < 20; id++) {
        Wall wall = {.id = id};
        assert(build_append_wall(&structure, &wall));
    }
    assert(structure.room_count == 0);
    assert(build_find_wall_by_id(&structure, 10)->id == 10);
    for (DomainId id = 1; id < 10; id++) {
        assert(build_add_room(&structure, id));
    }
    assert(build_find_room_by_id_const(&structure, 1)->id == 1);
    assert(build_find_wall_by_id_const(&structure, 10)->id == 10);
    assert(build_remove_wall_by_id(&structure, 10));
    assert(build_find_wall_by_id(&structure, 10) == NULL);
    assert(structure.room_count == 9);
    assert(build_find_room_by_id(&structure, 1)->id == 1);
    room_destroy(&structure.rooms[0]);
    assert(build_find_wall_by_id(&structure, 11)->id == 11);
    build_destroy(&structure);
}

static void test_global_entities_reject_duplicate_domain_ids(void)
{
    BuildStructure structure = {0};
    Wall wall = {.id = 70};
    assert(build_append_wall(&structure, &wall));
    assert(!build_add_room(&structure, 70));
    assert(build_add_room(&structure, 71));
    wall = (Wall){.id = 71};
    assert(!build_append_wall(&structure, &wall));
    build_destroy(&structure);
}

int main(void)
{
    test_global_wall_lookup_survives_independent_reallocation();
    test_global_entities_reject_duplicate_domain_ids();
    puts("build structure tests passed");
    return 0;
}
