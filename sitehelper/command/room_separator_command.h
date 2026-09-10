#ifndef ROOM_SEPARATOR_COMMAND_H
#define ROOM_SEPARATOR_COMMAND_H

#include "sitehelper_project.h"

typedef struct { PlanSegment segment; } AddRoomSeparatorCommand;
typedef struct { DomainId separator_id; } DeleteRoomSeparatorCommand;
typedef enum {
    ROOM_SEPARATOR_ENDPOINT_START,
    ROOM_SEPARATOR_ENDPOINT_END
} RoomSeparatorEndpoint;
typedef struct {
    DomainId separator_id;
    RoomSeparatorEndpoint endpoint;
    PlanPosition new_position;
} MoveRoomSeparatorEndpointCommand;

int add_room_separator_command_create(PlanSegment segment, AddRoomSeparatorCommand *command);
int add_room_separator_command_execute(SiteHelperProject *project,
    const AddRoomSeparatorCommand *command, DomainId *id_out);
int delete_room_separator_command_create(DomainId id, DeleteRoomSeparatorCommand *command);
int delete_room_separator_command_execute(SiteHelperProject *project,
    const DeleteRoomSeparatorCommand *command);
int move_room_separator_endpoint_command_create(DomainId id, RoomSeparatorEndpoint endpoint,
    PlanPosition position, MoveRoomSeparatorEndpointCommand *command);
int move_room_separator_endpoint_command_execute(SiteHelperProject *project,
    const MoveRoomSeparatorEndpointCommand *command);

#endif
