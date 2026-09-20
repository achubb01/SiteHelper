#include "document_plan_query.h"

#include <float.h>
#include <math.h>

DomainId document_plan_find_note_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm)
{
    if (document == NULL || storey_id == DOMAIN_ID_INVALID ||
        !(tolerance_mm >= 0.0) || tolerance_mm > DBL_MAX) { return DOMAIN_ID_INVALID; }
    DomainId best = DOMAIN_ID_INVALID;
    double best_distance2 = tolerance_mm * tolerance_mm;
    if (best_distance2 > DBL_MAX) { best_distance2 = DBL_MAX; }
    for (size_t i = document->annotation_count; i > 0; i--) {
        const DocumentAnnotation *annotation = &document->annotations[i - 1];
        if (annotation->kind != DOCUMENT_ANNOTATION_NOTE ||
            annotation->anchor.storey_id != storey_id) { continue; }
        double dx = point.x - annotation->anchor.position.x;
        double dy = point.y - annotation->anchor.position.y;
        double distance2 = dx * dx + dy * dy;
        if (distance2 <= best_distance2 &&
            (best == DOMAIN_ID_INVALID || distance2 < best_distance2)) {
            best = annotation->id;
            best_distance2 = distance2;
        }
    }
    return best;
}


DomainId document_plan_find_symbol_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm)
{
    if (document == NULL || storey_id == DOMAIN_ID_INVALID ||
        !(tolerance_mm >= 0.0) || tolerance_mm > DBL_MAX) { return DOMAIN_ID_INVALID; }
    DomainId best=DOMAIN_ID_INVALID;
    double best_distance2=tolerance_mm*tolerance_mm;
    if (best_distance2 > DBL_MAX) { best_distance2=DBL_MAX; }
    for (size_t i=document->symbol_count;i>0;i--) {
        const DocumentPlanSymbol *symbol=&document->symbols[i-1];
        if ((symbol->kind != DOCUMENT_PLAN_SYMBOL_POINT_MARKER &&
             symbol->kind != DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION) ||
            symbol->storey_id != storey_id) { continue; }
        double dx=point.x-symbol->anchor.x,dy=point.y-symbol->anchor.y;
        double distance2=dx*dx+dy*dy;
        if (distance2 <= best_distance2 &&
            (best == DOMAIN_ID_INVALID || distance2 < best_distance2)) {
            best=symbol->id;
            best_distance2=distance2;
        }
    }
    return best;
}


static double segment_distance(PlanPosition a, PlanPosition b, PlanPoint point)
{
    double ax=a.x,ay=a.y,bx=b.x,by=b.y;
    double dx=bx-ax,dy=by-ay;
    double length2=dx*dx+dy*dy;
    if (!(length2 > 0.0) || !isfinite(length2)) { return DBL_MAX; }
    double t=((point.x-ax)*dx+(point.y-ay)*dy)/length2;
    if (t < 0.0) t=0.0;
    if (t > 1.0) t=1.0;
    double nx=ax+t*dx,ny=ay+t*dy;
    double distance=hypot(point.x-nx,point.y-ny);
    return isfinite(distance) ? distance : DBL_MAX;
}

DomainId document_plan_find_callout_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm)
{
    if (document == NULL || storey_id == DOMAIN_ID_INVALID ||
        !(tolerance_mm >= 0.0) || !isfinite(tolerance_mm) ||
        !isfinite(point.x) || !isfinite(point.y)) { return DOMAIN_ID_INVALID; }
    DomainId best=DOMAIN_ID_INVALID;
    double best_distance=tolerance_mm;
    for (size_t i=document->callout_count;i>0;i--) {
        const DocumentPlanCallout *callout=&document->callouts[i-1];
        if (callout->storey_id != storey_id) { continue; }
        double distance=segment_distance(callout->target,callout->label_anchor,point);
        if (distance <= best_distance &&
            (best == DOMAIN_ID_INVALID || distance < best_distance)) {
            best=callout->id;
            best_distance=distance;
        }
    }
    return best;
}


DomainId document_plan_find_revision_cloud_at_position(const DocumentModel *document,
    DomainId storey_id, PlanPoint point, double tolerance_mm)
{
    if (document == NULL || storey_id == DOMAIN_ID_INVALID ||
        !(tolerance_mm >= 0.0) || !isfinite(tolerance_mm) ||
        !isfinite(point.x) || !isfinite(point.y)) {
        return DOMAIN_ID_INVALID;
    }
    DomainId best = DOMAIN_ID_INVALID;
    double best_distance = tolerance_mm;
    for (size_t i = document->revision_cloud_count; i > 0; i--) {
        const DocumentPlanRevisionCloud *cloud = &document->revision_clouds[i - 1];
        if (cloud->storey_id != storey_id ||
            !document_plan_revision_cloud_is_locally_valid(cloud)) {
            continue;
        }
        double cloud_distance = DBL_MAX;
        for (size_t v = 0; v < cloud->vertex_count; v++) {
            PlanPosition a = cloud->vertices[v];
            PlanPosition b = cloud->vertices[(v + 1) % cloud->vertex_count];
            double distance = segment_distance(a, b, point);
            if (distance < cloud_distance) { cloud_distance = distance; }
        }
        if (cloud_distance <= best_distance &&
            (best == DOMAIN_ID_INVALID || cloud_distance < best_distance)) {
            best = cloud->id;
            best_distance = cloud_distance;
        }
    }
    return best;
}
