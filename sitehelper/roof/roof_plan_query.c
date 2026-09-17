#include "roof_plan_query.h"

#include <math.h>

#include "roof.h"

static double segment_distance(PlanPoint p, PlanPosition a, PlanPosition b)
{
    double ax=(double)a.x, ay=(double)a.y;
    double bx=(double)b.x, by=(double)b.y;
    double dx=bx-ax, dy=by-ay;
    double denom=dx*dx+dy*dy;
    if (denom == 0.0) { return hypot(p.x-ax,p.y-ay); }
    double t=((p.x-ax)*dx+(p.y-ay)*dy)/denom;
    if (t < 0.0) t=0.0;
    if (t > 1.0) t=1.0;
    return hypot(p.x-(ax+t*dx),p.y-(ay+t*dy));
}

static int point_in_or_near_polygon(const PlanPosition *v, size_t n,
    PlanPoint p, double tolerance)
{
    if (v == NULL || n < 3 || !isfinite(p.x) || !isfinite(p.y) ||
        !isfinite(tolerance) || tolerance < 0.0) { return 0; }
    int inside=0;
    for (size_t i=0,j=n-1;i<n;j=i++) {
        if (segment_distance(p,v[j],v[i]) <= tolerance) { return 1; }
        double yi=(double)v[i].y, yj=(double)v[j].y;
        double xi=(double)v[i].x, xj=(double)v[j].x;
        int crosses=((yi>p.y)!=(yj>p.y));
        if (crosses) {
            double x=xi+(p.y-yi)*(xj-xi)/(yj-yi);
            if (p.x < x) { inside=!inside; }
        }
    }
    return inside;
}

RoofPlanHit roof_plan_hit_test_storey(const Storey *storey, PlanPoint point,
    double tolerance_mm)
{
    RoofPlanHit none={ROOF_PLAN_HIT_NONE,DOMAIN_ID_INVALID,DOMAIN_ID_INVALID};
    if (storey == NULL || !isfinite(point.x) || !isfinite(point.y) ||
        !isfinite(tolerance_mm) || tolerance_mm < 0.0) { return none; }
    const RoofCollection *collection=&storey->roofs;
    if (collection->count > collection->capacity ||
        (collection->capacity == 0 ? collection->items != NULL : collection->items == NULL)) {
        return none;
    }
    for (size_t r=collection->count;r>0;r--) {
        const Roof *roof=&collection->items[r-1];
        if (roof_validate(roof) != ROOF_SUCCESS) { continue; }
        const RoofDefinition *d=&roof->definition;
        for (size_t p=d->portion_count;p>0;p--) {
            const RoofPortionDefinition *portion=&d->portions[p-1];
            if (point_in_or_near_polygon(portion->support_vertices,
                    portion->support_vertex_count,point,tolerance_mm)) {
                return (RoofPlanHit){ROOF_PLAN_HIT_PORTION,roof->id,portion->id};
            }
        }
    }
    return none;
}
