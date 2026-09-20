#ifndef SITEHELPER_CAD_PLAN_MAPPING_H
#define SITEHELPER_CAD_PLAN_MAPPING_H

#include <stddef.h>

#include "cad_ir.h"
#include "position.h"

/* Priority 30D is the mapping boundary between source CAD values and canonical
 * SiteHelper Plan millimetres. It creates reference geometry only: no Project,
 * DomainId, Wall, Room, Slab, Roof, command or persistence types are involved. */

typedef enum {
    CAD_PLAN_LAYER_ALL = 0,
    CAD_PLAN_LAYER_INCLUDE,
    CAD_PLAN_LAYER_EXCLUDE
} CadPlanLayerFilterMode;

typedef struct {
    CadPlanLayerFilterMode mode;
    const char *const *layer_names;
    size_t layer_name_count;
} CadPlanLayerFilter;

typedef struct {
    int has_unit_override;
    CadIrSourceUnit unit_override;
    PlanPosition translation_mm;
    CadPlanLayerFilter layers;
} CadPlanMappingConfig;

typedef enum {
    CAD_PLAN_UNIT_RESOLUTION_NONE = 0,
    CAD_PLAN_UNIT_FROM_SOURCE,
    CAD_PLAN_UNIT_FROM_OVERRIDE
} CadPlanUnitResolution;

typedef enum {
    CAD_PLAN_REFERENCE_EMPTY = 0,
    CAD_PLAN_REFERENCE_READY,
    CAD_PLAN_REFERENCE_BLOCKED_UNITS
} CadPlanReferenceStatus;

typedef struct {
    PlanPosition *vertices;
    size_t vertex_count;
    int closed;
    CadIrProvenance provenance;
} CadPlanReferencePath;

/* Self-contained mapping proposal. Paths, diagnostics, metadata and provenance
 * are deep-owned so the source CadIrDocument may be destroyed after mapping. */
typedef struct {
    CadPlanReferenceStatus status;
    char *source_format;
    char *source_version;
    CadIrSourceUnit declared_unit;
    CadIrSourceUnit resolved_unit;
    CadPlanUnitResolution unit_resolution;
    int unit_override_conflicted;
    PlanPosition translation_mm;

    CadPlanReferencePath *paths;
    size_t path_count;
    size_t path_capacity;

    CadIrDiagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;

    size_t decoded_entity_count;
    size_t adapter_skipped_entity_count;
    size_t source_path_count;
    size_t filtered_path_count;
    size_t mapping_skipped_path_count;
} CadPlanReference;

typedef struct {
    size_t decoded_entity_count;
    size_t adapter_skipped_entity_count;
    size_t source_path_count;
    size_t emitted_path_count;
    size_t filtered_path_count;
    size_t mapping_skipped_path_count;
    size_t info_count;
    size_t warning_count;
    size_t error_count;
} CadPlanReferenceStatistics;

typedef enum {
    CAD_PLAN_MAP_SUCCESS = 0,
    CAD_PLAN_MAP_INVALID_ARGUMENT,
    CAD_PLAN_MAP_INVALID_SOURCE,
    CAD_PLAN_MAP_ALLOCATION_FAILED,
    CAD_PLAN_MAP_NUMERIC_OVERFLOW
} CadPlanMapCode;

void cad_plan_reference_init(CadPlanReference *reference);
void cad_plan_reference_destroy(CadPlanReference *reference);

/* Map a decoded source document into exact integer-mm reference geometry.
 *
 * output must be initialized (or zero-initialized). The operation is
 * transactional: technical failure leaves output unchanged. A globally
 * unresolved unit is a successfully produced BLOCKED_UNITS proposal containing
 * UNITS_REQUIRED and no paths, not a technical failure.
 *
 * Layer filtering is exact string matching against decoded layer names. Numeric
 * failures are entity-local: the affected path is omitted and diagnosed while
 * other paths may still map successfully. */
CadPlanMapCode cad_plan_map_reference(const CadIrDocument *source,
    const CadPlanMappingConfig *config, CadPlanReference *output);

CadPlanReferenceStatistics cad_plan_reference_statistics(
    const CadPlanReference *reference);

#endif
