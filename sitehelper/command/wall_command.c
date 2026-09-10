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
    WallPlanSegment segment,
    WallCommand *command
)
{
    if (command == NULL ||
        wall_plan_segment_length_mm(segment) == 0) {
        return 0;
    }

    *command = (WallCommand){
        .segment = segment
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

    DomainIdGenerator candidate_ids = project->domain_ids;
    DomainId wall_id = domain_id_generate(&candidate_ids);
    Wall candidate = {0};

    if (wall_id == DOMAIN_ID_INVALID ||
        !wall_command_build_wall(
            &project->settings,
            command,
            wall_id,
            &candidate)) {

        return 0;
    }

    if (!build_append_wall(&project->structure, &candidate)) {
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

    return build_remove_wall_by_id(&project->structure, wall_id);
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

    if (build_find_wall_by_id(&project->structure, wall_id) != NULL) {
        return 0;
    }

    Wall candidate = {0};

    if (!wall_command_build_wall(
            &project->settings,
            command,
            wall_id,
            &candidate)) {

        return 0;
    }

    if (!build_append_wall(&project->structure, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }

    return 1;
}
