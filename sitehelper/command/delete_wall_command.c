#include <stdlib.h>
#include <string.h>

#include "delete_wall_command_internal.h"
#include "wall.h"

int delete_wall_command_create(DomainId wall_id, DeleteWallCommand *command)
{
    if (wall_id == DOMAIN_ID_INVALID || command == NULL) {
        return 0;
    }
    *command = (DeleteWallCommand){ .wall_id = wall_id };
    return 1;
}

int delete_wall_command_execute(SiteHelperProject *project, const DeleteWallCommand *command)
{
    if (project == NULL || command == NULL || command->wall_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    return sitehelper_project_remove_wall_by_id(project, command->wall_id);
}

void deleted_wall_snapshot_destroy(DeletedWallSnapshot *snapshot)
{
    if (snapshot == NULL) {
        return;
    }
    free(snapshot->openings);
    *snapshot = (DeletedWallSnapshot){0};
}

int deleted_wall_snapshot_capture(const SiteHelperProject *project,
    const DeleteWallCommand *command, DeletedWallSnapshot *snapshot)
{
    if (project == NULL || command == NULL || snapshot == NULL) {
        return 0;
    }
    const Wall *wall = sitehelper_project_find_wall_by_id_const(project, command->wall_id);
    if (wall == NULL) {
        return 0;
    }
    DeletedWallSnapshot candidate = {
        .storey_id = sitehelper_project_find_owning_storey_const(project, wall->id)->id,
        .wall_id = wall->id,
        .segment = wall->definition.segment,
        .plan_specification = wall->definition.plan_specification,
        .opening_count = wall->definition.opening_count
    };
    if (candidate.opening_count > SIZE_MAX / sizeof *candidate.openings) {
        return 0;
    }
    if (candidate.opening_count > 0) {
        candidate.openings = malloc(candidate.opening_count * sizeof *candidate.openings);
        if (candidate.openings == NULL) {
            return 0;
        }
        memcpy(candidate.openings, wall->definition.openings,
            candidate.opening_count * sizeof *candidate.openings);
    }
    *snapshot = candidate;
    return 1;
}

/* Restoration must not reuse an identity claimed by any live domain object. */
static int identity_in_use(const SiteHelperProject *project, DomainId id)
{
    return id == DOMAIN_ID_INVALID || sitehelper_project_contains_domain_id(project, id);
}

int deleted_wall_snapshot_restore(SiteHelperProject *project,
    const DeletedWallSnapshot *snapshot)
{
    if (project == NULL || snapshot == NULL ||
        identity_in_use(project, snapshot->wall_id)) {
        return 0;
    }
    Storey *storey = sitehelper_project_find_storey_by_id(project, snapshot->storey_id);
    BuildSettings resolved;
    if (storey == NULL || !sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved)) { return 0; }
    for (size_t i = 0; i < snapshot->opening_count; i++) {
        if (identity_in_use(project, snapshot->openings[i].id)) {
            return 0;
        }
    }
    Wall candidate = { .id = snapshot->wall_id };
    if (!wall_set_plan_segment(&candidate, snapshot->segment) ||
        !wall_set_plan_specification(&candidate, snapshot->plan_specification)) {
        return 0;
    }
    for (size_t i = 0; i < snapshot->opening_count; i++) {
        if (!wall_add_opening_definition(&candidate, &resolved, &snapshot->openings[i])) {
            wall_destroy(&candidate);
            return 0;
        }
    }
    if (!wall_generate(&candidate, &resolved) ||
        !build_append_wall(&storey->structure, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }
    return 1;
}
