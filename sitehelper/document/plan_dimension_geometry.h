#ifndef PLAN_DIMENSION_GEOMETRY_H
#define PLAN_DIMENSION_GEOMETRY_H

#include "position.h"

typedef struct
{
    PlanPoint source_first;
    PlanPoint source_second;
    PlanPoint line_first;
    PlanPoint line_second;
    PlanPoint midpoint;
} DocumentPlanDimensionGeometry;

/* Pure drafting geometry derived from resolved authoritative endpoints. The
 * signed offset is perpendicular to first->second in Plan world millimetres. */
int document_plan_dimension_geometry(PlanPosition first, PlanPosition second,
    int offset_mm, DocumentPlanDimensionGeometry *output);

/* Minimum distance in Plan world millimetres to either extension line or the
 * dimension line. Returns a negative value for invalid geometry/input. */
double document_plan_dimension_hit_distance(const DocumentPlanDimensionGeometry *geometry,
    PlanPoint point);

#endif
