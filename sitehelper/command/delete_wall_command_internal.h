#ifndef DELETE_WALL_COMMAND_INTERNAL_H
#define DELETE_WALL_COMMAND_INTERNAL_H

#include "delete_wall_command.h"

/* Exclusively owned authoritative copies; no borrowed project storage/framing.
 * The snapshot survives both undo and redo. */
typedef struct
{
    DomainId storey_id;
    DomainId wall_id;
    WallPlanSegment segment;
    WallPlanSpecification plan_specification;
    Opening *openings;
    size_t opening_count;
} DeletedWallSnapshot;

/* Output must be zero initialized. Failure releases partial allocations. */
int deleted_wall_snapshot_capture(const SiteHelperProject *project,
    const DeleteWallCommand *command, DeletedWallSnapshot *snapshot);
void deleted_wall_snapshot_destroy(DeletedWallSnapshot *snapshot);
int deleted_wall_snapshot_restore(SiteHelperProject *project,
    const DeletedWallSnapshot *snapshot);

#endif
