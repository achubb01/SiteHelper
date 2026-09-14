#ifndef SLAB_PLAN_QUERY_H
#define SLAB_PLAN_QUERY_H

#include <stddef.h>

#include "position.h"
#include "slab.h"
#include "storey.h"

typedef enum {
    SLAB_PLAN_HIT_NONE = 0,
    SLAB_PLAN_HIT_SLAB,
    SLAB_PLAN_HIT_REGION,
    SLAB_PLAN_HIT_PENETRATION,
    SLAB_PLAN_HIT_EDGE_REBATE
} SlabPlanHitKind;

/* Value-only editor/query result. feature_index is SIZE_MAX for NONE/SLAB.
 * Subordinate indices are ephemeral positions in the owning Slab collection,
 * never domain identity. */
typedef struct {
    SlabPlanHitKind kind;
    DomainId slab_id;
    size_t feature_index;
} SlabPlanHit;

SlabPlanHit slab_plan_hit_none(void);

/* Derives the exact host vertices at U=0/U=L and linearly interpolates interior
 * integer-mm U values. Outputs remain unchanged if the slab/rebate is invalid. */
int slab_plan_rebate_endpoints(const SlabDefinition *definition,
    const SlabEdgeRebate *rebate, PlanPoint *start, PlanPoint *end);

/* Read-only current-Storey scan. Invalid slabs are ignored safely. Tolerance is
 * in plan-world millimetres and applies to visible boundaries/rebate segments.
 * Precedence is rebate, penetration, region, slab. Equal-precedence overlaps
 * choose the later Storey collection item, matching render stacking. */
SlabPlanHit slab_plan_hit_test_storey(const Storey *storey, PlanPoint point,
    double tolerance_mm);

#endif
