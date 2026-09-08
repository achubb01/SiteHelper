#ifndef DELETE_WALL_COMMAND_H
#define DELETE_WALL_COMMAND_H

#include "sitehelper_project.h"

typedef struct
{
    DomainId wall_id;
} DeleteWallCommand;

int delete_wall_command_create(DomainId wall_id, DeleteWallCommand *command);
int delete_wall_command_execute(SiteHelperProject *project, const DeleteWallCommand *command);

#endif
