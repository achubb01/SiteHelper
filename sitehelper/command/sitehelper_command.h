#ifndef SITEHELPER_COMMAND_H
#define SITEHELPER_COMMAND_H

#include "opening_command.h"
#include "sitehelper_project.h"

typedef enum
{
    SITEHELPER_COMMAND_NONE = 0,

    SITEHELPER_COMMAND_ADD_OPENING,

    SITEHELPER_COMMAND_COUNT
} SiteHelperCommandType;

typedef struct
{
    SiteHelperCommandType type;

    union
    {
        OpeningCommand opening;
    } data;
} SiteHelperCommand;

typedef struct
{
    SiteHelperCommandType type;

    union
    {
        struct
        {
            DomainId room_id;
            DomainId wall_id;
            DomainId opening_id;
        } add_opening;

    } data;

} SiteHelperCommandResult;

int sitehelper_command_from_opening(
    const OpeningCommand *opening,
    SiteHelperCommand *command
);

int sitehelper_command_execute(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
);

int sitehelper_command_undo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
);

#endif