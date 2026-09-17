#ifndef DELETE_ROOF_COMMAND_INTERNAL_H
#define DELETE_ROOF_COMMAND_INTERNAL_H

#include "delete_roof_command.h"

typedef struct {
    DomainId storey_id;
    size_t index;
    Roof roof;
} DeletedRoofSnapshot;

int deleted_roof_snapshot_capture(const SiteHelperProject *project,
    const DeleteRoofCommand *command, DeletedRoofSnapshot *snapshot);
void deleted_roof_snapshot_destroy(DeletedRoofSnapshot *snapshot);
int deleted_roof_snapshot_restore(SiteHelperProject *project,
    const DeletedRoofSnapshot *snapshot);

#endif
