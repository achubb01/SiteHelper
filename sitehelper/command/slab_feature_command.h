#ifndef SLAB_FEATURE_COMMAND_H
#define SLAB_FEATURE_COMMAND_H

#include "sitehelper_project.h"

typedef struct {
    DomainId slab_id;
    PlanPosition *vertices;
    size_t vertex_count;
} AddSlabPenetrationCommand;

typedef struct {
    DomainId slab_id;
    size_t feature_index;
} DeleteSlabPenetrationCommand;

typedef struct {
    DomainId slab_id;
    PlanPosition *vertices;
    size_t vertex_count;
    int top_level_offset_mm;
    int thickness_mm;
} AddSlabRegionCommand;

typedef struct {
    DomainId slab_id;
    size_t feature_index;
} DeleteSlabRegionCommand;

typedef struct {
    DomainId slab_id;
    size_t edge_index;
    int start_offset_mm;
    int end_offset_mm;
    int width_mm;
    int depth_mm;
} AddSlabEdgeRebateCommand;

typedef struct {
    DomainId slab_id;
    size_t feature_index;
} DeleteSlabEdgeRebateCommand;

/* Polygon ADD commands own independent vertex arrays. Outputs may be zero or a
 * previous valid owned command; success replaces them and failure preserves
 * them. Context-dependent geometry is validated when the command executes. */
int add_slab_penetration_command_create(DomainId slab_id,
    const PlanPosition *vertices, size_t vertex_count,
    AddSlabPenetrationCommand *output);
int add_slab_penetration_command_clone(const AddSlabPenetrationCommand *source,
    AddSlabPenetrationCommand *output);
void add_slab_penetration_command_destroy(AddSlabPenetrationCommand *command);

int add_slab_region_command_create(DomainId slab_id,
    const PlanPosition *vertices, size_t vertex_count,
    int top_level_offset_mm, int thickness_mm, AddSlabRegionCommand *output);
int add_slab_region_command_clone(const AddSlabRegionCommand *source,
    AddSlabRegionCommand *output);
void add_slab_region_command_destroy(AddSlabRegionCommand *command);

int add_slab_edge_rebate_command_create(DomainId slab_id, size_t edge_index,
    int start_offset_mm, int end_offset_mm, int width_mm, int depth_mm,
    AddSlabEdgeRebateCommand *output);
int delete_slab_penetration_command_create(DomainId slab_id,
    size_t feature_index, DeleteSlabPenetrationCommand *output);
int delete_slab_region_command_create(DomainId slab_id,
    size_t feature_index, DeleteSlabRegionCommand *output);
int delete_slab_edge_rebate_command_create(DomainId slab_id,
    size_t feature_index, DeleteSlabEdgeRebateCommand *output);

/* Execute appends and reports the appended source-order index. DELETE execute
 * removes by ephemeral position; history captures and verifies exact snapshots. */
int add_slab_penetration_command_execute(SiteHelperProject *project,
    const AddSlabPenetrationCommand *command, size_t *feature_index);
int add_slab_region_command_execute(SiteHelperProject *project,
    const AddSlabRegionCommand *command, size_t *feature_index);
int add_slab_edge_rebate_command_execute(SiteHelperProject *project,
    const AddSlabEdgeRebateCommand *command, size_t *feature_index);
int delete_slab_penetration_command_execute(SiteHelperProject *project,
    const DeleteSlabPenetrationCommand *command);
int delete_slab_region_command_execute(SiteHelperProject *project,
    const DeleteSlabRegionCommand *command);
int delete_slab_edge_rebate_command_execute(SiteHelperProject *project,
    const DeleteSlabEdgeRebateCommand *command);

#endif
