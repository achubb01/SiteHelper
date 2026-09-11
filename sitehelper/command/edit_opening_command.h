#ifndef EDIT_OPENING_COMMAND_H
#define EDIT_OPENING_COMMAND_H

#include "sitehelper_project.h"
#include "opening.h"

typedef struct
{
    DomainId wall_id;
    DomainId opening_id;
    Opening definition;
} EditOpeningCommand;

/* Construction checks identity only. Geometry belongs to the Wall transaction. */
int edit_opening_command_create(DomainId wall_id, DomainId opening_id,
    const Opening *definition, EditOpeningCommand *command);
int edit_opening_command_execute(SiteHelperProject *project,
    const EditOpeningCommand *command);

#endif
