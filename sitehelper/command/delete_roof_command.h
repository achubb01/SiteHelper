#ifndef DELETE_ROOF_COMMAND_H
#define DELETE_ROOF_COMMAND_H

#include "sitehelper_project.h"

typedef struct { DomainId roof_id; } DeleteRoofCommand;

int delete_roof_command_create(DomainId roof_id, DeleteRoofCommand *command);
int delete_roof_command_execute(SiteHelperProject *project, const DeleteRoofCommand *command);

#endif
