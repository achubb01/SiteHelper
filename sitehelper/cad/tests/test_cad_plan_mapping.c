#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cad_plan_mapping.h"
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

static CadIrPoint2 point(int64_t xc, int32_t xe, int64_t yc, int32_t ye)
{
    return (CadIrPoint2){{xc, xe}, {yc, ye}};
}

static void add_line(CadIrDocument *document, CadIrPoint2 a, CadIrPoint2 b,
    size_t ordinal, const char *layer, const char *handle)
{
    CadIrPoint2 vertices[2] = {a, b};
    CadIrPathInput path = {
        .vertices = vertices, .vertex_count = 2, .closed = 0,
        .provenance = {ordinal, "LINE", layer, handle}
    };
    assert(cad_ir_document_append_path(document, &path) == CAD_IR_SUCCESS);
}

static CadPlanMappingConfig default_config(void)
{
    return (CadPlanMappingConfig){0};
}

static const CadIrDiagnostic *find_diag(const CadPlanReference *reference,
    const char *code, size_t ordinal)
{
    for (size_t i = 0; i < reference->diagnostic_count; i++) {
        const CadIrDiagnostic *diagnostic = &reference->diagnostics[i];
        if (strcmp(diagnostic->code, code) == 0 &&
            (ordinal == SIZE_MAX || (diagnostic->has_source &&
            diagnostic->provenance.entity_ordinal == ordinal))) {
            return diagnostic;
        }
    }
    return NULL;
}

static void test_exact_metric_mapping_translation_and_lifetime(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_METRE) == CAD_IR_SUCCESS);
    add_line(&source, point(42, -1, -125, -2), point(5, 0, 2, -1),
        7, "REFERENCE", "0007");
    assert(cad_ir_document_set_entity_counts(&source, 1, 0) == CAD_IR_SUCCESS);

    CadPlanMappingConfig config = default_config();
    config.translation_mm = (PlanPosition){100, -200};
    CadPlanReference reference = {0};
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.status == CAD_PLAN_REFERENCE_READY);
    assert(reference.declared_unit == CAD_IR_UNIT_METRE);
    assert(reference.resolved_unit == CAD_IR_UNIT_METRE);
    assert(reference.unit_resolution == CAD_PLAN_UNIT_FROM_SOURCE);
    assert(reference.path_count == 1 && reference.source_path_count == 1);
    assert(reference.paths[0].vertices[0].x == 4300);
    assert(reference.paths[0].vertices[0].y == -1450);
    assert(reference.paths[0].vertices[1].x == 5100);
    assert(reference.paths[0].vertices[1].y == 0);
    assert(strcmp(reference.paths[0].provenance.layer, "REFERENCE") == 0);
    assert(strcmp(reference.paths[0].provenance.handle, "0007") == 0);
    assert(strcmp(reference.source_format, "DXF") == 0);
    assert(strcmp(reference.source_version, "AC1032") == 0);

    cad_ir_document_destroy(&source);
    assert(reference.paths[0].vertices[0].x == 4300);
    assert(strcmp(reference.paths[0].provenance.layer, "REFERENCE") == 0);
    cad_plan_reference_destroy(&reference);
}

static void test_units_required_and_explicit_override(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1018",
        CAD_IR_UNIT_UNSPECIFIED) == CAD_IR_SUCCESS);
    add_line(&source, point(0, 0, 0, 0), point(4200, 0, 0, 0), 1, "0", NULL);
    assert(cad_ir_document_set_entity_counts(&source, 1, 0) == CAD_IR_SUCCESS);

    CadPlanReference reference = {0};
    CadPlanMappingConfig config = default_config();
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.status == CAD_PLAN_REFERENCE_BLOCKED_UNITS);
    assert(reference.path_count == 0);
    assert(reference.resolved_unit == CAD_IR_UNIT_UNSPECIFIED);
    assert(reference.unit_resolution == CAD_PLAN_UNIT_RESOLUTION_NONE);
    const CadIrDiagnostic *required = find_diag(&reference, "UNITS_REQUIRED", SIZE_MAX);
    assert(required != NULL && required->severity == CAD_IR_DIAGNOSTIC_ERROR &&
        required->has_source == 0);

    config.has_unit_override = 1;
    config.unit_override = CAD_IR_UNIT_MILLIMETRE;
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.status == CAD_PLAN_REFERENCE_READY);
    assert(reference.unit_resolution == CAD_PLAN_UNIT_FROM_OVERRIDE);
    assert(reference.resolved_unit == CAD_IR_UNIT_MILLIMETRE);
    assert(reference.unit_override_conflicted == 0);
    assert(reference.path_count == 1 && reference.paths[0].vertices[1].x == 4200);
    assert(find_diag(&reference, "UNITS_REQUIRED", SIZE_MAX) == NULL);

    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);
}

static void test_override_conflict_and_source_diagnostics_are_preserved(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1027",
        CAD_IR_UNIT_METRE) == CAD_IR_SUCCESS);
    add_line(&source, point(10, 0, 0, 0), point(20, 0, 0, 0), 2, "WALLS", "2A");
    CadIrDiagnosticInput diagnostic = {
        .severity = CAD_IR_DIAGNOSTIC_WARNING,
        .code = "ENTITY_UNSUPPORTED_KIND", .detail = "ARC", .has_source = 1,
        .provenance = {9, "ARC", "DETAIL", "9A"}
    };
    assert(cad_ir_document_append_diagnostic(&source, &diagnostic) == CAD_IR_SUCCESS);
    assert(cad_ir_document_set_entity_counts(&source, 2, 1) == CAD_IR_SUCCESS);

    CadPlanMappingConfig config = default_config();
    config.has_unit_override = 1;
    config.unit_override = CAD_IR_UNIT_CENTIMETRE;
    CadPlanReference reference = {0};
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.unit_override_conflicted == 1);
    assert(reference.paths[0].vertices[0].x == 100);
    assert(find_diag(&reference, "UNITS_OVERRIDE_CONFLICT", SIZE_MAX) != NULL);
    const CadIrDiagnostic *copied = find_diag(&reference, "ENTITY_UNSUPPORTED_KIND", 9);
    assert(copied != NULL && strcmp(copied->detail, "ARC") == 0);
    assert(strcmp(copied->provenance.layer, "DETAIL") == 0);
    assert(reference.decoded_entity_count == 2 && reference.adapter_skipped_entity_count == 1);

    cad_ir_document_destroy(&source);
    assert(strcmp(copied->provenance.handle, "9A") == 0);
    cad_plan_reference_destroy(&reference);
}

static void test_inches_and_feet_exactness(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_INCH) == CAD_IR_SUCCESS);
    add_line(&source, point(10, 0, 0, 0), point(20, 0, 10, 0), 1, "EXACT", NULL);
    add_line(&source, point(1, 0, 0, 0), point(2, 0, 0, 0), 2, "FRACTIONAL", NULL);
    add_line(&source, point(5, -1, 0, 0), point(10, -1, 0, 0), 3, "HALF-INCH", NULL);
    assert(cad_ir_document_set_entity_counts(&source, 3, 0) == CAD_IR_SUCCESS);

    CadPlanReference reference = {0};
    CadPlanMappingConfig config = default_config();
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.path_count == 1);
    assert(reference.paths[0].vertices[0].x == 254);
    assert(reference.paths[0].vertices[1].x == 508);
    assert(reference.mapping_skipped_path_count == 2);
    assert(find_diag(&reference, "NUMERIC_NON_WHOLE_MM", 2) != NULL);
    assert(find_diag(&reference, "NUMERIC_NON_WHOLE_MM", 3) != NULL);
    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);

    source = (CadIrDocument){0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_FOOT) == CAD_IR_SUCCESS);
    add_line(&source, point(5, 0, 0, 0), point(10, 0, 5, 0), 4, "FEET", NULL);
    config = default_config();
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.path_count == 1);
    assert(reference.paths[0].vertices[0].x == 1524);
    assert(reference.paths[0].vertices[1].x == 3048);
    assert(reference.paths[0].vertices[1].y == 1524);
    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);
}

static void test_numeric_overflow_is_local_and_translation_is_checked(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_MILLIMETRE) == CAD_IR_SUCCESS);
    add_line(&source, point(INT_MAX, 0, 0, 0), point(INT_MAX, 0, 1, 0),
        1, "TRANSLATE", NULL);
    add_line(&source, point(1, 20, 0, 0), point(2, 20, 0, 0),
        2, "SOURCE-OVERFLOW", NULL);
    add_line(&source, point(0, 0, 0, 0), point(100, 0, 100, 0),
        3, "GOOD", NULL);

    CadPlanMappingConfig config = default_config();
    config.translation_mm = (PlanPosition){1, 0};
    CadPlanReference reference = {0};
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.path_count == 1 && reference.mapping_skipped_path_count == 2);
    assert(strcmp(reference.paths[0].provenance.layer, "GOOD") == 0);
    assert(reference.paths[0].vertices[0].x == 1);
    assert(reference.paths[0].vertices[1].x == 101);
    assert(find_diag(&reference, "NUMERIC_OVERFLOW", 1) != NULL);
    assert(find_diag(&reference, "NUMERIC_OVERFLOW", 2) != NULL);
    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);
}

static void test_exact_layer_filtering(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_MILLIMETRE) == CAD_IR_SUCCESS);
    add_line(&source, point(0,0,0,0), point(1,0,0,0), 1, "A-WALL", NULL);
    add_line(&source, point(0,0,1,0), point(1,0,1,0), 2, "a-wall", NULL);
    add_line(&source, point(0,0,2,0), point(1,0,2,0), 3, "GRID", NULL);

    const char *include[] = {"A-WALL"};
    CadPlanMappingConfig config = default_config();
    config.layers = (CadPlanLayerFilter){CAD_PLAN_LAYER_INCLUDE, include, 1};
    CadPlanReference reference = {0};
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.path_count == 1 && reference.filtered_path_count == 2);
    assert(reference.paths[0].provenance.entity_ordinal == 1);

    const char *exclude[] = {"GRID"};
    config.layers = (CadPlanLayerFilter){CAD_PLAN_LAYER_EXCLUDE, exclude, 1};
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.path_count == 2 && reference.filtered_path_count == 1);
    assert(reference.paths[0].provenance.entity_ordinal == 1);
    assert(reference.paths[1].provenance.entity_ordinal == 2);

    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);
}

static void test_decoder_to_mapper_acceptance_fixture(void)
{
    static const char dxf[] =
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1032\n"
        "9\n$INSUNITS\n70\n6\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n2A\n8\nPLAN\n10\n4.2\n20\n0\n11\n5.75\n21\n2.5\n"
        "0\nENDSEC\n0\nEOF\n";
    CadIrDocument source = {0};
    assert(dxf_ascii_decode_memory(dxf, strlen(dxf), &source).code ==
        DXF_ASCII_DECODE_SUCCESS);
    CadPlanReference reference = {0};
    CadPlanMappingConfig config = default_config();
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    assert(reference.path_count == 1);
    assert(reference.paths[0].vertices[0].x == 4200);
    assert(reference.paths[0].vertices[1].x == 5750);
    assert(reference.paths[0].vertices[1].y == 2500);
    assert(strcmp(reference.paths[0].provenance.handle, "2A") == 0);
    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);
}

static void test_statistics_and_invalid_arguments(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_MILLIMETRE) == CAD_IR_SUCCESS);
    add_line(&source, point(1, -1, 0, 0), point(2, -1, 0, 0), 1, "BAD", NULL);
    CadIrDiagnosticInput adapter = {
        .severity = CAD_IR_DIAGNOSTIC_WARNING, .code = "STYLE_NOT_PRESERVED",
        .detail = "width", .has_source = 0
    };
    assert(cad_ir_document_append_diagnostic(&source, &adapter) == CAD_IR_SUCCESS);
    assert(cad_ir_document_set_entity_counts(&source, 3, 2) == CAD_IR_SUCCESS);

    CadPlanReference reference = {0};
    CadPlanMappingConfig config = default_config();
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_SUCCESS);
    CadPlanReferenceStatistics stats = cad_plan_reference_statistics(&reference);
    assert(stats.decoded_entity_count == 3 && stats.adapter_skipped_entity_count == 2);
    assert(stats.source_path_count == 1 && stats.emitted_path_count == 0);
    assert(stats.mapping_skipped_path_count == 1 && stats.filtered_path_count == 0);
    assert(stats.warning_count == 1 && stats.error_count == 1 && stats.info_count == 0);
    CadPlanReferenceStatistics zero = cad_plan_reference_statistics(NULL);
    assert(memcmp(&zero, &(CadPlanReferenceStatistics){0}, sizeof zero) == 0);

    assert(cad_plan_map_reference(NULL, &config, &reference) == CAD_PLAN_MAP_INVALID_ARGUMENT);
    assert(cad_plan_map_reference(&source, NULL, &reference) == CAD_PLAN_MAP_INVALID_ARGUMENT);
    assert(cad_plan_map_reference(&source, &config, NULL) == CAD_PLAN_MAP_INVALID_ARGUMENT);
    config.has_unit_override = 1;
    config.unit_override = CAD_IR_UNIT_OTHER;
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_INVALID_ARGUMENT);
    config = default_config();
    config.layers.mode = CAD_PLAN_LAYER_INCLUDE;
    assert(cad_plan_map_reference(&source, &config, &reference) == CAD_PLAN_MAP_INVALID_ARGUMENT);

    CadIrDocument malformed = source;
    malformed.paths = NULL;
    assert(cad_plan_map_reference(&malformed, &(CadPlanMappingConfig){0}, &reference) ==
        CAD_PLAN_MAP_INVALID_SOURCE);

    cad_plan_reference_destroy(&reference);
    cad_ir_document_destroy(&source);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures_are_transactional(void)
{
    CadIrDocument source = {0};
    assert(cad_ir_document_set_metadata(&source, "DXF", "AC1032",
        CAD_IR_UNIT_METRE) == CAD_IR_SUCCESS);
    add_line(&source, point(0,0,0,0), point(42,-1,5,0), 1, "A", "1A");
    CadIrDiagnosticInput diagnostic = {
        .severity = CAD_IR_DIAGNOSTIC_WARNING, .code = "STYLE_NOT_PRESERVED",
        .detail = "width", .has_source = 1,
        .provenance = {2, "LWPOLYLINE", "B", "2B"}
    };
    assert(cad_ir_document_append_diagnostic(&source, &diagnostic) == CAD_IR_SUCCESS);

    CadPlanReference output = {0};
    CadPlanMappingConfig seed = default_config();
    seed.has_unit_override = 1;
    seed.unit_override = CAD_IR_UNIT_MILLIMETRE;
    assert(cad_plan_map_reference(&source, &seed, &output) == CAD_PLAN_MAP_SUCCESS);
    CadPlanReferencePath *old_paths = output.paths;
    CadIrDiagnostic *old_diagnostics = output.diagnostics;
    char *old_format = output.source_format;
    size_t old_path_count = output.path_count;
    size_t old_diagnostic_count = output.diagnostic_count;

    CadPlanMappingConfig config = default_config();
    config.has_unit_override = 1;
    config.unit_override = CAD_IR_UNIT_CENTIMETRE; /* conflict adds mapping diagnostic */
    size_t failures = 0;
    for (size_t attempt = 0; attempt < 256; attempt++) {
        fail_after = attempt;
        allocation_failed = 0;
        CadPlanMapCode status = cad_plan_map_reference(&source, &config, &output);
        fail_after = SIZE_MAX;
        if (status == CAD_PLAN_MAP_SUCCESS) { break; }
        assert(status == CAD_PLAN_MAP_ALLOCATION_FAILED);
        assert(allocation_failed);
        assert(output.paths == old_paths && output.diagnostics == old_diagnostics);
        assert(output.source_format == old_format);
        assert(output.path_count == old_path_count &&
            output.diagnostic_count == old_diagnostic_count);
        failures++;
    }
    assert(failures >= 10);
    assert(output.status == CAD_PLAN_REFERENCE_READY);
    assert(output.unit_resolution == CAD_PLAN_UNIT_FROM_OVERRIDE);
    assert(output.unit_override_conflicted == 1);
    assert(find_diag(&output, "UNITS_OVERRIDE_CONFLICT", SIZE_MAX) != NULL);
    printf("cad_plan_mapping allocation failure points: %zu\n", failures);

    cad_plan_reference_destroy(&output);
    cad_ir_document_destroy(&source);
}
#endif

int main(void)
{
    test_exact_metric_mapping_translation_and_lifetime();
    test_units_required_and_explicit_override();
    test_override_conflict_and_source_diagnostics_are_preserved();
    test_inches_and_feet_exactness();
    test_numeric_overflow_is_local_and_translation_is_checked();
    test_exact_layer_filtering();
    test_decoder_to_mapper_acceptance_fixture();
    test_statistics_and_invalid_arguments();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures_are_transactional();
#endif
    puts("cad plan mapping tests passed");
    return 0;
}
