#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cad_plan_export.h"
#include "dxf_ascii_export.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int reject_allocation(void)
{
    if (fail_after != SIZE_MAX && fail_after-- == 0) {
        allocation_failed = 1;
        return 1;
    }
    return 0;
}
void *__wrap_malloc(size_t size) { return reject_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return reject_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *pointer, size_t size)
{
    return reject_allocation() ? NULL : __real_realloc(pointer, size);
}
#endif

static CadPlanExportConfig all_geometry(void)
{
    return (CadPlanExportConfig){1, 1, 1};
}

static void build_project(SiteHelperProject *project, DomainId *first, DomainId *second)
{
    sitehelper_project_init(project);
    *first = sitehelper_project_add_storey(project, 0);
    *second = sitehelper_project_add_storey(project, 2700);
    assert(*first != DOMAIN_ID_INVALID && *second != DOMAIN_ID_INVALID);

    assert(sitehelper_project_add_wall(project, *first,
        (WallPlanSegment){{0, 0}, {4200, 0}}) != DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_room_separator(project, *first,
        (PlanSegment){{100, 200}, {100, 2200}}) != DOMAIN_ID_INVALID);
    PlanPosition slab[4] = {{-500, -500}, {5000, -500}, {5000, 3000}, {-500, 3000}};
    assert(sitehelper_project_add_slab(project, *first, slab, 4, 100, 0) != DOMAIN_ID_INVALID);
    assert(sitehelper_project_add_wall(project, *second,
        (WallPlanSegment){{10, 20}, {3010, 20}}) != DOMAIN_ID_INVALID);

    CadPlanReference reference = {.status = CAD_PLAN_REFERENCE_READY};
    assert(sitehelper_project_adopt_cad_plan_reference(project, *first, &reference) ==
        SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS);
    assert(reference.status == CAD_PLAN_REFERENCE_EMPTY);
    assert(sitehelper_project_validate(project).code == SITEHELPER_PROJECT_VALID);
}

static void test_storey_scope_categories_and_reference_exclusion(void)
{
    SiteHelperProject project;
    DomainId first, second;
    build_project(&project, &first, &second);

    CadExportDocument export = {0};
    CadPlanExportResult result = cad_plan_export_storey(&project, first,
        &(CadPlanExportConfig){1, 1, 1}, &export);
    assert(result.code == CAD_PLAN_EXPORT_SUCCESS);
    assert(result.statistics.wall_centerline_count == 1);
    assert(result.statistics.room_separator_count == 1);
    assert(result.statistics.slab_outline_count == 1);
    assert(result.statistics.path_count == 3 && export.path_count == 3);

    assert(strcmp(export.paths[0].layer, CAD_PLAN_EXPORT_LAYER_WALL_CENTERLINES) == 0);
    assert(export.paths[0].vertex_count == 2 && !export.paths[0].closed);
    assert(export.paths[0].vertices[1].x_mm == 4200);
    assert(strcmp(export.paths[1].layer, CAD_PLAN_EXPORT_LAYER_ROOM_SEPARATORS) == 0);
    assert(export.paths[1].vertices[0].x_mm == 100);
    assert(strcmp(export.paths[2].layer, CAD_PLAN_EXPORT_LAYER_SLAB_OUTLINES) == 0);
    assert(export.paths[2].closed && export.paths[2].vertex_count == 4);
    assert(export.paths[2].vertices[0].x_mm == -500);

    result = cad_plan_export_storey(&project, second,
        &(CadPlanExportConfig){1, 1, 1}, &export);
    assert(result.code == CAD_PLAN_EXPORT_SUCCESS);
    assert(export.path_count == 1 && result.statistics.wall_centerline_count == 1);
    assert(export.paths[0].vertices[0].x_mm == 10);
    assert(export.paths[0].vertices[1].x_mm == 3010);

    CadPlanExportConfig wall_only = {1, 0, 0};
    result = cad_plan_export_storey(&project, first, &wall_only, &export);
    assert(result.code == CAD_PLAN_EXPORT_SUCCESS && export.path_count == 1);
    assert(strcmp(export.paths[0].layer, CAD_PLAN_EXPORT_LAYER_WALL_CENTERLINES) == 0);

    cad_export_document_destroy(&export);
    sitehelper_project_destroy(&project);
}

static void test_export_to_dxf_contains_only_explicit_policy_geometry(void)
{
    SiteHelperProject project;
    DomainId first, second;
    build_project(&project, &first, &second);
    (void)second;

    CadExportDocument export = {0};
    CadPlanExportResult result = cad_plan_export_storey(&project, first,
        &(CadPlanExportConfig){1, 0, 1}, &export);
    assert(result.code == CAD_PLAN_EXPORT_SUCCESS && export.path_count == 2);

    DxfAsciiExportBuffer dxf = {0};
    assert(dxf_ascii_export_memory(&export, &dxf) == DXF_ASCII_EXPORT_SUCCESS);
    assert(strstr(dxf.bytes, CAD_PLAN_EXPORT_LAYER_WALL_CENTERLINES) != NULL);
    assert(strstr(dxf.bytes, CAD_PLAN_EXPORT_LAYER_SLAB_OUTLINES) != NULL);
    assert(strstr(dxf.bytes, CAD_PLAN_EXPORT_LAYER_ROOM_SEPARATORS) == NULL);
    assert(strstr(dxf.bytes, "REFERENCE") == NULL);

    dxf_ascii_export_buffer_destroy(&dxf);
    cad_export_document_destroy(&export);
    sitehelper_project_destroy(&project);
}

static void test_failure_preserves_previous_output(void)
{
    SiteHelperProject project;
    DomainId first, second;
    build_project(&project, &first, &second);
    (void)second;

    CadExportDocument export = {0};
    assert(cad_plan_export_storey(&project, first, &(CadPlanExportConfig){1, 0, 0},
        &export).code == CAD_PLAN_EXPORT_SUCCESS);
    assert(export.path_count == 1);
    int64_t previous_end = export.paths[0].vertices[1].x_mm;

    CadPlanExportConfig all = all_geometry();
    assert(cad_plan_export_storey(&project, UINT64_MAX, &all, &export).code ==
        CAD_PLAN_EXPORT_STOREY_NOT_FOUND);
    assert(export.path_count == 1 && export.paths[0].vertices[1].x_mm == previous_end);

    assert(cad_plan_export_storey(NULL, first, &all, &export).code ==
        CAD_PLAN_EXPORT_INVALID_ARGUMENT);
    assert(export.path_count == 1 && export.paths[0].vertices[1].x_mm == previous_end);

    cad_export_document_destroy(&export);
    sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_export_allocation_failure_is_transactional(void)
{
    SiteHelperProject project;
    DomainId first, second;
    build_project(&project, &first, &second);
    (void)second;

    size_t failures = 0;
    int succeeded = 0;
    for (size_t attempt = 0; attempt < 64 && !succeeded; attempt++) {
        fail_after = SIZE_MAX;
        CadExportDocument output = {0};
        CadExportPoint2 sentinel[2] = {{0, 0}, {77, 0}};
        CadExportPathInput input = {sentinel, 2, 0, "SENTINEL"};
        assert(cad_export_document_append_path(&output, &input) == CAD_EXPORT_IR_SUCCESS);

        fail_after = attempt;
        allocation_failed = 0;
        CadPlanExportResult result = cad_plan_export_storey(&project, first,
            &(CadPlanExportConfig){1, 1, 1}, &output);
        fail_after = SIZE_MAX;
        if (result.code == CAD_PLAN_EXPORT_SUCCESS) {
            succeeded = 1;
            assert(output.path_count == 3);
        } else {
            assert(result.code == CAD_PLAN_EXPORT_ALLOCATION_FAILED && allocation_failed);
            assert(output.path_count == 1);
            assert(strcmp(output.paths[0].layer, "SENTINEL") == 0);
            assert(output.paths[0].vertices[1].x_mm == 77);
            failures++;
        }
        cad_export_document_destroy(&output);
    }
    assert(succeeded && failures >= 6);
    sitehelper_project_destroy(&project);
}
#endif

int main(void)
{
    test_storey_scope_categories_and_reference_exclusion();
    test_export_to_dxf_contains_only_explicit_policy_geometry();
    test_failure_preserves_previous_output();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_export_allocation_failure_is_transactional();
#endif
    puts("All CAD Plan export policy tests passed.");
    return 0;
}
