#include "room_separator_command.h"

int add_room_separator_command_create(DomainId storey_id, PlanSegment segment, AddRoomSeparatorCommand *command)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID || !plan_segment_valid(segment)) { return 0; }
    *command = (AddRoomSeparatorCommand){.storey_id = storey_id, .segment = segment};
    return 1;
}

int add_room_separator_command_execute(SiteHelperProject *project,
    const AddRoomSeparatorCommand *command, DomainId *id_out)
{
    if (id_out == NULL) { return 0; }
    *id_out = DOMAIN_ID_INVALID;
    if (command == NULL) { return 0; }
    *id_out = sitehelper_project_add_room_separator(project, command->storey_id, command->segment);
    return *id_out != DOMAIN_ID_INVALID;
}

int delete_room_separator_command_create(DomainId id, DeleteRoomSeparatorCommand *command)
{
    if (command == NULL || id == DOMAIN_ID_INVALID) { return 0; }
    *command = (DeleteRoomSeparatorCommand){.separator_id = id};
    return 1;
}

int delete_room_separator_command_execute(SiteHelperProject *project,
    const DeleteRoomSeparatorCommand *command)
{
    return command != NULL && sitehelper_project_remove_room_separator_by_id(project, command->separator_id);
}

int move_room_separator_endpoint_command_create(DomainId id, RoomSeparatorEndpoint endpoint,
    PlanPosition position, MoveRoomSeparatorEndpointCommand *command)
{
    if (command == NULL || id == DOMAIN_ID_INVALID ||
        (endpoint != ROOM_SEPARATOR_ENDPOINT_START && endpoint != ROOM_SEPARATOR_ENDPOINT_END)) {
        return 0;
    }
    *command = (MoveRoomSeparatorEndpointCommand){
        .separator_id = id, .endpoint = endpoint, .new_position = position
    };
    return 1;
}

int move_room_separator_endpoint_command_execute(SiteHelperProject *project,
    const MoveRoomSeparatorEndpointCommand *command)
{
    if (project == NULL || command == NULL) { return 0; }
    const RoomSeparator *separator = sitehelper_project_find_room_separator_by_id_const(project, command->separator_id);
    if (separator == NULL) { return 0; }
    PlanSegment segment = separator->segment;
    switch (command->endpoint) {
        case ROOM_SEPARATOR_ENDPOINT_START: segment.start = command->new_position; break;
        case ROOM_SEPARATOR_ENDPOINT_END: segment.end = command->new_position; break;
        default: return 0;
    }
    return sitehelper_project_set_room_separator_segment(project, separator->id, segment);
}
