#ifndef SITEHELPER_CAD_PLAN_EXPORT_H
#define SITEHELPER_CAD_PLAN_EXPORT_H

#include "cad_export_ir.h"
#include "sitehelper_project.h"

/* Stable logical layer names for the first Priority 30F plan-export policy. */
#define CAD_PLAN_EXPORT_LAYER_WALL_CENTERLINES "SITEHELPER_WALL_CENTERLINES"
#define CAD_PLAN_EXPORT_LAYER_ROOM_SEPARATORS "SITEHELPER_ROOM_SEPARATORS"
#define CAD_PLAN_EXPORT_LAYER_SLAB_OUTLINES "SITEHELPER_SLAB_OUTLINES"

typedef struct {
    int include_wall_centerlines;
    int include_room_separators;
    int include_slab_outlines;
} CadPlanExportConfig;

typedef struct {
    size_t wall_centerline_count;
    size_t room_separator_count;
    size_t slab_outline_count;
    size_t path_count;
} CadPlanExportStatistics;

typedef enum {
    CAD_PLAN_EXPORT_SUCCESS = 0,
    CAD_PLAN_EXPORT_INVALID_ARGUMENT,
    CAD_PLAN_EXPORT_STOREY_NOT_FOUND,
    CAD_PLAN_EXPORT_INVALID_PROJECT,
    CAD_PLAN_EXPORT_ALLOCATION_FAILED,
    CAD_PLAN_EXPORT_NUMERIC_OVERFLOW,
    CAD_PLAN_EXPORT_INTERNAL_ERROR
} CadPlanExportCode;

typedef struct {
    CadPlanExportCode code;
    CadPlanExportStatistics statistics;
} CadPlanExportResult;

/* Build a self-contained normalized Plan export for exactly one Storey.
 * Coordinates are unchanged canonical integer millimetres. The first policy
 * exports only the explicitly selected authoritative primitives above; it does
 * not include CAD background references, generated framing, wall faces,
 * openings, roof geometry, document annotations or subordinate slab features.
 *
 * output must be initialized/zero-initialized. On failure output is unchanged.
 * On success previous output contents are destroyed and replaced atomically. */
CadPlanExportResult cad_plan_export_storey(const SiteHelperProject *project,
    DomainId storey_id, const CadPlanExportConfig *config,
    CadExportDocument *output);

#endif
