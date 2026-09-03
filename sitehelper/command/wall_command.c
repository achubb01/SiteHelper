#include "wall_command.h"

#include "wall.h"

static int wall_command_build_wall(
    const BuildSettings *settings,
    const WallCommand *command,
    DomainId wall_id,
    Wall *wall
)
{
    if (settings == NULL || command == NULL || wall == NULL ||
        wall_id == DOMAIN_ID_INVALID || command->length <= 0) {

        return 0;
    }

    *wall = (Wall){ .id = wall_id };

    if (!wall_set_origin(wall, command->origin) ||
        !wall_set_length(wall, command->length) ||
        !wall_generate(wall, settings)) {

        wall_destroy(wall);
        return 0;
    }

    return 1;
}

int wall_command_create(
    DomainId room_id,
    Position origin,
    int length,
    WallCommand *command
)
{
    if (command == NULL || room_id == DOMAIN_ID_INVALID || length <= 0) {
        return 0;
    }

    *command = (WallCommand){
        .room_id = room_id,
        .origin = origin,
        .length = length
    };

    return 1;
}

int wall_command_execute(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId *wall_id_out
)
{
    if (wall_id_out == NULL) {
        return 0;
    }

    *wall_id_out = DOMAIN_ID_INVALID;

    if (project == NULL || command == NULL ||
        command->room_id == DOMAIN_ID_INVALID || command->length <= 0) {

        return 0;
    }

    Room *room = build_find_room_by_id(
        &project->structure,
        command->room_id
    );

    if (room == NULL) {
        return 0;
    }

    DomainIdGenerator candidate_ids = project->domain_ids;
    DomainId wall_id = domain_id_generate(&candidate_ids);
    Wall candidate = {0};

    if (wall_id == DOMAIN_ID_INVALID ||
        !wall_command_build_wall(
            &project->settings,
            command,
            wall_id,
            &candidate)) {

        return 0;
    }

    if (!room_append_wall(room, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }

    project->domain_ids = candidate_ids;
    *wall_id_out = wall_id;
    return 1;
}

int wall_command_undo(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId wall_id
)
{
    if (project == NULL || command == NULL ||
        command->room_id == DOMAIN_ID_INVALID ||
        wall_id == DOMAIN_ID_INVALID) {

        return 0;
    }

    Room *room = build_find_room_by_id(
        &project->structure,
        command->room_id
    );

    return room != NULL && room_remove_wall_by_id(room, wall_id);
}

int wall_command_redo(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId wall_id
)
{
    if (project == NULL || command == NULL ||
        command->room_id == DOMAIN_ID_INVALID ||
        wall_id == DOMAIN_ID_INVALID) {

        return 0;
    }

    Room *room = build_find_room_by_id(
        &project->structure,
        command->room_id
    );

    if (room == NULL || room_find_wall_by_id(room, wall_id) != NULL) {
        return 0;
    }

    Wall candidate = {0};

    if (!wall_command_build_wall(
            &project->settings,
            command,
            wall_id,
            &candidate)) {

        return 0;
    }

    if (!room_append_wall(room, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }

    return 1;
}
