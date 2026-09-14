#include "delete_slab_command.h"
int delete_slab_command_create(DomainId id, DeleteSlabCommand *command)
{
    if (id == DOMAIN_ID_INVALID || command == NULL) { return 0; }
    *command=(DeleteSlabCommand){id}; return 1;
}
int delete_slab_command_execute(SiteHelperProject *project, const DeleteSlabCommand *command)
{
    return project != NULL && command != NULL &&
        sitehelper_project_remove_slab_by_id(project,command->slab_id);
}
