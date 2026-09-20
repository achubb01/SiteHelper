#include "plan_dimension_geometry.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

static double segment_distance(PlanPoint a, PlanPoint b, PlanPoint p)
{
    const double dx=b.x-a.x, dy=b.y-a.y;
    const double len2=dx*dx+dy*dy;
    if (!(len2 > 0.0) || !isfinite(len2)) {
        const double ex=p.x-a.x, ey=p.y-a.y;
        return hypot(ex,ey);
    }
    double t=((p.x-a.x)*dx+(p.y-a.y)*dy)/len2;
    if (t < 0.0) t=0.0;
    else if (t > 1.0) t=1.0;
    return hypot(p.x-(a.x+t*dx),p.y-(a.y+t*dy));
}

int document_plan_dimension_geometry(PlanPosition first, PlanPosition second,
    int offset_mm, DocumentPlanDimensionGeometry *output)
{
    if (output == NULL) return 0;
    const double ax=first.x, ay=first.y, bx=second.x, by=second.y;
    const double dx=bx-ax, dy=by-ay;
    const double length=hypot(dx,dy);
    if (!(length > 0.0) || !isfinite(length)) return 0;
    const double ox=(-dy/length)*(double)offset_mm;
    const double oy=( dx/length)*(double)offset_mm;
    DocumentPlanDimensionGeometry result={
        .source_first={ax,ay}, .source_second={bx,by},
        .line_first={ax+ox,ay+oy}, .line_second={bx+ox,by+oy},
        .midpoint={(ax+bx)*0.5+ox,(ay+by)*0.5+oy}
    };
    if (!isfinite(result.line_first.x)||!isfinite(result.line_first.y)||
        !isfinite(result.line_second.x)||!isfinite(result.line_second.y)||
        !isfinite(result.midpoint.x)||!isfinite(result.midpoint.y)) return 0;
    *output=result;
    return 1;
}

double document_plan_dimension_hit_distance(const DocumentPlanDimensionGeometry *geometry,
    PlanPoint point)
{
    if (geometry == NULL || !isfinite(point.x) || !isfinite(point.y)) return -1.0;
    double best=segment_distance(geometry->line_first,geometry->line_second,point);
    double d=segment_distance(geometry->source_first,geometry->line_first,point);
    if (d < best) best=d;
    d=segment_distance(geometry->source_second,geometry->line_second,point);
    if (d < best) best=d;
    return isfinite(best) && best <= DBL_MAX ? best : -1.0;
}
