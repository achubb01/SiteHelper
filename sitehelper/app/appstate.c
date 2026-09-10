#include <stdlib.h>

#include "appstate.h"
#include "wall.h"

Room *app_current_room(
    SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    Storey *storey = editor == NULL ? NULL : sitehelper_project_find_storey_by_id(project, editor->current_storey_id);
    if (storey == NULL ||
        editor == NULL) {

        return NULL;
    }

    if (editor->current_room_id ==
        DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_room_by_id(
        &storey->structure,
        editor->current_room_id
    );
}

Wall *app_current_wall(
    SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    Storey *storey = editor == NULL ? NULL : sitehelper_project_find_storey_by_id(project, editor->current_storey_id);
    if (storey == NULL ||
        editor == NULL) {

        return NULL;
    }

    if (editor->current_wall_id ==
        DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_wall_by_id(&storey->structure, editor->current_wall_id);
}

const Room *app_current_room_const(
    const SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    const Storey *storey = editor == NULL ? NULL : sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL ||
        editor == NULL ||
        editor->current_room_id ==
            DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_room_by_id_const(
        &storey->structure,
        editor->current_room_id
    );
}

const Wall *app_current_wall_const(
    const SiteHelperProject *project,
    const SiteHelperEditor *editor
)
{
    const Storey *storey = editor == NULL ? NULL : sitehelper_project_find_storey_by_id_const(project, editor->current_storey_id);
    if (storey == NULL ||
        editor == NULL ||
        editor->current_wall_id ==
            DOMAIN_ID_INVALID) {

        return NULL;
    }

    return build_find_wall_by_id_const(&storey->structure, editor->current_wall_id);
}
