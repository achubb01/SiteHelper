#ifndef DELETE_SLAB_COMMAND_H
#define DELETE_SLAB_COMMAND_H
#include "sitehelper_project.h"
typedef struct { DomainId slab_id; } DeleteSlabCommand;
int delete_slab_command_create(DomainId slab_id, DeleteSlabCommand *command);
int delete_slab_command_execute(SiteHelperProject *project, const DeleteSlabCommand *command);
#endif
