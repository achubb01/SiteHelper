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
    return build_remove_wall_by_id(&project->structure, command->wall_id);
}

void deleted_wall_snapshot_destroy(DeletedWallSnapshot *snapshot)
{
    if (snapshot == NULL) {
        return;
    }
    free(snapshot->openings);
    free(snapshot->room_ids);
    *snapshot = (DeletedWallSnapshot){0};
}

int deleted_wall_snapshot_capture(const SiteHelperProject *project,
    const DeleteWallCommand *command, DeletedWallSnapshot *snapshot)
{
    if (project == NULL || command == NULL || snapshot == NULL) {
        return 0;
    }
    const Wall *wall = build_find_wall_by_id_const(&project->structure, command->wall_id);
    if (wall == NULL) {
        return 0;
    }
    DeletedWallSnapshot candidate = {
        .wall_id = wall->id,
        .segment = wall->definition.segment,
        .opening_count = wall->definition.opening_count
    };
    for (size_t i = 0; i < project->structure.room_count; i++) {
        if (room_has_wall_id(&project->structure.rooms[i], wall->id)) {
            candidate.room_count++;
        }
    }
    if (candidate.opening_count > SIZE_MAX / sizeof *candidate.openings ||
        candidate.room_count > SIZE_MAX / sizeof *candidate.room_ids) {
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
    if (candidate.room_count > 0) {
        candidate.room_ids = malloc(candidate.room_count * sizeof *candidate.room_ids);
        if (candidate.room_ids == NULL) {
            deleted_wall_snapshot_destroy(&candidate);
            return 0;
        }
        size_t saved = 0;
        for (size_t i = 0; i < project->structure.room_count; i++) {
            const Room *room = &project->structure.rooms[i];
            if (room_has_wall_id(room, wall->id)) {
                candidate.room_ids[saved++] = room->id;
            }
        }
    }
    *snapshot = candidate;
    return 1;
}

/* Restoration must not reuse an identity claimed by any live domain object. */
static int identity_in_use(const BuildStructure *structure, DomainId id)
{
    if (id == DOMAIN_ID_INVALID || build_find_room_by_id_const(structure, id) != NULL ||
        build_find_wall_by_id_const(structure, id) != NULL) {
        return 1;
    }
    for (size_t i = 0; i < structure->wall_count; i++) {
        if (wall_find_opening_by_id_const(&structure->walls[i], id) != NULL) {
            return 1;
        }
    }
    return 0;
}

int deleted_wall_snapshot_restore(SiteHelperProject *project,
    const DeletedWallSnapshot *snapshot)
{
    if (project == NULL || snapshot == NULL ||
        identity_in_use(&project->structure, snapshot->wall_id)) {
        return 0;
    }
    for (size_t i = 0; i < snapshot->opening_count; i++) {
        if (identity_in_use(&project->structure, snapshot->openings[i].id)) {
            return 0;
        }
    }
    for (size_t i = 0; i < snapshot->room_count; i++) {
        if (build_find_room_by_id_const(&project->structure, snapshot->room_ids[i]) == NULL) {
            return 0;
        }
    }
    /* Reject dangling memberships before insertion so rollback only removes
     * references added by this restoration attempt. */
    for (size_t i = 0; i < project->structure.room_count; i++) {
        if (room_has_wall_id(&project->structure.rooms[i], snapshot->wall_id)) {
            return 0;
        }
    }

    Wall candidate = { .id = snapshot->wall_id };
    if (!wall_set_plan_segment(&candidate, snapshot->segment)) {
        return 0;
    }
    for (size_t i = 0; i < snapshot->opening_count; i++) {
        if (!wall_add_opening_definition(&candidate, &project->settings, &snapshot->openings[i])) {
            wall_destroy(&candidate);
            return 0;
        }
    }
    if (!wall_generate(&candidate, &project->settings) ||
        !build_append_wall(&project->structure, &candidate)) {
        wall_destroy(&candidate);
        return 0;
    }
    /* BuildStructure now owns the candidate. Deletion is a non-allocating
     * rollback that also removes any memberships already restored here. */
    for (size_t i = 0; i < snapshot->room_count; i++) {
        Room *room = build_find_room_by_id(&project->structure, snapshot->room_ids[i]);
        if (!room_add_wall_reference(room, snapshot->wall_id)) {
            (void)build_remove_wall_by_id(&project->structure, snapshot->wall_id);
            return 0;
        }
    }
    return 1;
}
