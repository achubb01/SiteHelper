#ifndef CREATE_SLAB_COMMAND_H
#define CREATE_SLAB_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId storey_id;
    PlanPosition *vertices;
    size_t vertex_count;
    int thickness_mm;
    int top_level_offset_mm;
} CreateSlabCommand;

/* Owns a deep outline copy. Output is zero or a previous valid command;
 * failure leaves it unchanged and success replaces it. */
int create_slab_command_create(DomainId storey_id, const PlanPosition *vertices,
    size_t vertex_count, int thickness_mm, int top_level_offset_mm,
    CreateSlabCommand *command);
int create_slab_command_clone(const CreateSlabCommand *source, CreateSlabCommand *output);
void create_slab_command_destroy(CreateSlabCommand *command);
int create_slab_command_execute(SiteHelperProject *project,
    const CreateSlabCommand *command, DomainId *slab_id);
int create_slab_command_undo(SiteHelperProject *project,
    const CreateSlabCommand *command, DomainId slab_id);
int create_slab_command_redo(SiteHelperProject *project,
    const CreateSlabCommand *command, DomainId slab_id);

#endif
