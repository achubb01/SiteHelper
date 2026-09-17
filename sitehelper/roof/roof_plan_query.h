#ifndef ROOF_PLAN_QUERY_H
#define ROOF_PLAN_QUERY_H

#include "storey.h"

typedef enum {
    ROOF_PLAN_HIT_NONE = 0,
    ROOF_PLAN_HIT_ROOF,
    ROOF_PLAN_HIT_PORTION
} RoofPlanHitKind;

typedef struct {
    RoofPlanHitKind kind;
    DomainId roof_id;
    DomainId portion_id;
} RoofPlanHit;

/* Authoring/source hit test for the active Storey Plan view. This intentionally
 * queries authoritative support polygons, not generated roof planes. Later
 * appended roofs and portions win overlaps. tolerance_mm expands polygon edges
 * for practical pointer selection; a point inside needs no tolerance. */
RoofPlanHit roof_plan_hit_test_storey(const Storey *storey, PlanPoint point,
    double tolerance_mm);

#endif
