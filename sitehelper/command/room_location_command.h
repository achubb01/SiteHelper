#ifndef ROOM_LOCATION_COMMAND_H
#define ROOM_LOCATION_COMMAND_H

#include "sitehelper_project.h"

/* Replace a room's optional semantic plan location. No topology or UI state. */
typedef struct
{
    DomainId room_id;
    bool has_location;
    PlanPosition location;
} RoomLocationCommand;

int room_location_command_create(DomainId room_id, PlanPosition location,
    RoomLocationCommand *command);
int room_location_command_create_clear(DomainId room_id,
    RoomLocationCommand *command);
int room_location_command_execute(SiteHelperProject *project,
    const RoomLocationCommand *command);

#endif
