#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cad_ir.h"

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

static CadIrPoint2 point(int64_t x_coefficient, int32_t x_exponent,
    int64_t y_coefficient, int32_t y_exponent)
{
    return (CadIrPoint2){
        .x = {x_coefficient, x_exponent},
        .y = {y_coefficient, y_exponent}
    };
}

static CadIrPathInput line_input(const CadIrPoint2 *vertices, size_t ordinal,
    const char *layer, const char *handle)
{
    return (CadIrPathInput){
        .vertices = vertices,
        .vertex_count = 2,
        .closed = 0,
        .provenance = {ordinal, "LINE", layer, handle}
    };
}

static void assert_zero(const CadIrDocument *document)
{
    assert(document->source_format == NULL && document->source_version == NULL);
    assert(document->declared_unit == CAD_IR_UNIT_UNSPECIFIED);
    assert(document->paths == NULL && document->path_count == 0 && document->path_capacity == 0);
    assert(document->diagnostics == NULL && document->diagnostic_count == 0 &&
        document->diagnostic_capacity == 0);
    assert(document->decoded_entity_count == 0 && document->skipped_entity_count == 0);
}

static void test_init_metadata_and_exact_source_values(void)
{
    CadIrDocument document;
    memset(&document, 0xA5, sizeof document);
    cad_ir_document_init(&document);
    assert_zero(&document);

    char format[] = "DXF";
    char version[] = "AC1032";
    assert(cad_ir_document_set_metadata(&document, format, version,
        CAD_IR_UNIT_METRE) == CAD_IR_SUCCESS);
    format[0] = 'X'; version[0] = 'X';
    assert(strcmp(document.source_format, "DXF") == 0);
    assert(strcmp(document.source_version, "AC1032") == 0);
    assert(document.declared_unit == CAD_IR_UNIT_METRE);

    /* Exact decimals remain source values rather than early doubles/Plan ints. */
    CadIrPoint2 vertices[2] = {
        point(42, -1, -123456789012345678LL, -7),
        point(INT64_MAX, -18, INT64_MIN + 1, 12)
    };
    char layer[] = "A-WALL-REF";
    char handle[] = "0000000000000042";
    CadIrPathInput line = line_input(vertices, 17, layer, handle);
    assert(cad_ir_document_append_path(&document, &line) == CAD_IR_SUCCESS);
    vertices[0] = point(0, 0, 0, 0); layer[0] = 'X'; handle[0] = 'X';

    assert(document.path_count == 1);
    const CadIrPath *stored = &document.paths[0];
    assert(stored->vertex_count == 2 && stored->closed == 0);
    assert(stored->vertices[0].x.coefficient == 42 && stored->vertices[0].x.exponent10 == -1);
    assert(stored->vertices[0].y.coefficient == -123456789012345678LL);
    assert(stored->vertices[1].x.coefficient == INT64_MAX);
    assert(stored->vertices[1].y.exponent10 == 12);
    assert(stored->provenance.entity_ordinal == 17);
    assert(strcmp(stored->provenance.entity_kind, "LINE") == 0);
    assert(strcmp(stored->provenance.layer, "A-WALL-REF") == 0);
    /* Numeric-looking external handle remains an opaque string. */
    assert(strcmp(stored->provenance.handle, "0000000000000042") == 0);

    assert(cad_ir_document_set_metadata(&document, "DXF", "AC1015",
        CAD_IR_UNIT_UNSPECIFIED) == CAD_IR_SUCCESS);
    assert(strcmp(document.source_version, "AC1015") == 0);
    assert(document.declared_unit == CAD_IR_UNIT_UNSPECIFIED);
    assert(document.path_count == 1); /* Metadata replacement never touches geometry. */

    cad_ir_document_destroy(&document);
    assert_zero(&document);
    cad_ir_document_destroy(&document);
    cad_ir_document_destroy(NULL);
}

static void test_open_closed_paths_provenance_and_lifetime(void)
{
    CadIrDocument document = {0};
    assert(cad_ir_document_set_metadata(&document, "DXF", "AC1027",
        CAD_IR_UNIT_MILLIMETRE) == CAD_IR_SUCCESS);

    CadIrPoint2 open_vertices[4] = {
        point(0, 0, 0, 0), point(1200, 0, 0, 0),
        point(1200, 0, 900, 0), point(2400, 0, 900, 0)
    };
    CadIrPathInput open = {
        .vertices = open_vertices, .vertex_count = 4, .closed = 0,
        .provenance = {3, "LWPOLYLINE", "GRID", NULL}
    };
    assert(cad_ir_document_append_path(&document, &open) == CAD_IR_SUCCESS);

    CadIrPoint2 closed_vertices[3] = {
        point(0, 0, 0, 0), point(5000, 0, 0, 0), point(0, 0, 4000, 0)
    };
    CadIrPathInput closed = {
        .vertices = closed_vertices, .vertex_count = 3, .closed = 1,
        .provenance = {4, "LWPOLYLINE", "BOUNDARY", "ABC"}
    };
    assert(cad_ir_document_append_path(&document, &closed) == CAD_IR_SUCCESS);
    assert(document.path_count == 2);
    assert(document.paths[0].closed == 0 && document.paths[0].vertex_count == 4);
    assert(document.paths[1].closed == 1 && document.paths[1].vertex_count == 3);
    /* Closed path does not manufacture a repeated first vertex. */
    assert(document.paths[1].vertices[2].y.coefficient == 4000);
    assert(strcmp(document.paths[0].provenance.layer, "GRID") == 0);
    assert(document.paths[0].provenance.handle == NULL);
    assert(strcmp(document.paths[1].provenance.handle, "ABC") == 0);

    /* Source stack storage may disappear/change after a successful append. */
    memset(open_vertices, 0, sizeof open_vertices);
    memset(closed_vertices, 0, sizeof closed_vertices);
    assert(document.paths[0].vertices[1].x.coefficient == 1200);
    assert(document.paths[1].vertices[1].x.coefficient == 5000);

    cad_ir_document_destroy(&document);
}

static void test_diagnostics_and_accounting(void)
{
    CadIrDocument document = {0};
    CadIrDiagnosticInput global = {
        .severity = CAD_IR_DIAGNOSTIC_ERROR,
        .code = "UNITS_REQUIRED",
        .detail = "source unit is unresolved",
        .has_source = 0
    };
    assert(cad_ir_document_append_diagnostic(&document, &global) == CAD_IR_SUCCESS);

    char code[] = "ENTITY_UNSUPPORTED_KIND";
    char detail[] = "ARC";
    char layer[] = "DETAIL";
    char handle[] = "2A";
    CadIrDiagnosticInput source = {
        .severity = CAD_IR_DIAGNOSTIC_WARNING,
        .code = code, .detail = detail, .has_source = 1,
        .provenance = {9, "ARC", layer, handle}
    };
    assert(cad_ir_document_append_diagnostic(&document, &source) == CAD_IR_SUCCESS);
    code[0] = 'X'; detail[0] = 'X'; layer[0] = 'X'; handle[0] = 'X';

    CadIrDiagnosticInput info = {
        .severity = CAD_IR_DIAGNOSTIC_INFO,
        .code = "FORMAT_NOTE",
        .detail = NULL,
        .has_source = 0
    };
    assert(cad_ir_document_append_diagnostic(&document, &info) == CAD_IR_SUCCESS);
    assert(cad_ir_document_set_entity_counts(&document, 12, 7) == CAD_IR_SUCCESS);

    assert(document.diagnostic_count == 3);
    assert(strcmp(document.diagnostics[0].code, "UNITS_REQUIRED") == 0);
    assert(document.diagnostics[0].has_source == 0);
    assert(document.diagnostics[0].provenance.entity_kind == NULL);
    assert(strcmp(document.diagnostics[1].code, "ENTITY_UNSUPPORTED_KIND") == 0);
    assert(strcmp(document.diagnostics[1].detail, "ARC") == 0);
    assert(document.diagnostics[1].provenance.entity_ordinal == 9);
    assert(strcmp(document.diagnostics[1].provenance.layer, "DETAIL") == 0);
    assert(strcmp(document.diagnostics[1].provenance.handle, "2A") == 0);

    CadIrStatistics statistics = cad_ir_document_statistics(&document);
    assert(statistics.decoded_entity_count == 12);
    assert(statistics.source_path_count == 0);
    assert(statistics.skipped_entity_count == 7);
    assert(statistics.info_count == 1 && statistics.warning_count == 1 && statistics.error_count == 1);
    CadIrStatistics zero = cad_ir_document_statistics(NULL);
    assert(memcmp(&zero, &(CadIrStatistics){0}, sizeof zero) == 0);
    cad_ir_document_destroy(&document);
}

static void assert_document_snapshot(const CadIrDocument *document,
    const char *format_pointer, const char *version_pointer,
    CadIrPath *paths_pointer, size_t path_count, size_t path_capacity,
    CadIrDiagnostic *diagnostics_pointer, size_t diagnostic_count,
    size_t diagnostic_capacity, size_t decoded, size_t skipped)
{
    assert(document->source_format == format_pointer);
    assert(document->source_version == version_pointer);
    assert(document->paths == paths_pointer && document->path_count == path_count &&
        document->path_capacity == path_capacity);
    assert(document->diagnostics == diagnostics_pointer &&
        document->diagnostic_count == diagnostic_count &&
        document->diagnostic_capacity == diagnostic_capacity);
    assert(document->decoded_entity_count == decoded && document->skipped_entity_count == skipped);
}

static void test_invalid_inputs_are_transactional(void)
{
    CadIrDocument document = {0};
    assert(cad_ir_document_set_metadata(&document, "DXF", "AC1032",
        CAD_IR_UNIT_MILLIMETRE) == CAD_IR_SUCCESS);
    CadIrPoint2 vertices[3] = {point(0,0,0,0), point(1,0,2,0), point(3,0,4,0)};
    CadIrPathInput valid = line_input(vertices, 1, "0", "10");
    assert(cad_ir_document_append_path(&document, &valid) == CAD_IR_SUCCESS);
    CadIrDiagnosticInput diagnostic = {
        .severity = CAD_IR_DIAGNOSTIC_WARNING, .code = "STYLE_NOT_PRESERVED",
        .detail = "width", .has_source = 1,
        .provenance = {1, "LWPOLYLINE", "0", "10"}
    };
    assert(cad_ir_document_append_diagnostic(&document, &diagnostic) == CAD_IR_SUCCESS);
    assert(cad_ir_document_set_entity_counts(&document, 2, 1) == CAD_IR_SUCCESS);

    char *format_pointer = document.source_format;
    char *version_pointer = document.source_version;
    CadIrPath *paths_pointer = document.paths;
    CadIrDiagnostic *diagnostics_pointer = document.diagnostics;
    size_t pc = document.path_count, pcap = document.path_capacity;
    size_t dc = document.diagnostic_count, dcap = document.diagnostic_capacity;
    size_t decoded = document.decoded_entity_count, skipped = document.skipped_entity_count;

    assert(cad_ir_document_set_metadata(NULL, "DXF", "AC1032", CAD_IR_UNIT_MILLIMETRE)
        == CAD_IR_INVALID_ARGUMENT);
    assert(cad_ir_document_set_metadata(&document, NULL, "AC1032", CAD_IR_UNIT_MILLIMETRE)
        == CAD_IR_INVALID_ARGUMENT);
    assert(cad_ir_document_set_metadata(&document, "", "AC1032", CAD_IR_UNIT_MILLIMETRE)
        == CAD_IR_INVALID_METADATA);
    assert(cad_ir_document_set_metadata(&document, "DXF", "", CAD_IR_UNIT_MILLIMETRE)
        == CAD_IR_INVALID_METADATA);
    assert(cad_ir_document_set_metadata(&document, "DXF", "AC1032", (CadIrSourceUnit)99)
        == CAD_IR_INVALID_METADATA);

    assert(cad_ir_document_append_path(NULL, &valid) == CAD_IR_INVALID_ARGUMENT);
    assert(cad_ir_document_append_path(&document, NULL) == CAD_IR_INVALID_ARGUMENT);
    CadIrPathInput bad = valid;
    bad.vertices = NULL;
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);
    bad = valid; bad.vertex_count = 1;
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);
    bad = valid; bad.closed = 2;
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);
    bad = valid; bad.closed = 1; bad.vertex_count = 2;
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);
    bad = valid; bad.provenance.entity_kind = "";
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);
    bad = valid; bad.provenance.layer = NULL;
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);
    bad = valid; bad.provenance.handle = "";
    assert(cad_ir_document_append_path(&document, &bad) == CAD_IR_INVALID_PATH);

    assert(cad_ir_document_append_diagnostic(NULL, &diagnostic) == CAD_IR_INVALID_ARGUMENT);
    assert(cad_ir_document_append_diagnostic(&document, NULL) == CAD_IR_INVALID_ARGUMENT);
    CadIrDiagnosticInput bad_diag = diagnostic;
    bad_diag.severity = (CadIrDiagnosticSeverity)99;
    assert(cad_ir_document_append_diagnostic(&document, &bad_diag) == CAD_IR_INVALID_DIAGNOSTIC);
    bad_diag = diagnostic; bad_diag.code = "";
    assert(cad_ir_document_append_diagnostic(&document, &bad_diag) == CAD_IR_INVALID_DIAGNOSTIC);
    bad_diag = diagnostic; bad_diag.detail = "";
    assert(cad_ir_document_append_diagnostic(&document, &bad_diag) == CAD_IR_INVALID_DIAGNOSTIC);
    bad_diag = diagnostic; bad_diag.has_source = 2;
    assert(cad_ir_document_append_diagnostic(&document, &bad_diag) == CAD_IR_INVALID_DIAGNOSTIC);
    bad_diag = diagnostic; bad_diag.provenance.entity_kind = NULL;
    assert(cad_ir_document_append_diagnostic(&document, &bad_diag) == CAD_IR_INVALID_DIAGNOSTIC);

    assert(cad_ir_document_set_entity_counts(NULL, 1, 0) == CAD_IR_INVALID_ARGUMENT);
    assert(cad_ir_document_set_entity_counts(&document, 2, 3) == CAD_IR_INVALID_COUNTS);
    assert_document_snapshot(&document, format_pointer, version_pointer, paths_pointer, pc, pcap,
        diagnostics_pointer, dc, dcap, decoded, skipped);

    /* Malformed public collection metadata is rejected before dereference/realloc. */
    CadIrDocument malformed = document;
    malformed.path_count = malformed.path_capacity + 1;
    assert(cad_ir_document_append_path(&malformed, &valid) == CAD_IR_INVALID_STATE);
    assert(cad_ir_document_set_metadata(&malformed, "DXF", "AC1015", CAD_IR_UNIT_MILLIMETRE)
        == CAD_IR_INVALID_STATE);
    malformed = document;
    malformed.diagnostics = NULL;
    assert(cad_ir_document_append_diagnostic(&malformed, &diagnostic) == CAD_IR_INVALID_STATE);
    CadIrStatistics malformed_statistics = cad_ir_document_statistics(&malformed);
    CadIrStatistics zero_statistics = {0};
    assert(memcmp(&malformed_statistics, &zero_statistics, sizeof zero_statistics) == 0);

    cad_ir_document_destroy(&document);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    size_t failures = 0;

    for (int operation = 0; operation < 3; operation++) {
        for (size_t n = 0; ; n++) {
            CadIrDocument document = {0};
            assert(cad_ir_document_set_metadata(&document, "DXF", "AC1015",
                CAD_IR_UNIT_MILLIMETRE) == CAD_IR_SUCCESS);
            CadIrPoint2 vertices[3] = {point(0,0,0,0), point(1,0,2,0), point(3,0,4,0)};
            CadIrPathInput path = {
                .vertices = vertices, .vertex_count = 3, .closed = 0,
                .provenance = {8, "LWPOLYLINE", "REFERENCE", "2F"}
            };
            CadIrDiagnosticInput diagnostic = {
                .severity = CAD_IR_DIAGNOSTIC_WARNING,
                .code = "ENTITY_UNSUPPORTED_KIND", .detail = "ARC", .has_source = 1,
                .provenance = {9, "ARC", "REFERENCE", "30"}
            };
            assert(cad_ir_document_append_path(&document, &path) == CAD_IR_SUCCESS);
            assert(cad_ir_document_append_diagnostic(&document, &diagnostic) == CAD_IR_SUCCESS);
            assert(cad_ir_document_set_entity_counts(&document, 2, 1) == CAD_IR_SUCCESS);

            char *format_pointer = document.source_format;
            char *version_pointer = document.source_version;
            CadIrPath *paths_pointer = document.paths;
            CadIrDiagnostic *diagnostics_pointer = document.diagnostics;
            size_t pc = document.path_count, pcap = document.path_capacity;
            size_t dc = document.diagnostic_count, dcap = document.diagnostic_capacity;

            fail_after = n;
            allocation_failed = 0;
            CadIrCode status;
            if (operation == 0) {
                status = cad_ir_document_set_metadata(&document, "DXF-ASCII", "AC1032",
                    CAD_IR_UNIT_METRE);
            } else if (operation == 1) {
                status = cad_ir_document_append_path(&document, &path);
            } else {
                status = cad_ir_document_append_diagnostic(&document, &diagnostic);
            }
            fail_after = SIZE_MAX;

            if (allocation_failed) {
                failures++;
                assert(status == CAD_IR_ALLOCATION_FAILED);
                assert_document_snapshot(&document, format_pointer, version_pointer,
                    paths_pointer, pc, pcap, diagnostics_pointer, dc, dcap, 2, 1);
                if (operation == 0) {
                    assert(strcmp(document.source_format, "DXF") == 0);
                    assert(strcmp(document.source_version, "AC1015") == 0);
                }
            } else {
                assert(status == CAD_IR_SUCCESS);
                cad_ir_document_destroy(&document);
                break;
            }
            cad_ir_document_destroy(&document);
            assert(n < 32);
        }
    }
    assert(failures >= 10);
    printf("cad-ir allocation failures checked: %zu\n", failures);
}
#endif

int main(void)
{
    test_init_metadata_and_exact_source_values();
    test_open_closed_paths_provenance_and_lifetime();
    test_diagnostics_and_accounting();
    test_invalid_inputs_are_transactional();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("cad-ir tests passed");
    return 0;
}
