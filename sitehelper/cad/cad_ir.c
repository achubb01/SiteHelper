#include "cad_ir.h"

#include <stdlib.h>
#include <string.h>

static int nonempty(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static int unit_valid(CadIrSourceUnit unit)
{
    return unit >= CAD_IR_UNIT_UNSPECIFIED && unit <= CAD_IR_UNIT_OTHER;
}

static int severity_valid(CadIrDiagnosticSeverity severity)
{
    return severity >= CAD_IR_DIAGNOSTIC_INFO &&
        severity <= CAD_IR_DIAGNOSTIC_ERROR;
}

static int collection_valid(const void *items, size_t count, size_t capacity,
    size_t item_size)
{
    if (count > capacity) { return 0; }
    if ((capacity == 0) != (items == NULL)) { return 0; }
    return capacity <= SIZE_MAX / item_size;
}

static int document_storage_valid(const CadIrDocument *document)
{
    return document != NULL &&
        ((document->source_format == NULL) == (document->source_version == NULL)) &&
        unit_valid(document->declared_unit) &&
        document->skipped_entity_count <= document->decoded_entity_count &&
        collection_valid(document->paths, document->path_count,
            document->path_capacity, sizeof *document->paths) &&
        collection_valid(document->diagnostics, document->diagnostic_count,
            document->diagnostic_capacity, sizeof *document->diagnostics);
}

static CadIrCode duplicate_string(const char *source, int optional, char **output)
{
    *output = NULL;
    if (source == NULL) {
        return optional ? CAD_IR_SUCCESS : CAD_IR_INVALID_ARGUMENT;
    }
    if (source[0] == '\0') { return CAD_IR_INVALID_ARGUMENT; }
    size_t length = strlen(source);
    if (length == SIZE_MAX) { return CAD_IR_NUMERIC_OVERFLOW; }
    char *copy = malloc(length + 1);
    if (copy == NULL) { return CAD_IR_ALLOCATION_FAILED; }
    memcpy(copy, source, length + 1);
    *output = copy;
    return CAD_IR_SUCCESS;
}

static void provenance_destroy(CadIrProvenance *provenance)
{
    if (provenance == NULL) { return; }
    free(provenance->entity_kind);
    free(provenance->layer);
    free(provenance->handle);
    *provenance = (CadIrProvenance){0};
}

static int provenance_input_valid(const CadIrProvenanceInput *input)
{
    return input != NULL && nonempty(input->entity_kind) && nonempty(input->layer) &&
        (input->handle == NULL || nonempty(input->handle));
}

static CadIrCode provenance_clone(const CadIrProvenanceInput *input,
    CadIrProvenance *output)
{
    if (!provenance_input_valid(input) || output == NULL) {
        return CAD_IR_INVALID_ARGUMENT;
    }
    CadIrProvenance copy = {.entity_ordinal = input->entity_ordinal};
    CadIrCode status = duplicate_string(input->entity_kind, 0, &copy.entity_kind);
    if (status != CAD_IR_SUCCESS) { goto cleanup; }
    status = duplicate_string(input->layer, 0, &copy.layer);
    if (status != CAD_IR_SUCCESS) { goto cleanup; }
    status = duplicate_string(input->handle, 1, &copy.handle);
    if (status != CAD_IR_SUCCESS) { goto cleanup; }
    *output = copy;
    return CAD_IR_SUCCESS;
cleanup:
    provenance_destroy(&copy);
    return status;
}

static void path_destroy(CadIrPath *path)
{
    if (path == NULL) { return; }
    free(path->vertices);
    provenance_destroy(&path->provenance);
    *path = (CadIrPath){0};
}

static void diagnostic_destroy(CadIrDiagnostic *diagnostic)
{
    if (diagnostic == NULL) { return; }
    free(diagnostic->code);
    free(diagnostic->detail);
    provenance_destroy(&diagnostic->provenance);
    *diagnostic = (CadIrDiagnostic){0};
}

static CadIrCode ensure_path_capacity(CadIrDocument *document, size_t required)
{
    if (required <= document->path_capacity) { return CAD_IR_SUCCESS; }
    const size_t maximum = SIZE_MAX / sizeof *document->paths;
    if (required > maximum) { return CAD_IR_NUMERIC_OVERFLOW; }
    size_t capacity = document->path_capacity == 0 ? 1 : document->path_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) {
            capacity = maximum;
            break;
        }
        capacity *= 2;
    }
    CadIrPath *storage = realloc(document->paths, capacity * sizeof *storage);
    if (storage == NULL) { return CAD_IR_ALLOCATION_FAILED; }
    document->paths = storage;
    document->path_capacity = capacity;
    return CAD_IR_SUCCESS;
}

static CadIrCode ensure_diagnostic_capacity(CadIrDocument *document, size_t required)
{
    if (required <= document->diagnostic_capacity) { return CAD_IR_SUCCESS; }
    const size_t maximum = SIZE_MAX / sizeof *document->diagnostics;
    if (required > maximum) { return CAD_IR_NUMERIC_OVERFLOW; }
    size_t capacity = document->diagnostic_capacity == 0 ? 1 : document->diagnostic_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) {
            capacity = maximum;
            break;
        }
        capacity *= 2;
    }
    CadIrDiagnostic *storage = realloc(document->diagnostics,
        capacity * sizeof *storage);
    if (storage == NULL) { return CAD_IR_ALLOCATION_FAILED; }
    document->diagnostics = storage;
    document->diagnostic_capacity = capacity;
    return CAD_IR_SUCCESS;
}

void cad_ir_document_init(CadIrDocument *document)
{
    if (document != NULL) { *document = (CadIrDocument){0}; }
}

void cad_ir_document_destroy(CadIrDocument *document)
{
    if (document == NULL) { return; }
    free(document->source_format);
    free(document->source_version);
    for (size_t i = 0; i < document->path_count; i++) {
        path_destroy(&document->paths[i]);
    }
    free(document->paths);
    for (size_t i = 0; i < document->diagnostic_count; i++) {
        diagnostic_destroy(&document->diagnostics[i]);
    }
    free(document->diagnostics);
    *document = (CadIrDocument){0};
}

CadIrCode cad_ir_document_set_metadata(CadIrDocument *document,
    const char *source_format, const char *source_version,
    CadIrSourceUnit declared_unit)
{
    if (document == NULL || source_format == NULL || source_version == NULL) {
        return CAD_IR_INVALID_ARGUMENT;
    }
    if (!document_storage_valid(document)) { return CAD_IR_INVALID_STATE; }
    if (!nonempty(source_format) || !nonempty(source_version) || !unit_valid(declared_unit)) {
        return CAD_IR_INVALID_METADATA;
    }

    char *format_copy = NULL;
    char *version_copy = NULL;
    CadIrCode status = duplicate_string(source_format, 0, &format_copy);
    if (status != CAD_IR_SUCCESS) { return status; }
    status = duplicate_string(source_version, 0, &version_copy);
    if (status != CAD_IR_SUCCESS) {
        free(format_copy);
        return status;
    }

    free(document->source_format);
    free(document->source_version);
    document->source_format = format_copy;
    document->source_version = version_copy;
    document->declared_unit = declared_unit;
    return CAD_IR_SUCCESS;
}

CadIrCode cad_ir_document_append_path(CadIrDocument *document,
    const CadIrPathInput *path)
{
    if (document == NULL || path == NULL) { return CAD_IR_INVALID_ARGUMENT; }
    if (!document_storage_valid(document)) { return CAD_IR_INVALID_STATE; }
    if ((path->closed != 0 && path->closed != 1) ||
        path->vertices == NULL || path->vertex_count < 2 ||
        (path->closed && path->vertex_count < 3) ||
        !provenance_input_valid(&path->provenance)) {
        return CAD_IR_INVALID_PATH;
    }
    if (path->vertex_count > SIZE_MAX / sizeof *path->vertices) {
        return CAD_IR_NUMERIC_OVERFLOW;
    }
    if (document->path_count == SIZE_MAX) { return CAD_IR_NUMERIC_OVERFLOW; }

    CadIrPath copy = {.vertex_count = path->vertex_count, .closed = path->closed};
    copy.vertices = malloc(copy.vertex_count * sizeof *copy.vertices);
    if (copy.vertices == NULL) { return CAD_IR_ALLOCATION_FAILED; }
    memcpy(copy.vertices, path->vertices, copy.vertex_count * sizeof *copy.vertices);
    CadIrCode status = provenance_clone(&path->provenance, &copy.provenance);
    if (status != CAD_IR_SUCCESS) {
        path_destroy(&copy);
        return status == CAD_IR_INVALID_ARGUMENT ? CAD_IR_INVALID_PATH : status;
    }
    status = ensure_path_capacity(document, document->path_count + 1);
    if (status != CAD_IR_SUCCESS) {
        path_destroy(&copy);
        return status;
    }
    document->paths[document->path_count++] = copy;
    return CAD_IR_SUCCESS;
}

CadIrCode cad_ir_document_append_diagnostic(CadIrDocument *document,
    const CadIrDiagnosticInput *diagnostic)
{
    if (document == NULL || diagnostic == NULL) { return CAD_IR_INVALID_ARGUMENT; }
    if (!document_storage_valid(document)) { return CAD_IR_INVALID_STATE; }
    if (!severity_valid(diagnostic->severity) || !nonempty(diagnostic->code) ||
        (diagnostic->detail != NULL && !nonempty(diagnostic->detail)) ||
        (diagnostic->has_source != 0 && diagnostic->has_source != 1) ||
        (diagnostic->has_source && !provenance_input_valid(&diagnostic->provenance))) {
        return CAD_IR_INVALID_DIAGNOSTIC;
    }
    if (document->diagnostic_count == SIZE_MAX) { return CAD_IR_NUMERIC_OVERFLOW; }

    CadIrDiagnostic copy = {
        .severity = diagnostic->severity,
        .has_source = diagnostic->has_source
    };
    CadIrCode status = duplicate_string(diagnostic->code, 0, &copy.code);
    if (status != CAD_IR_SUCCESS) { goto cleanup; }
    status = duplicate_string(diagnostic->detail, 1, &copy.detail);
    if (status != CAD_IR_SUCCESS) { goto cleanup; }
    if (copy.has_source) {
        status = provenance_clone(&diagnostic->provenance, &copy.provenance);
        if (status != CAD_IR_SUCCESS) {
            if (status == CAD_IR_INVALID_ARGUMENT) { status = CAD_IR_INVALID_DIAGNOSTIC; }
            goto cleanup;
        }
    }
    status = ensure_diagnostic_capacity(document, document->diagnostic_count + 1);
    if (status != CAD_IR_SUCCESS) { goto cleanup; }
    document->diagnostics[document->diagnostic_count++] = copy;
    return CAD_IR_SUCCESS;
cleanup:
    diagnostic_destroy(&copy);
    return status;
}

CadIrCode cad_ir_document_set_entity_counts(CadIrDocument *document,
    size_t decoded, size_t skipped)
{
    if (document == NULL) { return CAD_IR_INVALID_ARGUMENT; }
    if (!document_storage_valid(document)) { return CAD_IR_INVALID_STATE; }
    if (skipped > decoded) { return CAD_IR_INVALID_COUNTS; }
    document->decoded_entity_count = decoded;
    document->skipped_entity_count = skipped;
    return CAD_IR_SUCCESS;
}

CadIrStatistics cad_ir_document_statistics(const CadIrDocument *document)
{
    CadIrStatistics statistics = {0};
    if (!document_storage_valid(document)) { return statistics; }
    statistics.decoded_entity_count = document->decoded_entity_count;
    statistics.source_path_count = document->path_count;
    statistics.skipped_entity_count = document->skipped_entity_count;
    for (size_t i = 0; i < document->diagnostic_count; i++) {
        switch (document->diagnostics[i].severity) {
            case CAD_IR_DIAGNOSTIC_INFO: statistics.info_count++; break;
            case CAD_IR_DIAGNOSTIC_WARNING: statistics.warning_count++; break;
            case CAD_IR_DIAGNOSTIC_ERROR: statistics.error_count++; break;
            default: break;
        }
    }
    return statistics;
}
