#include "wall_command.h"

#include "wall.h"

static int wall_command_build_wall(
    const BuildSettings *settings,
    const WallCommand *command,
    DomainId wall_id,
    Wall *wall
)
{
    if (settings == NULL || command == NULL || wall == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return 0;
    }

    *wall = (Wall){ .id = wall_id };

    if (!wall_set_plan_segment(wall, command->segment) ||
        !wall_generate(wall, settings)) {

        wall_destroy(wall);
        return 0;
    }

    return 1;
}

int wall_command_create(
    DomainId storey_id,
    WallPlanSegment segment,
    WallCommand *command
)
{
    if (command == NULL || storey_id == DOMAIN_ID_INVALID ||
        wall_plan_segment_length_mm(segment) == 0) {
        return 0;
    }

    *command = (WallCommand){
        .storey_id = storey_id, .segment = segment
    };

    return 1;
}

int wall_command_execute(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId *wall_id_out
)
{
    if (wall_id_out == NULL) {
        return 0;
    }

    *wall_id_out = DOMAIN_ID_INVALID;

    if (project == NULL || command == NULL ||
        wall_plan_segment_length_mm(command->segment) == 0) {

        return 0;
    }

    Storey *storey = sitehelper_project_find_storey_by_id(project, command->storey_id);
    BuildSettings resolved;
    if (storey == NULL || !sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved)) { return 0; }

    DomainIdGenerator candidate_ids = project->domain_ids;
    DomainId wall_id = domain_id_generate(&candidate_ids);
    Wall candidate = {0};

    if (wall_id == DOMAIN_ID_INVALID || candidate_ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, wall_id) ||
        !wall_command_build_wall(
            &resolved,
            command,
            wall_id,
            &candidate)) {

        return 0;
    }

    if (!build_append_wall(&storey->structure, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }

    project->domain_ids = candidate_ids;
    *wall_id_out = wall_id;
    return 1;
}

int wall_command_undo(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId wall_id
)
{
    if (project == NULL || command == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return 0;
    }

    Storey *storey = sitehelper_project_find_storey_by_id(project, command->storey_id);
    return storey != NULL && build_remove_wall_by_id(&storey->structure, wall_id);
}

int wall_command_redo(
    SiteHelperProject *project,
    const WallCommand *command,
    DomainId wall_id
)
{
    if (project == NULL || command == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return 0;
    }

    Storey *storey = sitehelper_project_find_storey_by_id(project, command->storey_id);
    BuildSettings resolved;
    if (storey == NULL || !sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved) ||
        sitehelper_project_contains_domain_id(project, wall_id)) {
        return 0;
    }

    Wall candidate = {0};

    if (!wall_command_build_wall(
            &resolved,
            command,
            wall_id,
            &candidate)) {

        return 0;
    }

    if (!build_append_wall(&storey->structure, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }

    return 1;
}
