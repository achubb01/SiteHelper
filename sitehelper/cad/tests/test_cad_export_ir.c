#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cad_export_ir.h"
#include "cad_plan_mapping.h"
#include "dxf_ascii.h"
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

static void append_path(CadExportDocument *document, const CadExportPoint2 *vertices,
    size_t count, int closed, const char *layer)
{
    CadExportPathInput input = {vertices, count, closed, layer};
    assert(cad_export_document_append_path(document, &input) == CAD_EXPORT_IR_SUCCESS);
}

static void test_ir_deep_ownership_and_validation(void)
{
    CadExportDocument document = {0};
    CadExportPoint2 line[2] = {{10, 20}, {30, 40}};
    char layer[] = "WALLS";
    append_path(&document, line, 2, 0, layer);
    line[0].x_mm = 999;
    layer[0] = 'X';
    assert(document.path_count == 1);
    assert(document.paths[0].vertices[0].x_mm == 10);
    assert(strcmp(document.paths[0].layer, "WALLS") == 0);

    CadExportPoint2 closed_bad[3] = {{0, 0}, {10, 0}, {0, 0}};
    CadExportPathInput invalid = {closed_bad, 3, 1, "SLAB"};
    assert(cad_export_document_append_path(&document, &invalid) == CAD_EXPORT_IR_INVALID_PATH);
    assert(document.path_count == 1);
    invalid = (CadExportPathInput){line, 2, 0, "BAD\nLAYER"};
    assert(cad_export_document_append_path(&document, &invalid) == CAD_EXPORT_IR_INVALID_PATH);
    assert(cad_export_document_append_path(NULL, &invalid) == CAD_EXPORT_IR_INVALID_ARGUMENT);

    cad_export_document_destroy(&document);
    assert(document.paths == NULL && document.path_count == 0);
}

static void test_ascii_dxf_writer_and_round_trip(void)
{
    CadExportDocument document = {0};
    CadExportPoint2 wall[2] = {{-100, 50}, {4200, 50}};
    CadExportPoint2 slab[4] = {{0, 0}, {5000, 0}, {5000, 3000}, {0, 3000}};
    append_path(&document, wall, 2, 0, "SITEHELPER_WALL_CENTERLINES");
    append_path(&document, slab, 4, 1, "SITEHELPER_SLAB_OUTLINES");

    DxfAsciiExportBuffer output = {0};
    assert(dxf_ascii_export_memory(&document, &output) == DXF_ASCII_EXPORT_SUCCESS);
    assert(output.bytes != NULL && output.byte_count == strlen(output.bytes));
    assert(strstr(output.bytes, "$ACADVER\n1\nAC1032\n") != NULL);
    assert(strstr(output.bytes, "$INSUNITS\n70\n4\n") != NULL);
    assert(strstr(output.bytes, "0\nLINE\n") != NULL);
    assert(strstr(output.bytes, "0\nLWPOLYLINE\n") != NULL);
    assert(strstr(output.bytes, "2\nSITEHELPER_WALL_CENTERLINES\n") != NULL);
    assert(strstr(output.bytes, "2\nSITEHELPER_SLAB_OUTLINES\n") != NULL);

    CadIrDocument decoded = {0};
    DxfAsciiDecodeResult decode = dxf_ascii_decode_memory(output.bytes,
        output.byte_count, &decoded);
    assert(decode.code == DXF_ASCII_DECODE_SUCCESS);
    assert(decoded.declared_unit == CAD_IR_UNIT_MILLIMETRE);
    assert(decoded.path_count == 2);
    assert(decoded.paths[0].vertex_count == 2 && !decoded.paths[0].closed);
    assert(decoded.paths[1].vertex_count == 4 && decoded.paths[1].closed);

    CadPlanReference mapped = {0};
    assert(cad_plan_map_reference(&decoded, &(CadPlanMappingConfig){0}, &mapped) ==
        CAD_PLAN_MAP_SUCCESS);
    assert(mapped.status == CAD_PLAN_REFERENCE_READY && mapped.path_count == 2);
    assert(mapped.paths[0].vertices[0].x == -100);
    assert(mapped.paths[0].vertices[1].x == 4200);
    assert(mapped.paths[1].vertices[2].x == 5000);
    assert(mapped.paths[1].vertices[2].y == 3000);
    assert(mapped.paths[1].closed);

    cad_plan_reference_destroy(&mapped);
    cad_ir_document_destroy(&decoded);
    dxf_ascii_export_buffer_destroy(&output);
    cad_export_document_destroy(&document);
}

static void test_empty_export_is_valid_dxf(void)
{
    CadExportDocument document = {0};
    DxfAsciiExportBuffer output = {0};
    assert(dxf_ascii_export_memory(&document, &output) == DXF_ASCII_EXPORT_SUCCESS);
    assert(strstr(output.bytes, "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n") != NULL);
    dxf_ascii_export_buffer_destroy(&output);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_ir_allocation_failure_is_transactional(void)
{
    CadExportDocument document = {0};
    CadExportPoint2 first[2] = {{0, 0}, {1, 0}};
    append_path(&document, first, 2, 0, "OLD");
    CadExportPoint2 second[2] = {{0, 0}, {2, 0}};
    CadExportPathInput input = {second, 2, 0, "NEW"};

    size_t failures = 0;
    for (size_t attempt = 0; attempt < 16; attempt++) {
        fail_after = attempt;
        allocation_failed = 0;
        CadExportIrCode code = cad_export_document_append_path(&document, &input);
        fail_after = SIZE_MAX;
        if (code == CAD_EXPORT_IR_SUCCESS) { break; }
        assert(code == CAD_EXPORT_IR_ALLOCATION_FAILED && allocation_failed);
        assert(document.path_count == 1);
        assert(strcmp(document.paths[0].layer, "OLD") == 0);
        failures++;
    }
    assert(failures >= 2 && document.path_count == 2);
    cad_export_document_destroy(&document);
}

static void test_writer_allocation_failure_is_transactional(void)
{
    CadExportDocument document = {0};
    CadExportPoint2 line[2] = {{0, 0}, {4200, 0}};
    append_path(&document, line, 2, 0, "WALLS");

    DxfAsciiExportBuffer output = {0};
    output.bytes = malloc(4);
    assert(output.bytes != NULL);
    memcpy(output.bytes, "OLD", 4);
    output.byte_count = 3;

    size_t failures = 0;
    for (size_t attempt = 0; attempt < 64; attempt++) {
        fail_after = attempt;
        allocation_failed = 0;
        DxfAsciiExportCode code = dxf_ascii_export_memory(&document, &output);
        fail_after = SIZE_MAX;
        if (code == DXF_ASCII_EXPORT_SUCCESS) { break; }
        assert(code == DXF_ASCII_EXPORT_ALLOCATION_FAILED && allocation_failed);
        assert(output.byte_count == 3 && strcmp(output.bytes, "OLD") == 0);
        failures++;
    }
    assert(failures >= 1);
    assert(output.byte_count > 3 && strstr(output.bytes, "AC1032") != NULL);

    dxf_ascii_export_buffer_destroy(&output);
    cad_export_document_destroy(&document);
}
#endif

int main(void)
{
    test_ir_deep_ownership_and_validation();
    test_ascii_dxf_writer_and_round_trip();
    test_empty_export_is_valid_dxf();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_ir_allocation_failure_is_transactional();
    test_writer_allocation_failure_is_transactional();
#endif
    puts("All CAD export IR / ASCII DXF writer tests passed.");
    return 0;
}
