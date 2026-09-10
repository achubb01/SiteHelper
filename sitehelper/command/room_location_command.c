#include "room_location_command.h"

int room_location_command_create(DomainId room_id, PlanPosition location,
    RoomLocationCommand *command)
{
    if (command == NULL || room_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    *command = (RoomLocationCommand){
        .room_id = room_id, .has_location = true, .location = location
    };
    return 1;
}

int room_location_command_create_clear(DomainId room_id,
    RoomLocationCommand *command)
{
    if (command == NULL || room_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    *command = (RoomLocationCommand){.room_id = room_id};
    return 1;
}

int room_location_command_execute(SiteHelperProject *project,
    const RoomLocationCommand *command)
{
    if (command == NULL) {
        return 0;
    }
    return command->has_location
        ? sitehelper_project_set_room_location(project, command->room_id, command->location)
        : sitehelper_project_clear_room_location(project, command->room_id);
}
