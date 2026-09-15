#ifndef MOVE_SLAB_VERTEX_COMMAND_H
#define MOVE_SLAB_VERTEX_COMMAND_H

#include <stddef.h>
#include <stdint.h>

#include "sitehelper_project.h"

typedef enum {
    MOVE_SLAB_VERTEX_OUTLINE = 0,
    MOVE_SLAB_VERTEX_PENETRATION,
    MOVE_SLAB_VERTEX_REGION,
    MOVE_SLAB_VERTEX_TARGET_COUNT
} MoveSlabVertexTarget;

typedef struct {
    DomainId slab_id;
    MoveSlabVertexTarget target;
    /* SIZE_MAX for the slab outer outline; subordinate collection position
     * otherwise. This remains an ephemeral location, never feature identity. */
    size_t feature_index;
    size_t vertex_index;
    PlanPosition new_position;
} MoveSlabVertexCommand;

/* Constructor validates request shape only. Complete slab relationships remain
 * authoritative at execution time. */
int move_slab_vertex_command_create(DomainId slab_id, MoveSlabVertexTarget target,
    size_t feature_index, size_t vertex_index, PlanPosition new_position,
    MoveSlabVertexCommand *output);

int move_slab_vertex_command_execute(SiteHelperProject *project,
    const MoveSlabVertexCommand *command);

#endif
