#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "wall.h"


static void test_find_room_by_id(void)
{
    BuildStructure structure = {0};

    DomainId room_id = 42;

    assert(
        build_add_room(
            &structure,
            room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &structure,
            room_id
        );

    assert(room != NULL);

    assert(
        room->id ==
        room_id
    );

    free(
        structure.rooms
    );
}


static void test_find_room_by_id_rejects_invalid_id(void)
{
    BuildStructure structure = {0};

    assert(
        build_find_room_by_id(
            &structure,
            DOMAIN_ID_INVALID
        ) == NULL
    );
}


static void test_find_room_by_id_returns_null_when_missing(void)
{
    BuildStructure structure = {0};

    assert(
        build_find_room_by_id(
            &structure,
            999
        ) == NULL
    );
}


static void test_find_wall_by_id(void)
{
    Room room = {0};

    DomainId wall_id = 84;

    assert(
        room_add_wall(
            &room,
            wall_id
        )
    );

    Wall *wall =
        room_find_wall_by_id(
            &room,
            wall_id
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        wall_id
    );

    free(
        room.walls
    );
}


static void test_find_wall_by_id_rejects_invalid_id(void)
{
    Room room = {0};

    assert(
        room_find_wall_by_id(
            &room,
            DOMAIN_ID_INVALID
        ) == NULL
    );
}


static void test_find_wall_by_id_returns_null_when_missing(void)
{
    Room room = {0};

    assert(
        room_find_wall_by_id(
            &room,
            999
        ) == NULL
    );
}


static void test_room_identity_survives_reallocation(void)
{
    BuildStructure structure = {0};

    DomainId first_room_id = 1;

    assert(
        build_add_room(
            &structure,
            first_room_id
        )
    );

    /*
     * Initial capacity is 1 and then doubles.
     * Adding several rooms therefore forces
     * the rooms array to reallocate.
     */
    assert(
        build_add_room(
            &structure,
            2
        )
    );

    assert(
        build_add_room(
            &structure,
            3
        )
    );

    assert(
        build_add_room(
            &structure,
            4
        )
    );

    Room *room =
        build_find_room_by_id(
            &structure,
            first_room_id
        );

    assert(room != NULL);

    assert(
        room->id ==
        first_room_id
    );

    free(
        structure.rooms
    );
}


static void test_wall_identity_survives_reallocation(void)
{
    Room room = {0};

    DomainId first_wall_id = 1;

    assert(
        room_add_wall(
            &room,
            first_wall_id
        )
    );

    /*
     * Initial capacity is 1 and then doubles.
     * Adding several walls therefore forces
     * the walls array to reallocate.
     */
    assert(
        room_add_wall(
            &room,
            2
        )
    );

    assert(
        room_add_wall(
            &room,
            3
        )
    );

    assert(
        room_add_wall(
            &room,
            4
        )
    );

    Wall *wall =
        room_find_wall_by_id(
            &room,
            first_wall_id
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        first_wall_id
    );

    free(
        room.walls
    );
}

static void test_build_add_room_rejects_duplicate_id(void)
{
    BuildStructure structure = {0};

    DomainId room_id = 1;

    assert(
        build_add_room(
            &structure,
            room_id
        )
    );

    assert(
        !build_add_room(
            &structure,
            room_id
        )
    );

    assert(
        structure.room_count == 1
    );

    free(
        structure.rooms
    );
}

static void test_room_add_wall_rejects_duplicate_id(void)
{
    Room room = {0};

    DomainId wall_id = 1;

    assert(
        room_add_wall(
            &room,
            wall_id
        )
    );

    assert(
        !room_add_wall(
            &room,
            wall_id
        )
    );

    assert(
        room.wall_count == 1
    );

    free(
        room.walls
    );
}

int main(void)
{
    test_find_room_by_id();
    test_find_room_by_id_rejects_invalid_id();
    test_find_room_by_id_returns_null_when_missing();

    test_find_wall_by_id();
    test_find_wall_by_id_rejects_invalid_id();
    test_find_wall_by_id_returns_null_when_missing();

    test_room_identity_survives_reallocation();
    test_wall_identity_survives_reallocation();

    test_build_add_room_rejects_duplicate_id();
    test_room_add_wall_rejects_duplicate_id();

    printf(
        "All build structure tests passed.\n"
    );

    return 0;
}