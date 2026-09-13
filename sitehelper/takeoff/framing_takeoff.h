#ifndef FRAMING_TAKEOFF_H
#define FRAMING_TAKEOFF_H

#include <stdint.h>
#include "sitehelper_project.h"

typedef struct {
    TimberType type;
    /* Applicable only to TIMBER_STUD; otherwise canonical STUD_COMMON,
     * meaning not applicable, and excluded from the aggregation key. */
    StudType stud_type;
    int length_mm, depth_mm, width_mm;
    uint64_t quantity;
    int64_t total_length_mm;
} FramingTakeoffItem;

/* Owned derived snapshot, with no identity or borrowed model pointers.
 * Items are sorted ascending lexicographically by type (enum value),
 * stud_type (studs only), length_mm, depth_mm, width_mm. Identical keys
 * aggregate. Do not shallow-copy this structure into another owner. */
typedef struct {
    FramingTakeoffItem *items;
    size_t item_count;
} FramingTakeoff;

typedef enum {
    FRAMING_TAKEOFF_SUCCESS = 0,
    FRAMING_TAKEOFF_INVALID_ARGUMENT,
    FRAMING_TAKEOFF_INVALID_SOURCE, /* Storey/Wall collection metadata. */
    FRAMING_TAKEOFF_INVALID_FRAMING, /* Missing or incoherent framing. */
    FRAMING_TAKEOFF_ALLOCATION_FAILED,
    FRAMING_TAKEOFF_NUMERIC_OVERFLOW
} FramingTakeoffCode;

typedef struct {
    FramingTakeoffCode code;
    DomainId wall_id; /* Offending Wall when known, otherwise zero. */
} FramingTakeoffResult;

/* Read only committed framing, never definitions, openings or settings.
 * Output must be zero-initialized or a previous successful result, independent
 * of input. Success replaces it atomically; every failure preserves it entirely.
 * Empty Storey/Project succeeds. Every Wall requires two plates, at least two
 * studs, coherent array metadata, positive dimensions and supported types.
 * Noggins/additional members may be empty. This is a narrow consumption check,
 * not a geometry validator or a freshness check against authoritative state.
 * Input collections must own valid, disjoint allocations; metadata checks
 * cannot establish actual allocation sizes or arbitrary pointer validity.
 * Callers must prevent concurrent input mutation during a query.
 * On failure an older output still describes its earlier framing snapshot. */
FramingTakeoffResult framing_takeoff_build_wall(const Wall *wall, FramingTakeoff *output);
FramingTakeoffResult framing_takeoff_build_storey(const Storey *storey, FramingTakeoff *output);
FramingTakeoffResult framing_takeoff_build_project(const SiteHelperProject *project, FramingTakeoff *output);

/* Frees owned storage and zeros the result. NULL/zero/repeated calls safe.
 * A snapshot can outlive its source; its pointers expire on rebuild/destroy. */
void framing_takeoff_destroy(FramingTakeoff *takeoff);

#endif
