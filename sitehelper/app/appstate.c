#include <stdlib.h>

#include "appstate.h"
#include "wall.h"

Room *app_current_room(
    AppContext *app
)
{
    if (app == NULL) {
        return NULL;
    }

    if (app->editor.current_room_id ==
        DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_room_by_id(
        &app->project.structure,
        app->editor.current_room_id
    );
}

Wall *app_current_wall(
    AppContext *app
)
{
    if (app == NULL) {
        return NULL;
    }

    if (app->editor.current_wall_id ==
        DOMAIN_ID_INVALID) {

        return NULL;
    }

    Room *room =
        app_current_room(
            app
        );

    if (room == NULL) {
        return NULL;
    }

    return room_find_wall_by_id(
        room,
        app->editor.current_wall_id
    );
}