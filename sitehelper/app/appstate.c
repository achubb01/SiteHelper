#include <stdlib.h>

#include "appstate.h"
#include "wall.h"

Room *app_current_room(
    AppContext *app
)
{
    if (app == NULL ||
        !app->room_selected ||
        app->current_room_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_room_by_id(
        &app->project.structure,
        app->current_room_id
    );
}

Wall *app_current_wall(
    AppContext *app
)
{
    if (app == NULL ||
        !app->room_selected ||
        !app->wall_selected ||
        app->current_room_id == DOMAIN_ID_INVALID ||
        app->current_wall_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    Room *room =
        build_find_room_by_id(
            &app->project.structure,
            app->current_room_id
        );

    if (room == NULL) {
        return NULL;
    }

    return room_find_wall_by_id(
        room,
        app->current_wall_id
    );
}