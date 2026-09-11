#include "edit_opening_command.h"
#include "wall.h"

int edit_opening_command_create(DomainId wall_id, DomainId opening_id,
    const Opening *definition, EditOpeningCommand *command)
{
    if (command == NULL || definition == NULL || wall_id == DOMAIN_ID_INVALID ||
        opening_id == DOMAIN_ID_INVALID || definition->id != opening_id) { return 0; }
    *command = (EditOpeningCommand){wall_id, opening_id, *definition};
    return 1;
}

int edit_opening_command_execute(SiteHelperProject *project,
    const EditOpeningCommand *command)
{
    if (project == NULL || command == NULL) { return 0; }
    Wall *wall = sitehelper_project_find_wall_by_id(project, command->wall_id);
    if (wall == NULL) { return 0; }
    const Storey *storey = sitehelper_project_find_owning_storey_const(project, wall->id);
    BuildSettings resolved;
    return storey != NULL &&
        sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved) &&
        wall_apply_opening_definition(wall, &resolved, command->opening_id, &command->definition);
}
