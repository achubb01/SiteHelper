#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "appcontext.h"
#include "appstate.h"
#include "wall.h"


static void test_current_room_resolves_selected_room_by_id(void)
{
    AppContext app = {0};

    DomainId room_id = 10;

    assert(
        build_add_room(
            &app.structure,
            room_id
        )
    );

    app.current_room_id =
        room_id;

    app.room_selected =
        true;

    Room *room =
        app_current_room(
            &app
        );

    assert(room != NULL);

    assert(
        room->id ==
        room_id
    );

    free(
        app.structure.rooms
    );
}


static void test_current_room_returns_null_when_no_room_selected(void)
{
    AppContext app = {0};

    assert(
        app_current_room(
            &app
        ) == NULL
    );
}


static void test_current_room_returns_null_when_selected_id_missing(void)
{
    AppContext app = {0};

    app.current_room_id = 999;
    app.room_selected = true;

    assert(
        app_current_room(
            &app
        ) == NULL
    );
}


static void test_current_room_survives_room_array_reallocation(void)
{
    AppContext app = {0};

    DomainId selected_room_id = 10;

    assert(
        build_add_room(
            &app.structure,
            selected_room_id
        )
    );

    app.current_room_id =
        selected_room_id;

    app.room_selected =
        true;

    /*
     * Room capacity starts at 1 and grows
     * through realloc(), so these additions
     * force the underlying array to move/grow.
     */
    assert(
        build_add_room(
            &app.structure,
            20
        )
    );

    assert(
        build_add_room(
            &app.structure,
            30
        )
    );

    assert(
        build_add_room(
            &app.structure,
            40
        )
    );

    Room *room =
        app_current_room(
            &app
        );

    assert(room != NULL);

    assert(
        room->id ==
        selected_room_id
    );

    free(
        app.structure.rooms
    );
}


static void test_current_wall_resolves_selected_wall_by_id(void)
{
    AppContext app = {0};

    DomainId room_id = 10;
    DomainId wall_id = 20;

    assert(
        build_add_room(
            &app.structure,
            room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &app.structure,
            room_id
        );

    assert(room != NULL);

    assert(
        room_add_wall(
            room,
            wall_id
        )
    );

    app.current_room_id =
        room_id;

    app.current_wall_id =
        wall_id;

    app.room_selected =
        true;

    app.wall_selected =
        true;

    Wall *wall =
        app_current_wall(
            &app
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        wall_id
    );

    free(
        room->walls
    );

    free(
        app.structure.rooms
    );
}


static void test_current_wall_returns_null_when_no_wall_selected(void)
{
    AppContext app = {0};

    DomainId room_id = 10;

    assert(
        build_add_room(
            &app.structure,
            room_id
        )
    );

    app.current_room_id =
        room_id;

    app.room_selected =
        true;

    app.wall_selected =
        false;

    assert(
        app_current_wall(
            &app
        ) == NULL
    );

    free(
        app.structure.rooms
    );
}


static void test_current_wall_returns_null_when_selected_wall_id_missing(void)
{
    AppContext app = {0};

    DomainId room_id = 10;

    assert(
        build_add_room(
            &app.structure,
            room_id
        )
    );

    app.current_room_id =
        room_id;

    app.current_wall_id =
        999;

    app.room_selected =
        true;

    app.wall_selected =
        true;

    assert(
        app_current_wall(
            &app
        ) == NULL
    );

    free(
        app.structure.rooms
    );
}


static void test_current_wall_survives_wall_array_reallocation(void)
{
    AppContext app = {0};

    DomainId room_id = 10;
    DomainId selected_wall_id = 20;

    assert(
        build_add_room(
            &app.structure,
            room_id
        )
    );

    app.current_room_id =
        room_id;

    app.room_selected =
        true;

    Room *room =
        app_current_room(
            &app
        );

    assert(room != NULL);

    assert(
        room_add_wall(
            room,
            selected_wall_id
        )
    );

    app.current_wall_id =
        selected_wall_id;

    app.wall_selected =
        true;

    /*
     * Force Room.walls through several
     * reallocations after the selection
     * has already been stored.
     */
    assert(
        room_add_wall(
            room,
            30
        )
    );

    assert(
        room_add_wall(
            room,
            40
        )
    );

    assert(
        room_add_wall(
            room,
            50
        )
    );

    Wall *wall =
        app_current_wall(
            &app
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        selected_wall_id
    );

    /*
     * app_current_room() performs a fresh
     * resolution. Use that pointer for
     * cleanup rather than retaining some
     * other long-lived reference.
     */
    room =
        app_current_room(
            &app
        );

    assert(room != NULL);

    free(
        room->walls
    );

    free(
        app.structure.rooms
    );
}


static void test_current_wall_survives_room_array_reallocation(void)
{
    AppContext app = {0};

    DomainId selected_room_id = 10;
    DomainId selected_wall_id = 20;

    assert(
        build_add_room(
            &app.structure,
            selected_room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &app.structure,
            selected_room_id
        );

    assert(room != NULL);

    assert(
        room_add_wall(
            room,
            selected_wall_id
        )
    );

    app.current_room_id =
        selected_room_id;

    app.current_wall_id =
        selected_wall_id;

    app.room_selected =
        true;

    app.wall_selected =
        true;

    /*
     * Force BuildStructure.rooms to realloc.
     *
     * Any pointer previously held to the
     * selected Room may now be invalid, but
     * AppContext only retains its DomainId.
     */
    assert(
        build_add_room(
            &app.structure,
            30
        )
    );

    assert(
        build_add_room(
            &app.structure,
            40
        )
    );

    assert(
        build_add_room(
            &app.structure,
            50
        )
    );

    Wall *wall =
        app_current_wall(
            &app
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        selected_wall_id
    );

    room =
        app_current_room(
            &app
        );

    assert(room != NULL);

    free(
        room->walls
    );

    free(
        app.structure.rooms
    );
}


int main(void)
{
    test_current_room_resolves_selected_room_by_id();
    test_current_room_returns_null_when_no_room_selected();
    test_current_room_returns_null_when_selected_id_missing();
    test_current_room_survives_room_array_reallocation();

    test_current_wall_resolves_selected_wall_by_id();
    test_current_wall_returns_null_when_no_wall_selected();
    test_current_wall_returns_null_when_selected_wall_id_missing();
    test_current_wall_survives_wall_array_reallocation();
    test_current_wall_survives_room_array_reallocation();

    printf(
        "All app state tests passed.\n"
    );

    return 0;
}