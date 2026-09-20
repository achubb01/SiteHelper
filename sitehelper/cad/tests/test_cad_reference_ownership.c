#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cad_plan_mapping.h"
#include "dxf_ascii.h"
#include "sitehelper_project.h"

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

#define DXF_BEGIN(UNITS) \
    "0\nSECTION\n2\nHEADER\n" \
    "9\n$ACADVER\n1\nAC1032\n" \
    "9\n$INSUNITS\n70\n" UNITS "\n" \
    "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
#define DXF_END "0\nENDSEC\n0\nEOF\n"

static CadPlanReference map_text(const char *text, CadPlanMappingConfig config)
{
    CadIrDocument source = {0};
    DxfAsciiDecodeResult decode = dxf_ascii_decode_memory(text, strlen(text), &source);
    assert(decode.code == DXF_ASCII_DECODE_SUCCESS);
    CadPlanReference result = {0};
    assert(cad_plan_map_reference(&source, &config, &result) == CAD_PLAN_MAP_SUCCESS);
    cad_ir_document_destroy(&source);
    return result;
}

static CadPlanReference mapped_line(int end_x)
{
    char dxf[512];
    int written = snprintf(dxf, sizeof dxf,
        DXF_BEGIN("4")
        "0\nLINE\n5\n2A\n8\nREFERENCE\n10\n0\n20\n0\n11\n%d\n21\n0\n"
        DXF_END, end_x);
    assert(written > 0 && (size_t)written < sizeof dxf);
    return map_text(dxf, (CadPlanMappingConfig){0});
}

static void test_adopt_replace_clear_and_identity_independence(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId first = sitehelper_project_add_storey(&project, 0);
    DomainId second = sitehelper_project_add_storey(&project, 2700);
    assert(first != DOMAIN_ID_INVALID && second != DOMAIN_ID_INVALID);
    DomainId next_id = project.domain_ids.next;

    CadPlanReference initial = mapped_line(4200);
    assert(initial.status == CAD_PLAN_REFERENCE_READY);
    assert(sitehelper_project_adopt_cad_plan_reference(&project, first, &initial) ==
        SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS);
    assert(initial.status == CAD_PLAN_REFERENCE_EMPTY && initial.paths == NULL);
    assert(project.cad_plan_reference_count == 1);
    assert(project.domain_ids.next == next_id);

    const CadPlanReference *stored = sitehelper_project_find_cad_plan_reference(&project, first);
    assert(stored != NULL && stored->path_count == 1);
    assert(stored->paths[0].vertices[1].x == 4200);
    assert(sitehelper_project_find_cad_plan_reference(&project, second) == NULL);

    CadPlanReference replacement = mapped_line(6000);
    assert(sitehelper_project_adopt_cad_plan_reference(&project, first, &replacement) ==
        SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS);
    assert(replacement.status == CAD_PLAN_REFERENCE_EMPTY && replacement.paths == NULL);
    assert(project.cad_plan_reference_count == 1 && project.domain_ids.next == next_id);
    stored = sitehelper_project_find_cad_plan_reference(&project, first);
    assert(stored != NULL && stored->paths[0].vertices[1].x == 6000);

    assert(sitehelper_project_clear_cad_plan_reference(&project, first));
    assert(project.cad_plan_reference_count == 0);
    assert(sitehelper_project_find_cad_plan_reference(&project, first) == NULL);
    assert(!sitehelper_project_clear_cad_plan_reference(&project, first));
    assert(project.domain_ids.next == next_id);
    assert(sitehelper_project_validate(&project).code == SITEHELPER_PROJECT_VALID);
    sitehelper_project_destroy(&project);
}

static void test_rejected_proposals_preserve_both_owners(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    assert(storey != DOMAIN_ID_INVALID);

    CadPlanReference good = mapped_line(4200);
    assert(sitehelper_project_adopt_cad_plan_reference(&project, storey, &good) ==
        SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS);
    const CadPlanReference *before = sitehelper_project_find_cad_plan_reference(&project, storey);
    assert(before != NULL && before->paths[0].vertices[1].x == 4200);

    static const char unitless[] =
        DXF_BEGIN("0")
        "0\nLINE\n8\nREFERENCE\n10\n0\n20\n0\n11\n6000\n21\n0\n"
        DXF_END;
    CadPlanReference blocked = map_text(unitless, (CadPlanMappingConfig){0});
    assert(blocked.status == CAD_PLAN_REFERENCE_BLOCKED_UNITS);
    assert(sitehelper_project_adopt_cad_plan_reference(&project, storey, &blocked) ==
        SITEHELPER_CAD_REFERENCE_APPLY_REFERENCE_NOT_READY);
    assert(blocked.status == CAD_PLAN_REFERENCE_BLOCKED_UNITS);
    before = sitehelper_project_find_cad_plan_reference(&project, storey);
    assert(before != NULL && before->paths[0].vertices[1].x == 4200);

    CadPlanReference candidate = mapped_line(7000);
    assert(sitehelper_project_adopt_cad_plan_reference(&project, UINT64_MAX, &candidate) ==
        SITEHELPER_CAD_REFERENCE_APPLY_STOREY_NOT_FOUND);
    assert(candidate.status == CAD_PLAN_REFERENCE_READY && candidate.path_count == 1);
    before = sitehelper_project_find_cad_plan_reference(&project, storey);
    assert(before != NULL && before->paths[0].vertices[1].x == 4200);

    cad_plan_reference_destroy(&candidate);
    cad_plan_reference_destroy(&blocked);
    sitehelper_project_destroy(&project);
}

static void test_decode_or_mapping_failure_never_mutates_live_project(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    CadPlanReference initial = mapped_line(4200);
    assert(sitehelper_project_adopt_cad_plan_reference(&project, storey, &initial) ==
        SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS);

    CadIrDocument decoded = {0};
    static const char malformed[] = "0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n0\n";
    assert(dxf_ascii_decode_memory(malformed, sizeof malformed - 1, &decoded).code !=
        DXF_ASCII_DECODE_SUCCESS);
    assert(decoded.paths == NULL);
    const CadPlanReference *stored = sitehelper_project_find_cad_plan_reference(&project, storey);
    assert(stored != NULL && stored->paths[0].vertices[1].x == 4200);

    static const char unitless[] =
        DXF_BEGIN("0")
        "0\nLINE\n8\nREFERENCE\n10\n0\n20\n0\n11\n8000\n21\n0\n"
        DXF_END;
    assert(dxf_ascii_decode_memory(unitless, sizeof unitless - 1, &decoded).code ==
        DXF_ASCII_DECODE_SUCCESS);
    CadPlanReference proposal = {0};
    assert(cad_plan_map_reference(&decoded, &(CadPlanMappingConfig){0}, &proposal) ==
        CAD_PLAN_MAP_SUCCESS);
    assert(proposal.status == CAD_PLAN_REFERENCE_BLOCKED_UNITS);
    stored = sitehelper_project_find_cad_plan_reference(&project, storey);
    assert(stored != NULL && stored->paths[0].vertices[1].x == 4200);

    cad_plan_reference_destroy(&proposal);
    cad_ir_document_destroy(&decoded);
    sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_new_attachment_allocation_failure_is_transactional(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    CadPlanReference candidate = mapped_line(4200);
    DomainId next = project.domain_ids.next;

    fail_after = 0;
    allocation_failed = 0;
    assert(sitehelper_project_adopt_cad_plan_reference(&project, storey, &candidate) ==
        SITEHELPER_CAD_REFERENCE_APPLY_ALLOCATION_FAILED);
    assert(allocation_failed);
    fail_after = SIZE_MAX;

    assert(project.cad_plan_reference_count == 0);
    assert(project.cad_plan_references == NULL);
    assert(project.domain_ids.next == next);
    assert(candidate.status == CAD_PLAN_REFERENCE_READY && candidate.path_count == 1);
    assert(candidate.paths[0].vertices[1].x == 4200);

    cad_plan_reference_destroy(&candidate);
    sitehelper_project_destroy(&project);
}
#endif

int main(void)
{
    test_adopt_replace_clear_and_identity_independence();
    test_rejected_proposals_preserve_both_owners();
    test_decode_or_mapping_failure_never_mutates_live_project();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_new_attachment_allocation_failure_is_transactional();
#endif
    puts("All CAD reference ownership tests passed.");
    return 0;
}
