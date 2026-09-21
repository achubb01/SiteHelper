#include "wall_plan_geometry.h"

#include <float.h>
#include <math.h>

static void wall_face_offsets(WallPlanSpecification specification,
    double *left, double *right)
{
    double thickness=(double)specification.thickness_mm;
    switch (specification.alignment) {
        case WALL_PLAN_ALIGNMENT_LEFT_FACE:
            *left=0.0; *right=-thickness; break;
        case WALL_PLAN_ALIGNMENT_RIGHT_FACE:
            *left=thickness; *right=0.0; break;
        case WALL_PLAN_ALIGNMENT_CENTER:
        default:
            *left=thickness*0.5; *right=-thickness*0.5; break;
    }
}

int wall_plan_geometry_build(const WallDefinition *definition,
    WallPlanGeometry *geometry)
{
    if (definition == NULL || geometry == NULL ||
        !wall_plan_specification_valid(definition->plan_specification)) return 0;
    WallPlanSegment segment=definition->segment;
    int length=wall_plan_segment_length_mm(segment);
    if (length <= 0) return 0;
    double dx=(double)segment.end.x-(double)segment.start.x;
    double dy=(double)segment.end.y-(double)segment.start.y;
    double nx=-dy/(double)length, ny=dx/(double)length;
    double left_offset,right_offset;
    wall_face_offsets(definition->plan_specification,&left_offset,&right_offset);
    PlanPoint ls={(double)segment.start.x+nx*left_offset,
                  (double)segment.start.y+ny*left_offset};
    PlanPoint le={(double)segment.end.x+nx*left_offset,
                  (double)segment.end.y+ny*left_offset};
    PlanPoint rs={(double)segment.start.x+nx*right_offset,
                  (double)segment.start.y+ny*right_offset};
    PlanPoint re={(double)segment.end.x+nx*right_offset,
                  (double)segment.end.y+ny*right_offset};
    *geometry=(WallPlanGeometry){
        .corners={ls,rs,re,le},
        .left_start=ls,.left_end=le,.right_start=rs,.right_end=re
    };
    return 1;
}

static double segment_distance(PlanPoint a, PlanPoint b, PlanPoint p)
{
    double dx=b.x-a.x,dy=b.y-a.y;
    double denom=dx*dx+dy*dy;
    if (denom <= 0.0) return DBL_MAX;
    double t=((p.x-a.x)*dx+(p.y-a.y)*dy)/denom;
    if (t<0.0)t=0.0; else if(t>1.0)t=1.0;
    return hypot(p.x-(a.x+t*dx),p.y-(a.y+t*dy));
}

static double cross(PlanPoint a,PlanPoint b,PlanPoint p)
{
    return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);
}

double wall_plan_geometry_distance(const WallPlanGeometry *geometry,
    PlanPoint point)
{
    if (geometry == NULL || !isfinite(point.x) || !isfinite(point.y)) return DBL_MAX;
    int positive=0,negative=0;
    double best=DBL_MAX;
    for (size_t i=0;i<4;i++) {
        PlanPoint a=geometry->corners[i],b=geometry->corners[(i+1)%4];
        double c=cross(a,b,point);
        if (c>1e-9) positive=1; else if(c<-1e-9) negative=1;
        double d=segment_distance(a,b,point); if(d<best)best=d;
    }
    return positive && negative ? best : 0.0;
}
