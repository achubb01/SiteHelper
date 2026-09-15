#ifndef EDIT_SLAB_COMMAND_H
#define EDIT_SLAB_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId slab_id;
    int thickness_mm;
    int top_level_offset_mm;
} EditSlabCommand;

typedef struct {
    DomainId slab_id;
    size_t feature_index;
    int top_level_offset_mm;
    int thickness_mm;
} EditSlabRegionCommand;

typedef struct {
    DomainId slab_id;
    size_t feature_index;
    size_t edge_index; /* Read-only host edge copied into the proposed value. */
    int start_offset_mm;
    int end_offset_mm;
    int width_mm;
    int depth_mm;
} EditSlabEdgeRebateCommand;

/* Constructors validate identity/shape of the request only. Context-dependent
 * Slab rules are authoritative at execute time. */
int edit_slab_command_create(DomainId slab_id, int thickness_mm,
    int top_level_offset_mm, EditSlabCommand *output);
int edit_slab_region_command_create(DomainId slab_id, size_t feature_index,
    int top_level_offset_mm, int thickness_mm, EditSlabRegionCommand *output);
int edit_slab_edge_rebate_command_create(DomainId slab_id, size_t feature_index,
    size_t edge_index, int start_offset_mm, int end_offset_mm, int width_mm,
    int depth_mm, EditSlabEdgeRebateCommand *output);

int edit_slab_command_execute(SiteHelperProject *project,
    const EditSlabCommand *command);
int edit_slab_region_command_execute(SiteHelperProject *project,
    const EditSlabRegionCommand *command);
int edit_slab_edge_rebate_command_execute(SiteHelperProject *project,
    const EditSlabEdgeRebateCommand *command);

#endif
