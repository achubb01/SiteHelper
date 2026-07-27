#include <stdlib.h>

#include "appstate.h"

// void app_set_current_room(AppContext *app, size_t room) {
//     if (app == NULL) {
//         return NULL;
//     }


// }

// void app_set_current_wall(AppContext *app);

Room *build_get_room(
    BuildStructure *structure,
    size_t index
)
{
    if (structure == NULL) {
        return NULL;
    }

    if (index >= structure->room_count) {
        return NULL;
    }

    return &structure->rooms[index];
}

Room *app_current_room(AppContext *app)
{
    if (app == NULL || !app->room_selected) {
        return NULL;
    }

    return build_get_room(
        &app->structure,
        app->current_room
    );
}

Wall *build_get_wall(
    BuildStructure *structure,
    size_t room_index,
    size_t wall_index
)
{
    if (structure == NULL) {
        return NULL;
    }

    if (room_index >= structure->room_count) {
        return NULL;
    }

    Room *room =
        &structure->rooms[room_index];

    if (wall_index >= room->wall_count) {
        return NULL;
    }

    return &room->walls[wall_index];
}

Wall *app_current_wall(AppContext *app)
{
    if (app == NULL ||
        !app->room_selected ||
        !app->wall_selected) {
        return NULL;
    }

    return build_get_wall(
        &app->structure,
        app->current_room,
        app->current_wall
    );
}