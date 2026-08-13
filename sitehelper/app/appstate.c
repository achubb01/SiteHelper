#include <stdlib.h>

#include "appstate.h"
#include "wall.h"

Room *app_current_room(
    SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    if (project == NULL ||
        editor == NULL) {

        return NULL;
    }

    if (editor->current_room_id ==
        DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_room_by_id(
        &project->structure,
        editor->current_room_id
    );
}

Wall *app_current_wall(
    SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    if (project == NULL ||
        editor == NULL) {

        return NULL;
    }

    if (editor->current_wall_id ==
        DOMAIN_ID_INVALID) {

        return NULL;
    }

    Room *room =
        app_current_room(
            project,
            editor
        );

    if (room == NULL) {
        return NULL;
    }

    return room_find_wall_by_id(
        room,
        editor->current_wall_id
    );
}

const Room *app_current_room_const(
    const SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    if (project == NULL ||
        editor == NULL ||
        editor->current_room_id ==
            DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_room_by_id_const(
        &project->structure,
        editor->current_room_id
    );
}

const Wall *app_current_wall_const(
    const SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    if (project == NULL ||
        editor == NULL ||
        editor->current_wall_id ==
            DOMAIN_ID_INVALID) {

        return NULL;
    }

    const Room *room =
        app_current_room_const(
            project,
            editor
        );

    if (room == NULL) {
        return NULL;
    }

    return room_find_wall_by_id_const(
        room,
        editor->current_wall_id
    );
}