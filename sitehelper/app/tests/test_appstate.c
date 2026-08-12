#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "appcontext.h"
#include "appstate.h"
#include "wall.h"


static void test_current_room_resolves_selected_room_by_id(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId room_id = 10;

    assert(
        build_add_room(
            &app.project.structure,
            room_id
        )
    );

    app.editor.current_room_id =
        room_id;

    Room *room =
        app_current_room(
            &app.project,
            &app.editor
        );

    assert(room != NULL);

    assert(
        room->id ==
        room_id
    );

    sitehelper_project_destroy(
        &app.project
    );
}


static void test_current_room_returns_null_when_no_room_selected(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    assert(
        app_current_room(
            &app.project,
            &app.editor
        ) == NULL
    );
}


static void test_current_room_returns_null_when_selected_id_missing(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    app.editor.current_room_id = 999;

    assert(
        app_current_room(
            &app.project,
            &app.editor
        ) == NULL
    );
}


static void test_current_room_survives_room_array_reallocation(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId selected_room_id = 10;

    assert(
        build_add_room(
            &app.project.structure,
            selected_room_id
        )
    );

    app.editor.current_room_id =
        selected_room_id;

    /*
     * Room capacity starts at 1 and grows
     * through realloc(), so these additions
     * force the underlying array to move/grow.
     */
    assert(
        build_add_room(
            &app.project.structure,
            20
        )
    );

    assert(
        build_add_room(
            &app.project.structure,
            30
        )
    );

    assert(
        build_add_room(
            &app.project.structure,
            40
        )
    );

    Room *room =
        app_current_room(
            &app.project,
            &app.editor
        );

    assert(room != NULL);

    assert(
        room->id ==
        selected_room_id
    );

    sitehelper_project_destroy(
        &app.project
    );
}


static void test_current_wall_resolves_selected_wall_by_id(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId room_id = 10;
    DomainId wall_id = 20;

    assert(
        build_add_room(
            &app.project.structure,
            room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &app.project.structure,
            room_id
        );

    assert(room != NULL);

    assert(
        room_add_wall(
            room,
            wall_id
        )
    );

    app.editor.current_room_id =
        room_id;

    app.editor.current_wall_id =
        wall_id;

    Wall *wall =
        app_current_wall(
            &app.project,
            &app.editor
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        wall_id
    );

    sitehelper_project_destroy(
        &app.project
    );
}


static void test_current_wall_returns_null_when_no_wall_selected(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId room_id = 10;

    assert(
        build_add_room(
            &app.project.structure,
            room_id
        )
    );

    app.editor.current_room_id =
        room_id;

    assert(
        app_current_wall(
            &app.project,
            &app.editor
        ) == NULL
    );

    sitehelper_project_destroy(
        &app.project
    );
}


static void test_current_wall_returns_null_when_selected_wall_id_missing(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId room_id = 10;

    assert(
        build_add_room(
            &app.project.structure,
            room_id
        )
    );

    app.editor.current_room_id =
        room_id;

    app.editor.current_wall_id =
        999;

    assert(
        app_current_wall(
            &app.project,
            &app.editor
        ) == NULL
    );

    sitehelper_project_destroy(
        &app.project
    );
}


static void test_current_wall_survives_wall_array_reallocation(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId room_id = 10;
    DomainId selected_wall_id = 20;

    assert(
        build_add_room(
            &app.project.structure,
            room_id
        )
    );

    app.editor.current_room_id =
        room_id;

    Room *room =
        app_current_room(
            &app.project,
            &app.editor
        );

    assert(room != NULL);

    assert(
        room_add_wall(
            room,
            selected_wall_id
        )
    );

    app.editor.current_wall_id =
        selected_wall_id;

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
            &app.project,
            &app.editor
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
            &app.project,
            &app.editor
        );

    assert(room != NULL);

    sitehelper_project_destroy(
        &app.project
    );
}


static void test_current_wall_survives_room_array_reallocation(void)
{
    AppContext app = {0};

sitehelper_project_init(
    &app.project
);

sitehelper_editor_init(
    &app.editor
);

    DomainId selected_room_id = 10;
    DomainId selected_wall_id = 20;

    assert(
        build_add_room(
            &app.project.structure,
            selected_room_id
        )
    );

    Room *room =
        build_find_room_by_id(
            &app.project.structure,
            selected_room_id
        );

    assert(room != NULL);

    assert(
        room_add_wall(
            room,
            selected_wall_id
        )
    );

    app.editor.current_room_id =
        selected_room_id;

    app.editor.current_wall_id =
        selected_wall_id;

    /*
     * Force BuildStructure.rooms to realloc.
     *
     * Any pointer previously held to the
     * selected Room may now be invalid, but
     * AppContext only retains its DomainId.
     */
    assert(
        build_add_room(
            &app.project.structure,
            30
        )
    );

    assert(
        build_add_room(
            &app.project.structure,
            40
        )
    );

    assert(
        build_add_room(
            &app.project.structure,
            50
        )
    );

    Wall *wall =
        app_current_wall(
            &app.project,
            &app.editor
        );

    assert(wall != NULL);

    assert(
        wall->id ==
        selected_wall_id
    );

    room =
        app_current_room(
            &app.project,
            &app.editor
        );

    assert(room != NULL);

    sitehelper_project_destroy(
        &app.project
    );
}

static void test_editor_initial_state_resolves_no_selection(void)
{
    AppContext app = {0};

    sitehelper_project_init(
        &app.project
    );

    sitehelper_editor_init(
        &app.editor
    );

    assert(
        app.editor.current_room_id ==
        DOMAIN_ID_INVALID
    );

    assert(
        app.editor.current_wall_id ==
        DOMAIN_ID_INVALID
    );

    assert(
        app_current_room(
            &app.project,
            &app.editor
        ) == NULL
    );

    assert(
        app_current_wall(
            &app.project,
            &app.editor
        ) == NULL
    );

    sitehelper_project_destroy(
        &app.project
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
    test_editor_initial_state_resolves_no_selection();

    printf(
        "All app state tests passed.\n"
    );

    return 0;
}