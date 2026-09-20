#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dxf_ascii.h"

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

#define DXF_BEGIN(VERSION, UNITS) \
    "0\nSECTION\n2\nHEADER\n" \
    "9\n$ACADVER\n1\n" VERSION "\n" \
    "9\n$INSUNITS\n70\n" UNITS "\n" \
    "0\nENDSEC\n" \
    "0\nSECTION\n2\nENTITIES\n"
#define DXF_END "0\nENDSEC\n0\nEOF\n"

static DxfAsciiDecodeResult decode_text(const char *text, CadIrDocument *document)
{
    return dxf_ascii_decode_memory(text, strlen(text), document);
}

static const CadIrDiagnostic *find_diagnostic(const CadIrDocument *document,
    const char *code, size_t ordinal)
{
    for (size_t i = 0; i < document->diagnostic_count; i++) {
        const CadIrDiagnostic *diagnostic = &document->diagnostics[i];
        if (strcmp(diagnostic->code, code) == 0 && diagnostic->has_source &&
            diagnostic->provenance.entity_ordinal == ordinal) {
            return diagnostic;
        }
    }
    return NULL;
}

static void test_valid_line_exact_decimal_and_provenance(void)
{
    static const char dxf[] =
        DXF_BEGIN("AC1015", "4")
        "0\nLINE\n"
        "5\n0000000000000042\n"
        "8\nA-WALL-REF\n"
        "10\n4.2e3\n"
        "20\n-1.2500\n"
        "11\n+5000.000\n"
        "21\n2E2\n"
        DXF_END;
    CadIrDocument document = {0};
    DxfAsciiDecodeResult status = decode_text(dxf, &document);
    assert(status.code == DXF_ASCII_DECODE_SUCCESS && status.line_number == 0);
    assert(strcmp(document.source_format, "DXF") == 0);
    assert(strcmp(document.source_version, "AC1015") == 0);
    assert(document.declared_unit == CAD_IR_UNIT_MILLIMETRE);
    assert(document.path_count == 1 && document.diagnostic_count == 0);
    assert(document.decoded_entity_count == 1 && document.skipped_entity_count == 0);

    const CadIrPath *path = &document.paths[0];
    assert(path->vertex_count == 2 && path->closed == 0);
    assert(path->vertices[0].x.coefficient == 42 && path->vertices[0].x.exponent10 == 2);
    assert(path->vertices[0].y.coefficient == -125 && path->vertices[0].y.exponent10 == -2);
    assert(path->vertices[1].x.coefficient == 5 && path->vertices[1].x.exponent10 == 3);
    assert(path->vertices[1].y.coefficient == 2 && path->vertices[1].y.exponent10 == 2);
    assert(path->provenance.entity_ordinal == 1);
    assert(strcmp(path->provenance.entity_kind, "LINE") == 0);
    assert(strcmp(path->provenance.layer, "A-WALL-REF") == 0);
    assert(strcmp(path->provenance.handle, "0000000000000042") == 0);
    cad_ir_document_destroy(&document);
}

static void test_lwpolyline_open_closed_curve_and_width(void)
{
    static const char dxf[] =
        DXF_BEGIN("AC1032", "6")
        "0\nLWPOLYLINE\n8\nOPEN\n90\n3\n70\n0\n"
        "10\n0\n20\n0\n10\n1.2\n20\n0\n10\n1.2\n20\n2.5\n"
        "0\nLWPOLYLINE\n8\nCLOSED\n5\n2A\n90\n3\n70\n1\n43\n0.10\n"
        "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n"
        "0\nLWPOLYLINE\n8\nCURVED\n90\n2\n"
        "10\n0\n20\n0\n42\n0.5\n10\n10\n20\n0\n"
        DXF_END;
    CadIrDocument document = {0};
    assert(decode_text(dxf, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.declared_unit == CAD_IR_UNIT_METRE);
    assert(document.decoded_entity_count == 3);
    assert(document.path_count == 2 && document.skipped_entity_count == 1);
    assert(document.paths[0].closed == 0 && document.paths[0].vertex_count == 3);
    assert(document.paths[0].vertices[1].x.coefficient == 12);
    assert(document.paths[0].vertices[1].x.exponent10 == -1);
    assert(document.paths[1].closed == 1 && document.paths[1].vertex_count == 3);
    assert(strcmp(document.paths[1].provenance.handle, "2A") == 0);
    const CadIrDiagnostic *style = find_diagnostic(&document, "STYLE_NOT_PRESERVED", 2);
    assert(style != NULL && style->severity == CAD_IR_DIAGNOSTIC_WARNING);
    assert(strcmp(style->detail, "LWPOLYLINE width") == 0);
    assert(find_diagnostic(&document, "ENTITY_UNSUPPORTED_CURVE", 3) != NULL);
    cad_ir_document_destroy(&document);
}

static void test_skip_reasons_and_unsupported_entities(void)
{
    static const char dxf[] =
        DXF_BEGIN("AC1027", "2")
        "0\nLINE\n8\nPAPER\n67\n1\n10\n0\n20\n0\n11\n1\n21\n1\n"
        "0\nLINE\n8\n3D\n10\n0\n20\n0\n30\n1\n11\n2\n21\n0\n"
        "0\nARC\n8\nDETAIL\n5\n7F\n10\n0\n20\n0\n40\n10\n"
        "0\nLINE\n10\n5\n20\n5\n11\n5.0\n21\n5.00\n"
        "0\nLWPOLYLINE\n8\nLAYOUT\n410\nLayout1\n90\n2\n"
        "10\n0\n20\n0\n10\n1\n20\n1\n"
        DXF_END;
    CadIrDocument document = {0};
    assert(decode_text(dxf, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.declared_unit == CAD_IR_UNIT_FOOT);
    assert(document.decoded_entity_count == 5 && document.path_count == 0);
    assert(document.skipped_entity_count == 5);
    assert(find_diagnostic(&document, "ENTITY_PAPER_SPACE", 1) != NULL);
    assert(find_diagnostic(&document, "ENTITY_UNSUPPORTED_3D_OR_OCS", 2) != NULL);
    const CadIrDiagnostic *unsupported = find_diagnostic(&document,
        "ENTITY_UNSUPPORTED_KIND", 3);
    assert(unsupported != NULL);
    assert(strcmp(unsupported->provenance.entity_kind, "ARC") == 0);
    assert(strcmp(unsupported->provenance.layer, "DETAIL") == 0);
    assert(strcmp(unsupported->provenance.handle, "7F") == 0);
    assert(find_diagnostic(&document, "ENTITY_DEGENERATE", 4) != NULL);
    assert(find_diagnostic(&document, "ENTITY_PAPER_SPACE", 5) != NULL);
    cad_ir_document_destroy(&document);
}

static void test_numeric_and_malformed_entities_are_partial_success(void)
{
    static const char dxf[] =
        DXF_BEGIN("AC1024", "5")
        "0\nLINE\n8\nBADNUM\n10\nNaN\n20\n0\n11\n1\n21\n1\n"
        "0\nLINE\n8\nOVERFLOW\n10\n1e999999999999999999999\n20\n0\n11\n1\n21\n1\n"
        "0\nLWPOLYLINE\n8\nCOUNT\n90\n3\n10\n0\n20\n0\n10\n1\n20\n1\n"
        "0\nLINE\n8\nGOOD\n10\n-9223372036854775808\n20\n0\n11\n-9223372036854775807\n21\n0\n"
        DXF_END;
    CadIrDocument document = {0};
    assert(decode_text(dxf, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.declared_unit == CAD_IR_UNIT_CENTIMETRE);
    assert(document.decoded_entity_count == 4 && document.skipped_entity_count == 3);
    assert(document.path_count == 1);
    assert(find_diagnostic(&document, "NUMERIC_INVALID", 1) != NULL);
    assert(find_diagnostic(&document, "NUMERIC_OVERFLOW", 2) != NULL);
    assert(find_diagnostic(&document, "FORMAT_MALFORMED", 3) != NULL);
    assert(document.paths[0].vertices[0].x.coefficient == INT64_MIN);
    assert(strcmp(document.paths[0].provenance.layer, "GOOD") == 0);
    cad_ir_document_destroy(&document);
}

static void test_units_metadata_only(void)
{
    static const char unitless[] =
        DXF_BEGIN("AC1018", "0") DXF_END;
    static const char unsupported_unit[] =
        DXF_BEGIN("AC1021", "3") DXF_END;
    static const char no_units[] =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    CadIrDocument document = {0};
    assert(decode_text(unitless, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.declared_unit == CAD_IR_UNIT_UNSPECIFIED);
    cad_ir_document_destroy(&document);
    assert(decode_text(unsupported_unit, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.declared_unit == CAD_IR_UNIT_OTHER);
    cad_ir_document_destroy(&document);
    assert(decode_text(no_units, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.declared_unit == CAD_IR_UNIT_UNSPECIFIED);
    cad_ir_document_destroy(&document);
}

static void test_file_level_failures_are_transactional(void)
{
    static const unsigned char binary[] =
        "AutoCAD Binary DXF\r\n\x1A\0more";
    static const char malformed_pair[] = "0\nSECTION\n2\nHEADER\n0\n";
    static const char missing_version[] =
        "0\nSECTION\n2\nHEADER\n9\n$INSUNITS\n70\n4\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    static const char unsupported_version[] =
        DXF_BEGIN("AC1014", "4") DXF_END;
    static const char bad_units[] =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n"
        "9\n$INSUNITS\n70\nbanana\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";

    CadIrDocument document = {0};
    assert(cad_ir_document_set_metadata(&document, "OLD", "OLD1",
        CAD_IR_UNIT_INCH) == CAD_IR_SUCCESS);
    char *format_pointer = document.source_format;
    char *version_pointer = document.source_version;

    DxfAsciiDecodeResult status = dxf_ascii_decode_memory(binary,
        sizeof binary - 1, &document);
    assert(status.code == DXF_ASCII_DECODE_UNSUPPORTED_REPRESENTATION);
    assert(document.source_format == format_pointer && document.source_version == version_pointer);

    status = decode_text(malformed_pair, &document);
    assert(status.code == DXF_ASCII_DECODE_MALFORMED);
    assert(document.source_format == format_pointer && document.source_version == version_pointer);

    status = decode_text(missing_version, &document);
    assert(status.code == DXF_ASCII_DECODE_MISSING_VERSION);
    assert(document.source_format == format_pointer && document.source_version == version_pointer);

    status = decode_text(unsupported_version, &document);
    assert(status.code == DXF_ASCII_DECODE_UNSUPPORTED_VERSION && status.line_number != 0);
    assert(document.source_format == format_pointer && document.source_version == version_pointer);

    status = decode_text(bad_units, &document);
    assert(status.code == DXF_ASCII_DECODE_MALFORMED);
    assert(document.source_format == format_pointer && document.source_version == version_pointer);

    assert(dxf_ascii_decode_memory(NULL, 1, &document).code == DXF_ASCII_DECODE_INVALID_ARGUMENT);
    assert(dxf_ascii_decode_memory("", 0, &document).code == DXF_ASCII_DECODE_INVALID_ARGUMENT);
    assert(dxf_ascii_decode_memory("x", 1, NULL).code == DXF_ASCII_DECODE_INVALID_ARGUMENT);
    cad_ir_document_destroy(&document);
}

static void test_success_replaces_destination_and_crlf_is_accepted(void)
{
    static const char dxf[] =
        "  0\r\nSECTION\r\n999\r\ncomment between structural pairs\r\n  2\r\nHEADER\r\n  9\r\n$ACADVER\r\n  1\r\nAC1032\r\n"
        "  9\r\n$INSUNITS\r\n 70\r\n1\r\n  0\r\nENDSEC\r\n"
        "  0\r\nSECTION\r\n  2\r\nENTITIES\r\n"
        "  0\r\nLINE\r\n 10\r\n0\r\n 20\r\n0\r\n 11\r\n10\r\n 21\r\n0\r\n"
        "  0\r\nENDSEC\r\n  0\r\nEOF\r\n";
    CadIrDocument document = {0};
    assert(cad_ir_document_set_metadata(&document, "OLD", "OLD1",
        CAD_IR_UNIT_METRE) == CAD_IR_SUCCESS);
    char *old_format = document.source_format;
    assert(decode_text(dxf, &document).code == DXF_ASCII_DECODE_SUCCESS);
    assert(document.source_format != old_format);
    assert(strcmp(document.source_version, "AC1032") == 0);
    assert(document.declared_unit == CAD_IR_UNIT_INCH);
    assert(document.path_count == 1);
    assert(strcmp(document.paths[0].provenance.layer, "0") == 0);
    assert(document.paths[0].provenance.handle == NULL);
    cad_ir_document_destroy(&document);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures_are_transactional(void)
{
    static const char dxf[] =
        DXF_BEGIN("AC1032", "4")
        "0\nLINE\n5\n10\n8\nREF\n10\n0\n20\n0\n11\n100\n21\n0\n"
        "0\nLWPOLYLINE\n8\nREF\n90\n3\n70\n1\n43\n2\n"
        "10\n0\n20\n0\n10\n20\n20\n0\n10\n20\n20\n20\n"
        "0\nARC\n8\nOTHER\n5\n11\n"
        DXF_END;
    size_t failures = 0;
    for (size_t n = 0; ; n++) {
        CadIrDocument document = {0};
        assert(cad_ir_document_set_metadata(&document, "OLD", "OLD1",
            CAD_IR_UNIT_METRE) == CAD_IR_SUCCESS);
        char *old_format = document.source_format;
        char *old_version = document.source_version;

        fail_after = n;
        allocation_failed = 0;
        DxfAsciiDecodeResult status = decode_text(dxf, &document);
        fail_after = SIZE_MAX;

        if (allocation_failed) {
            failures++;
            assert(status.code == DXF_ASCII_DECODE_ALLOCATION_FAILED);
            assert(document.source_format == old_format && document.source_version == old_version);
            assert(strcmp(document.source_format, "OLD") == 0);
        } else {
            assert(status.code == DXF_ASCII_DECODE_SUCCESS);
            assert(strcmp(document.source_format, "DXF") == 0);
            cad_ir_document_destroy(&document);
            break;
        }
        cad_ir_document_destroy(&document);
        assert(n < 128);
    }
    assert(failures >= 10);
    printf("dxf-ascii allocation failures checked: %zu\n", failures);
}
#endif

int main(void)
{
    test_valid_line_exact_decimal_and_provenance();
    test_lwpolyline_open_closed_curve_and_width();
    test_skip_reasons_and_unsupported_entities();
    test_numeric_and_malformed_entities_are_partial_success();
    test_units_metadata_only();
    test_file_level_failures_are_transactional();
    test_success_replaces_destination_and_crlf_is_accepted();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures_are_transactional();
#endif
    puts("dxf-ascii tests passed");
    return 0;
}
