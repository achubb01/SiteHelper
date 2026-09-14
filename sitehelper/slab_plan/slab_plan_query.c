#include "slab_plan_query.h"

#include <math.h>

typedef enum {
    PLAN_POINT_OUTSIDE,
    PLAN_POINT_INSIDE,
    PLAN_POINT_NEAR_BOUNDARY
} PlanPointLocation;

SlabPlanHit slab_plan_hit_none(void)
{
    return (SlabPlanHit){
        .kind = SLAB_PLAN_HIT_NONE,
        .slab_id = DOMAIN_ID_INVALID,
        .feature_index = SIZE_MAX
    };
}

static int coherent_slab_collection(const SlabCollection *collection)
{
    return collection != NULL && collection->count <= collection->capacity &&
        ((collection->capacity == 0 && collection->items == NULL) ||
         (collection->capacity != 0 && collection->items != NULL));
}

static double segment_distance(PlanPoint point, PlanPosition a, PlanPosition b)
{
    double ax = (double)a.x, ay = (double)a.y;
    double dx = (double)b.x - ax, dy = (double)b.y - ay;
    double length2 = dx * dx + dy * dy;
    if (!isfinite(length2) || length2 <= 0.0) { return INFINITY; }
    double t = ((point.x - ax) * dx + (point.y - ay) * dy) / length2;
    if (t < 0.0) { t = 0.0; }
    else if (t > 1.0) { t = 1.0; }
    return hypot(point.x - (ax + t * dx), point.y - (ay + t * dy));
}

static SlabPlanEdgeHit edge_hit_none(void)
{
    return (SlabPlanEdgeHit){.slab_id=DOMAIN_ID_INVALID,.edge_index=SIZE_MAX};
}

static SlabPlanEdgeHit project_valid_edge(const Slab *slab, size_t edge_index,
    PlanPoint point, double tolerance)
{
    SlabPlanEdgeHit none=edge_hit_none();
    if (slab == NULL || !isfinite(point.x) || !isfinite(point.y) || !isfinite(tolerance) ||
        tolerance < 0.0 || edge_index >= slab->definition.outline.vertex_count) {
        return none;
    }
    const SlabOutline *outline=&slab->definition.outline;
    size_t next=edge_index+1 == outline->vertex_count ? 0 : edge_index+1;
    PlanPosition a=outline->vertices[edge_index],b=outline->vertices[next];
    double dx=(double)b.x-a.x,dy=(double)b.y-a.y;
    double length2=dx*dx+dy*dy;
    if (!isfinite(length2) || length2 <= 0.0) { return none; }
    double t=((point.x-a.x)*dx+(point.y-a.y)*dy)/length2;
    if (t < 0.0) { t=0.0; }
    else if (t > 1.0) { t=1.0; }
    PlanPoint projected={(double)a.x+t*dx,(double)a.y+t*dy};
    double distance=hypot(point.x-projected.x,point.y-projected.y);
    int length_mm;
    if (!isfinite(distance) || distance > tolerance ||
        slab_edge_local_length_mm(&slab->definition,edge_index,&length_mm) != SLAB_SUCCESS) {
        return none;
    }
    int u_mm;
    if (t <= 0.0) { u_mm=0; projected=(PlanPoint){a.x,a.y}; }
    else if (t >= 1.0) { u_mm=length_mm; projected=(PlanPoint){b.x,b.y}; }
    else {
        double local=t*length_mm;
        if (!isfinite(local) || local < 0.0 || local > length_mm) { return none; }
        u_mm=(int)floor(local+0.5);
        if (u_mm < 0) { u_mm=0; }
        else if (u_mm > length_mm) { u_mm=length_mm; }
        /* Preview the same interval that the authoritative integer U denotes,
         * rather than retaining the sub-millimetre cursor projection. */
        double authoritative_t=(double)u_mm/length_mm;
        projected=(PlanPoint){(double)a.x+authoritative_t*dx,
            (double)a.y+authoritative_t*dy};
    }
    return (SlabPlanEdgeHit){1,slab->id,edge_index,u_mm,distance,projected};
}

SlabPlanEdgeHit slab_plan_project_outer_edge(const Slab *slab,
    size_t edge_index, PlanPoint point, double tolerance_mm)
{
    if (slab == NULL || slab_validate(slab) != SLAB_SUCCESS) {
        return edge_hit_none();
    }
    return project_valid_edge(slab,edge_index,point,tolerance_mm);
}

SlabPlanEdgeHit slab_plan_find_outer_edge(const Storey *storey,
    PlanPoint point, double tolerance_mm)
{
    SlabPlanEdgeHit best=edge_hit_none();
    if (storey == NULL || !coherent_slab_collection(&storey->slabs) ||
        !isfinite(point.x) || !isfinite(point.y) ||
        !isfinite(tolerance_mm) || tolerance_mm < 0.0) { return best; }
    for (size_t s=0;s<storey->slabs.count;s++) {
        const Slab *slab=&storey->slabs.items[s];
        if (slab_validate(slab) != SLAB_SUCCESS) { continue; }
        for (size_t edge=0;edge<slab->definition.outline.vertex_count;edge++) {
            SlabPlanEdgeHit candidate=project_valid_edge(slab,edge,point,tolerance_mm);
            if (candidate.has_edge && (!best.has_edge ||
                candidate.distance_mm <= best.distance_mm)) { best=candidate; }
        }
    }
    return best;
}

/* UI-space predicate only: authoritative validity remains in sitehelper_slab's
 * checked integer predicates. The adapter first requires slab_validate(). */
static PlanPointLocation outline_point_location(const SlabOutline *outline,
    PlanPoint point, double tolerance)
{
    int inside = 0;
    for (size_t i = 0, j = outline->vertex_count - 1;
         i < outline->vertex_count; j = i++) {
        PlanPosition a = outline->vertices[j], b = outline->vertices[i];
        if (segment_distance(point, a, b) <= tolerance) {
            return PLAN_POINT_NEAR_BOUNDARY;
        }
        double ay = (double)a.y, by = (double)b.y;
        if ((ay > point.y) != (by > point.y)) {
            double crossing_x = (double)a.x +
                (point.y - ay) * ((double)b.x - a.x) / (by - ay);
            if (point.x < crossing_x) { inside = !inside; }
        }
    }
    return inside ? PLAN_POINT_INSIDE : PLAN_POINT_OUTSIDE;
}

static void consider_hit(SlabPlanHit *best, SlabPlanHitKind kind,
    DomainId slab_id, size_t feature_index)
{
    int rank = kind == SLAB_PLAN_HIT_EDGE_REBATE ? 4 :
        kind == SLAB_PLAN_HIT_PENETRATION ? 3 :
        kind == SLAB_PLAN_HIT_REGION ? 2 :
        kind == SLAB_PLAN_HIT_SLAB ? 1 : 0;
    int best_rank = best->kind == SLAB_PLAN_HIT_EDGE_REBATE ? 4 :
        best->kind == SLAB_PLAN_HIT_PENETRATION ? 3 :
        best->kind == SLAB_PLAN_HIT_REGION ? 2 :
        best->kind == SLAB_PLAN_HIT_SLAB ? 1 : 0;
    if (rank >= best_rank) {
        *best = (SlabPlanHit){kind, slab_id, feature_index};
    }
}

int slab_plan_rebate_endpoints(const SlabDefinition *definition,
    const SlabEdgeRebate *rebate, PlanPoint *start, PlanPoint *end)
{
    if (definition == NULL || rebate == NULL || start == NULL || end == NULL ||
        slab_definition_validate(definition) != SLAB_SUCCESS ||
        slab_edge_rebate_validate(definition, rebate) != SLAB_SUCCESS) {
        return 0;
    }
    int local_length;
    if (slab_edge_local_length_mm(definition, rebate->edge_index,
            &local_length) != SLAB_SUCCESS) {
        return 0;
    }
    const SlabOutline *outline = &definition->outline;
    PlanPosition a = outline->vertices[rebate->edge_index];
    size_t next = rebate->edge_index + 1 == outline->vertex_count ? 0 : rebate->edge_index + 1;
    PlanPosition b = outline->vertices[next];
    PlanPoint candidate_start, candidate_end;
    if (rebate->start_offset_mm == 0) {
        candidate_start = (PlanPoint){a.x, a.y};
    } else {
        double t = (double)rebate->start_offset_mm / local_length;
        candidate_start = (PlanPoint){
            (double)a.x + t * ((double)b.x - a.x),
            (double)a.y + t * ((double)b.y - a.y)
        };
    }
    if (rebate->end_offset_mm == local_length) {
        candidate_end = (PlanPoint){b.x, b.y};
    } else {
        double t = (double)rebate->end_offset_mm / local_length;
        candidate_end = (PlanPoint){
            (double)a.x + t * ((double)b.x - a.x),
            (double)a.y + t * ((double)b.y - a.y)
        };
    }
    if (!isfinite(candidate_start.x) || !isfinite(candidate_start.y) ||
        !isfinite(candidate_end.x) || !isfinite(candidate_end.y)) {
        return 0;
    }
    *start = candidate_start;
    *end = candidate_end;
    return 1;
}

static double point_segment_distance(PlanPoint point, PlanPoint a, PlanPoint b)
{
    double dx = b.x - a.x, dy = b.y - a.y;
    double length2 = dx * dx + dy * dy;
    if (!isfinite(length2) || length2 <= 0.0) { return INFINITY; }
    double t = ((point.x - a.x) * dx + (point.y - a.y) * dy) / length2;
    if (t < 0.0) { t = 0.0; }
    else if (t > 1.0) { t = 1.0; }
    return hypot(point.x - (a.x + t * dx), point.y - (a.y + t * dy));
}

SlabPlanHit slab_plan_hit_test_storey(const Storey *storey, PlanPoint point,
    double tolerance_mm)
{
    SlabPlanHit best = slab_plan_hit_none();
    if (storey == NULL || !coherent_slab_collection(&storey->slabs) ||
        !isfinite(point.x) || !isfinite(point.y) ||
        !isfinite(tolerance_mm) || tolerance_mm < 0.0) {
        return best;
    }
    for (size_t slab_index = 0; slab_index < storey->slabs.count; slab_index++) {
        const Slab *slab = &storey->slabs.items[slab_index];
        if (slab_validate(slab) != SLAB_SUCCESS) { continue; }
        const SlabDefinition *definition = &slab->definition;
        PlanPointLocation outer = outline_point_location(
            &definition->outline, point, tolerance_mm);
        if (outer != PLAN_POINT_OUTSIDE) {
            consider_hit(&best, SLAB_PLAN_HIT_SLAB, slab->id, SIZE_MAX);
        }
        for (size_t i = 0; i < definition->regions.count; i++) {
            if (outline_point_location(&definition->regions.items[i].outline,
                    point, tolerance_mm) != PLAN_POINT_OUTSIDE) {
                consider_hit(&best, SLAB_PLAN_HIT_REGION, slab->id, i);
            }
        }
        for (size_t i = 0; i < definition->penetrations.count; i++) {
            if (outline_point_location(&definition->penetrations.items[i].outline,
                    point, tolerance_mm) != PLAN_POINT_OUTSIDE) {
                consider_hit(&best, SLAB_PLAN_HIT_PENETRATION, slab->id, i);
            }
        }
        for (size_t i = 0; i < definition->edge_rebates.count; i++) {
            PlanPoint start, end;
            if (slab_plan_rebate_endpoints(definition,
                    &definition->edge_rebates.items[i], &start, &end) &&
                point_segment_distance(point, start, end) <= tolerance_mm) {
                consider_hit(&best, SLAB_PLAN_HIT_EDGE_REBATE, slab->id, i);
            }
        }
    }
    return best;
}
