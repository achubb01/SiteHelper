#include "cad_plan_export.h"

#include <stdint.h>
#include <stdlib.h>

static CadPlanExportResult export_result(CadPlanExportCode code,
    CadPlanExportStatistics statistics)
{
    CadPlanExportResult result = {code, statistics};
    return result;
}

static CadPlanExportCode map_ir_code(CadExportIrCode code)
{
    switch (code) {
    case CAD_EXPORT_IR_SUCCESS: return CAD_PLAN_EXPORT_SUCCESS;
    case CAD_EXPORT_IR_ALLOCATION_FAILED: return CAD_PLAN_EXPORT_ALLOCATION_FAILED;
    case CAD_EXPORT_IR_NUMERIC_OVERFLOW: return CAD_PLAN_EXPORT_NUMERIC_OVERFLOW;
    case CAD_EXPORT_IR_INVALID_ARGUMENT:
    case CAD_EXPORT_IR_INVALID_STATE:
    case CAD_EXPORT_IR_INVALID_PATH:
    default: return CAD_PLAN_EXPORT_INTERNAL_ERROR;
    }
}

static CadPlanExportCode append_pair(CadExportDocument *document,
    PlanPosition start, PlanPosition end, const char *layer)
{
    CadExportPoint2 vertices[2] = {
        {(int64_t)start.x, (int64_t)start.y},
        {(int64_t)end.x, (int64_t)end.y}
    };
    CadExportPathInput input = {vertices, 2, 0, layer};
    return map_ir_code(cad_export_document_append_path(document, &input));
}

static CadPlanExportCode append_outline(CadExportDocument *document,
    const SlabOutline *outline, const char *layer)
{
    if (outline == NULL || outline->vertices == NULL || outline->vertex_count < 3 ||
        outline->vertex_count > SIZE_MAX / sizeof(CadExportPoint2)) {
        return CAD_PLAN_EXPORT_INTERNAL_ERROR;
    }
    CadExportPoint2 *vertices = malloc(outline->vertex_count * sizeof *vertices);
    if (vertices == NULL) { return CAD_PLAN_EXPORT_ALLOCATION_FAILED; }
    for (size_t i = 0; i < outline->vertex_count; i++) {
        vertices[i] = (CadExportPoint2){
            (int64_t)outline->vertices[i].x,
            (int64_t)outline->vertices[i].y
        };
    }
    CadExportPathInput input = {vertices, outline->vertex_count, 1, layer};
    CadPlanExportCode result = map_ir_code(cad_export_document_append_path(document, &input));
    free(vertices);
    return result;
}

CadPlanExportResult cad_plan_export_storey(const SiteHelperProject *project,
    DomainId storey_id, const CadPlanExportConfig *config,
    CadExportDocument *output)
{
    CadPlanExportStatistics statistics = {0};
    if (project == NULL || config == NULL || output == NULL ||
        storey_id == DOMAIN_ID_INVALID) {
        return export_result(CAD_PLAN_EXPORT_INVALID_ARGUMENT, statistics);
    }
    if (sitehelper_project_validate(project).code != SITEHELPER_PROJECT_VALID) {
        return export_result(CAD_PLAN_EXPORT_INVALID_PROJECT, statistics);
    }
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, storey_id);
    if (storey == NULL) {
        return export_result(CAD_PLAN_EXPORT_STOREY_NOT_FOUND, statistics);
    }

    CadExportDocument candidate = {0};
    CadPlanExportCode code = CAD_PLAN_EXPORT_SUCCESS;

    if (config->include_wall_centerlines) {
        for (size_t i = 0; i < storey->structure.wall_count; i++) {
            const WallPlanSegment segment = storey->structure.walls[i].definition.segment;
            code = append_pair(&candidate, segment.start, segment.end,
                CAD_PLAN_EXPORT_LAYER_WALL_CENTERLINES);
            if (code != CAD_PLAN_EXPORT_SUCCESS) { goto fail; }
            statistics.wall_centerline_count++;
        }
    }

    if (config->include_room_separators) {
        for (size_t i = 0; i < storey->structure.room_separator_count; i++) {
            const PlanSegment segment = storey->structure.room_separators[i].segment;
            code = append_pair(&candidate, segment.start, segment.end,
                CAD_PLAN_EXPORT_LAYER_ROOM_SEPARATORS);
            if (code != CAD_PLAN_EXPORT_SUCCESS) { goto fail; }
            statistics.room_separator_count++;
        }
    }

    if (config->include_slab_outlines) {
        for (size_t i = 0; i < storey->slabs.count; i++) {
            code = append_outline(&candidate, &storey->slabs.items[i].definition.outline,
                CAD_PLAN_EXPORT_LAYER_SLAB_OUTLINES);
            if (code != CAD_PLAN_EXPORT_SUCCESS) { goto fail; }
            statistics.slab_outline_count++;
        }
    }

    statistics.path_count = candidate.path_count;
    cad_export_document_destroy(output);
    *output = candidate;
    return export_result(CAD_PLAN_EXPORT_SUCCESS, statistics);

fail:
    cad_export_document_destroy(&candidate);
    return export_result(code, (CadPlanExportStatistics){0});
}
