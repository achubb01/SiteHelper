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

typedef struct {
    int has_edge;
    DomainId slab_id;
    size_t edge_index;
    int u_mm;
    double distance_mm;
    PlanPoint projected_point;
} SlabPlanEdgeHit;

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

/* UI query for the nearest current-Storey exterior edge. Projection clamps to
 * the segment. U uses the authoritative rounded edge length and nearest-mm
 * rounding (half upward); exact endpoints map to 0/L. Exact distance ties use
 * later slab/edge render order. Invalid slabs are ignored. */
SlabPlanEdgeHit slab_plan_find_outer_edge(const Storey *storey,
    PlanPoint point, double tolerance_mm);
/* Projects onto one specified valid exterior edge, used after a rebate tool has
 * locked its host. Returns no edge outside tolerance. */
SlabPlanEdgeHit slab_plan_project_outer_edge(const Slab *slab,
    size_t edge_index, PlanPoint point, double tolerance_mm);

#endif
