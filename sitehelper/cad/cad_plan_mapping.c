#include "cad_plan_mapping.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int nonempty(const char *text)
{
    return text != NULL && text[0] != '\0';
}

static int physical_unit(CadIrSourceUnit unit)
{
    return unit == CAD_IR_UNIT_INCH || unit == CAD_IR_UNIT_FOOT ||
        unit == CAD_IR_UNIT_MILLIMETRE || unit == CAD_IR_UNIT_CENTIMETRE ||
        unit == CAD_IR_UNIT_METRE;
}

static int source_unit_valid(CadIrSourceUnit unit)
{
    return unit >= CAD_IR_UNIT_UNSPECIFIED && unit <= CAD_IR_UNIT_OTHER;
}

static int severity_valid(CadIrDiagnosticSeverity severity)
{
    return severity == CAD_IR_DIAGNOSTIC_INFO ||
        severity == CAD_IR_DIAGNOSTIC_WARNING ||
        severity == CAD_IR_DIAGNOSTIC_ERROR;
}

static void provenance_destroy(CadIrProvenance *provenance)
{
    if (provenance == NULL) { return; }
    free(provenance->entity_kind);
    free(provenance->layer);
    free(provenance->handle);
    *provenance = (CadIrProvenance){0};
}

static void path_destroy(CadPlanReferencePath *path)
{
    if (path == NULL) { return; }
    free(path->vertices);
    provenance_destroy(&path->provenance);
    *path = (CadPlanReferencePath){0};
}

static void diagnostic_destroy(CadIrDiagnostic *diagnostic)
{
    if (diagnostic == NULL) { return; }
    free(diagnostic->code);
    free(diagnostic->detail);
    provenance_destroy(&diagnostic->provenance);
    *diagnostic = (CadIrDiagnostic){0};
}

void cad_plan_reference_init(CadPlanReference *reference)
{
    if (reference != NULL) { *reference = (CadPlanReference){0}; }
}

void cad_plan_reference_destroy(CadPlanReference *reference)
{
    if (reference == NULL) { return; }
    free(reference->source_format);
    free(reference->source_version);
    for (size_t i = 0; i < reference->path_count; i++) {
        path_destroy(&reference->paths[i]);
    }
    free(reference->paths);
    for (size_t i = 0; i < reference->diagnostic_count; i++) {
        diagnostic_destroy(&reference->diagnostics[i]);
    }
    free(reference->diagnostics);
    *reference = (CadPlanReference){0};
}

static CadPlanMapCode duplicate_string(const char *source, int optional, char **output)
{
    *output = NULL;
    if (source == NULL) { return optional ? CAD_PLAN_MAP_SUCCESS : CAD_PLAN_MAP_INVALID_SOURCE; }
    if (source[0] == '\0') { return CAD_PLAN_MAP_INVALID_SOURCE; }
    size_t length = strlen(source);
    if (length == SIZE_MAX) { return CAD_PLAN_MAP_NUMERIC_OVERFLOW; }
    char *copy = malloc(length + 1);
    if (copy == NULL) { return CAD_PLAN_MAP_ALLOCATION_FAILED; }
    memcpy(copy, source, length + 1);
    *output = copy;
    return CAD_PLAN_MAP_SUCCESS;
}

static int provenance_valid(const CadIrProvenance *provenance)
{
    return provenance != NULL && nonempty(provenance->entity_kind) &&
        nonempty(provenance->layer) &&
        (provenance->handle == NULL || nonempty(provenance->handle));
}

static CadPlanMapCode provenance_clone(const CadIrProvenance *source,
    CadIrProvenance *output)
{
    if (!provenance_valid(source) || output == NULL) { return CAD_PLAN_MAP_INVALID_SOURCE; }
    CadIrProvenance copy = {.entity_ordinal = source->entity_ordinal};
    CadPlanMapCode status = duplicate_string(source->entity_kind, 0, &copy.entity_kind);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    status = duplicate_string(source->layer, 0, &copy.layer);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    status = duplicate_string(source->handle, 1, &copy.handle);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    *output = copy;
    return CAD_PLAN_MAP_SUCCESS;
fail:
    provenance_destroy(&copy);
    return status;
}

static int diagnostic_valid(const CadIrDiagnostic *diagnostic)
{
    return diagnostic != NULL && severity_valid(diagnostic->severity) &&
        nonempty(diagnostic->code) &&
        (diagnostic->detail == NULL || nonempty(diagnostic->detail)) &&
        (diagnostic->has_source == 0 || diagnostic->has_source == 1) &&
        (!diagnostic->has_source || provenance_valid(&diagnostic->provenance));
}

static int source_valid(const CadIrDocument *source)
{
    if (source == NULL || !nonempty(source->source_format) ||
        !nonempty(source->source_version) || !source_unit_valid(source->declared_unit) ||
        source->skipped_entity_count > source->decoded_entity_count ||
        (source->path_count > 0 && source->paths == NULL) ||
        (source->diagnostic_count > 0 && source->diagnostics == NULL)) {
        return 0;
    }
    for (size_t i = 0; i < source->path_count; i++) {
        const CadIrPath *path = &source->paths[i];
        if (path->vertices == NULL || path->vertex_count < 2 ||
            (path->closed != 0 && path->closed != 1) ||
            (path->closed && path->vertex_count < 3) ||
            !provenance_valid(&path->provenance)) {
            return 0;
        }
    }
    for (size_t i = 0; i < source->diagnostic_count; i++) {
        if (!diagnostic_valid(&source->diagnostics[i])) { return 0; }
    }
    return 1;
}

static int config_valid(const CadPlanMappingConfig *config)
{
    if (config == NULL || (config->has_unit_override != 0 && config->has_unit_override != 1)) {
        return 0;
    }
    if (config->has_unit_override && !physical_unit(config->unit_override)) { return 0; }
    if (config->layers.mode < CAD_PLAN_LAYER_ALL ||
        config->layers.mode > CAD_PLAN_LAYER_EXCLUDE) {
        return 0;
    }
    if (config->layers.mode == CAD_PLAN_LAYER_ALL) {
        return config->layers.layer_name_count == 0;
    }
    if (config->layers.layer_names == NULL || config->layers.layer_name_count == 0) {
        return 0;
    }
    for (size_t i = 0; i < config->layers.layer_name_count; i++) {
        if (!nonempty(config->layers.layer_names[i])) { return 0; }
    }
    return 1;
}

static CadPlanMapCode ensure_path_capacity(CadPlanReference *reference, size_t required)
{
    if (required <= reference->path_capacity) { return CAD_PLAN_MAP_SUCCESS; }
    if (required > SIZE_MAX / sizeof *reference->paths) { return CAD_PLAN_MAP_NUMERIC_OVERFLOW; }
    size_t maximum = SIZE_MAX / sizeof *reference->paths;
    size_t capacity = reference->path_capacity == 0 ? 1 : reference->path_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) { capacity = maximum; break; }
        capacity *= 2;
    }
    CadPlanReferencePath *storage = realloc(reference->paths, capacity * sizeof *storage);
    if (storage == NULL) { return CAD_PLAN_MAP_ALLOCATION_FAILED; }
    reference->paths = storage;
    reference->path_capacity = capacity;
    return CAD_PLAN_MAP_SUCCESS;
}

static CadPlanMapCode ensure_diagnostic_capacity(CadPlanReference *reference, size_t required)
{
    if (required <= reference->diagnostic_capacity) { return CAD_PLAN_MAP_SUCCESS; }
    if (required > SIZE_MAX / sizeof *reference->diagnostics) { return CAD_PLAN_MAP_NUMERIC_OVERFLOW; }
    size_t maximum = SIZE_MAX / sizeof *reference->diagnostics;
    size_t capacity = reference->diagnostic_capacity == 0 ? 1 : reference->diagnostic_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) { capacity = maximum; break; }
        capacity *= 2;
    }
    CadIrDiagnostic *storage = realloc(reference->diagnostics,
        capacity * sizeof *storage);
    if (storage == NULL) { return CAD_PLAN_MAP_ALLOCATION_FAILED; }
    reference->diagnostics = storage;
    reference->diagnostic_capacity = capacity;
    return CAD_PLAN_MAP_SUCCESS;
}

static CadPlanMapCode append_diagnostic(CadPlanReference *reference,
    CadIrDiagnosticSeverity severity, const char *code, const char *detail,
    const CadIrProvenance *provenance)
{
    if (reference->diagnostic_count == SIZE_MAX) { return CAD_PLAN_MAP_NUMERIC_OVERFLOW; }
    CadIrDiagnostic copy = {
        .severity = severity,
        .has_source = provenance != NULL
    };
    CadPlanMapCode status = duplicate_string(code, 0, &copy.code);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    status = duplicate_string(detail, 1, &copy.detail);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    if (provenance != NULL) {
        status = provenance_clone(provenance, &copy.provenance);
        if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    }
    status = ensure_diagnostic_capacity(reference, reference->diagnostic_count + 1);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    reference->diagnostics[reference->diagnostic_count++] = copy;
    return CAD_PLAN_MAP_SUCCESS;
fail:
    diagnostic_destroy(&copy);
    return status;
}

static CadPlanMapCode copy_source_diagnostics(const CadIrDocument *source,
    CadPlanReference *reference)
{
    for (size_t i = 0; i < source->diagnostic_count; i++) {
        const CadIrDiagnostic *diagnostic = &source->diagnostics[i];
        CadPlanMapCode status = append_diagnostic(reference, diagnostic->severity,
            diagnostic->code, diagnostic->detail,
            diagnostic->has_source ? &diagnostic->provenance : NULL);
        if (status != CAD_PLAN_MAP_SUCCESS) { return status; }
    }
    return CAD_PLAN_MAP_SUCCESS;
}

static int layer_selected(const CadPlanLayerFilter *filter, const char *layer)
{
    if (filter->mode == CAD_PLAN_LAYER_ALL) { return 1; }
    int match = 0;
    for (size_t i = 0; i < filter->layer_name_count; i++) {
        if (strcmp(layer, filter->layer_names[i]) == 0) { match = 1; break; }
    }
    return filter->mode == CAD_PLAN_LAYER_INCLUDE ? match : !match;
}

typedef enum {
    CONVERT_EXACT = 0,
    CONVERT_NON_WHOLE,
    CONVERT_OVERFLOW
} ConvertStatus;

static ConvertStatus multiply_into_plan_range(int64_t value, int64_t factor,
    int64_t *output)
{
    if (factor <= 0) { return CONVERT_OVERFLOW; }
    if (value > 0 && value > (int64_t)INT_MAX / factor) { return CONVERT_OVERFLOW; }
    if (value < 0 && value < (int64_t)INT_MIN / factor) { return CONVERT_OVERFLOW; }
    *output = value * factor;
    return CONVERT_EXACT;
}

static ConvertStatus decimal_to_mm(CadIrDecimal value, CadIrSourceUnit unit, int *output)
{
    int64_t multiplier = 1;
    int exponent_shift = 0;
    switch (unit) {
        case CAD_IR_UNIT_MILLIMETRE: break;
        case CAD_IR_UNIT_CENTIMETRE: exponent_shift = 1; break;
        case CAD_IR_UNIT_METRE: exponent_shift = 3; break;
        case CAD_IR_UNIT_INCH: multiplier = 254; exponent_shift = -1; break;
        case CAD_IR_UNIT_FOOT: multiplier = 3048; exponent_shift = -1; break;
        default: return CONVERT_OVERFLOW;
    }

    int64_t coefficient = value.coefficient;
    if (coefficient == 0) { *output = 0; return CONVERT_EXACT; }
    int64_t exponent = (int64_t)value.exponent10 + exponent_shift;

    if (exponent < 0) {
        uint64_t denominator_power = (uint64_t)(-exponent);
        uint64_t twos = denominator_power;
        uint64_t fives = denominator_power;

        while (twos > 0 && multiplier % 2 == 0) { multiplier /= 2; twos--; }
        while (fives > 0 && multiplier % 5 == 0) { multiplier /= 5; fives--; }
        while (twos > 0 && coefficient % 2 == 0) { coefficient /= 2; twos--; }
        while (fives > 0 && coefficient % 5 == 0) { coefficient /= 5; fives--; }
        if (twos != 0 || fives != 0) { return CONVERT_NON_WHOLE; }
    }

    int64_t result = 0;
    if (multiply_into_plan_range(coefficient, multiplier, &result) != CONVERT_EXACT) {
        return CONVERT_OVERFLOW;
    }
    if (exponent > 0) {
        for (int64_t i = 0; i < exponent; i++) {
            if (multiply_into_plan_range(result, 10, &result) != CONVERT_EXACT) {
                return CONVERT_OVERFLOW;
            }
        }
    }
    *output = (int)result;
    return CONVERT_EXACT;
}

static ConvertStatus translate_coordinate(int coordinate, int translation, int *output)
{
    int64_t sum = (int64_t)coordinate + translation;
    if (sum < INT_MIN || sum > INT_MAX) { return CONVERT_OVERFLOW; }
    *output = (int)sum;
    return CONVERT_EXACT;
}

static ConvertStatus map_point(CadIrPoint2 source, CadIrSourceUnit unit,
    PlanPosition translation, PlanPosition *output)
{
    int x = 0, y = 0;
    ConvertStatus status = decimal_to_mm(source.x, unit, &x);
    if (status != CONVERT_EXACT) { return status; }
    status = decimal_to_mm(source.y, unit, &y);
    if (status != CONVERT_EXACT) { return status; }
    if (translate_coordinate(x, translation.x, &x) != CONVERT_EXACT ||
        translate_coordinate(y, translation.y, &y) != CONVERT_EXACT) {
        return CONVERT_OVERFLOW;
    }
    *output = (PlanPosition){x, y};
    return CONVERT_EXACT;
}

static CadPlanMapCode append_mapped_path(CadPlanReference *reference,
    const CadIrPath *source_path, CadIrSourceUnit unit, PlanPosition translation)
{
    if (source_path->vertex_count > SIZE_MAX / sizeof(PlanPosition)) {
        return CAD_PLAN_MAP_NUMERIC_OVERFLOW;
    }
    PlanPosition *vertices = malloc(source_path->vertex_count * sizeof *vertices);
    if (vertices == NULL) { return CAD_PLAN_MAP_ALLOCATION_FAILED; }

    ConvertStatus convert = CONVERT_EXACT;
    for (size_t i = 0; i < source_path->vertex_count; i++) {
        convert = map_point(source_path->vertices[i], unit, translation, &vertices[i]);
        if (convert != CONVERT_EXACT) { break; }
    }
    if (convert != CONVERT_EXACT) {
        free(vertices);
        reference->mapping_skipped_path_count++;
        return append_diagnostic(reference, CAD_IR_DIAGNOSTIC_ERROR,
            convert == CONVERT_NON_WHOLE ? "NUMERIC_NON_WHOLE_MM" : "NUMERIC_OVERFLOW",
            NULL, &source_path->provenance);
    }

    CadPlanReferencePath mapped = {
        .vertices = vertices,
        .vertex_count = source_path->vertex_count,
        .closed = source_path->closed
    };
    CadPlanMapCode status = provenance_clone(&source_path->provenance, &mapped.provenance);
    if (status != CAD_PLAN_MAP_SUCCESS) { path_destroy(&mapped); return status; }
    if (reference->path_count == SIZE_MAX) { path_destroy(&mapped); return CAD_PLAN_MAP_NUMERIC_OVERFLOW; }
    status = ensure_path_capacity(reference, reference->path_count + 1);
    if (status != CAD_PLAN_MAP_SUCCESS) { path_destroy(&mapped); return status; }
    reference->paths[reference->path_count++] = mapped;
    return CAD_PLAN_MAP_SUCCESS;
}

static CadPlanMapCode prepare_metadata(const CadIrDocument *source,
    const CadPlanMappingConfig *config, CadPlanReference *reference)
{
    CadPlanMapCode status = duplicate_string(source->source_format, 0,
        &reference->source_format);
    if (status != CAD_PLAN_MAP_SUCCESS) { return status; }
    status = duplicate_string(source->source_version, 0, &reference->source_version);
    if (status != CAD_PLAN_MAP_SUCCESS) { return status; }
    reference->declared_unit = source->declared_unit;
    reference->translation_mm = config->translation_mm;
    reference->decoded_entity_count = source->decoded_entity_count;
    reference->adapter_skipped_entity_count = source->skipped_entity_count;
    reference->source_path_count = source->path_count;

    if (config->has_unit_override) {
        reference->resolved_unit = config->unit_override;
        reference->unit_resolution = CAD_PLAN_UNIT_FROM_OVERRIDE;
        if (physical_unit(source->declared_unit) && source->declared_unit != config->unit_override) {
            reference->unit_override_conflicted = 1;
            return append_diagnostic(reference, CAD_IR_DIAGNOSTIC_WARNING,
                "UNITS_OVERRIDE_CONFLICT", NULL, NULL);
        }
        return CAD_PLAN_MAP_SUCCESS;
    }
    if (physical_unit(source->declared_unit)) {
        reference->resolved_unit = source->declared_unit;
        reference->unit_resolution = CAD_PLAN_UNIT_FROM_SOURCE;
        return CAD_PLAN_MAP_SUCCESS;
    }

    reference->status = CAD_PLAN_REFERENCE_BLOCKED_UNITS;
    return append_diagnostic(reference, CAD_IR_DIAGNOSTIC_ERROR,
        "UNITS_REQUIRED", NULL, NULL);
}

CadPlanMapCode cad_plan_map_reference(const CadIrDocument *source,
    const CadPlanMappingConfig *config, CadPlanReference *output)
{
    if (source == NULL || config == NULL || output == NULL || !config_valid(config)) {
        return CAD_PLAN_MAP_INVALID_ARGUMENT;
    }
    if (!source_valid(source)) { return CAD_PLAN_MAP_INVALID_SOURCE; }

    CadPlanReference candidate = {0};
    CadPlanMapCode status = copy_source_diagnostics(source, &candidate);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
    status = prepare_metadata(source, config, &candidate);
    if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }

    if (candidate.status != CAD_PLAN_REFERENCE_BLOCKED_UNITS) {
        candidate.status = CAD_PLAN_REFERENCE_READY;
        for (size_t i = 0; i < source->path_count; i++) {
            const CadIrPath *path = &source->paths[i];
            if (!layer_selected(&config->layers, path->provenance.layer)) {
                candidate.filtered_path_count++;
                continue;
            }
            status = append_mapped_path(&candidate, path, candidate.resolved_unit,
                config->translation_mm);
            if (status != CAD_PLAN_MAP_SUCCESS) { goto fail; }
        }
    }

    cad_plan_reference_destroy(output);
    *output = candidate;
    return CAD_PLAN_MAP_SUCCESS;
fail:
    cad_plan_reference_destroy(&candidate);
    return status;
}

CadPlanReferenceStatistics cad_plan_reference_statistics(
    const CadPlanReference *reference)
{
    CadPlanReferenceStatistics statistics = {0};
    if (reference == NULL) { return statistics; }
    statistics.decoded_entity_count = reference->decoded_entity_count;
    statistics.adapter_skipped_entity_count = reference->adapter_skipped_entity_count;
    statistics.source_path_count = reference->source_path_count;
    statistics.emitted_path_count = reference->path_count;
    statistics.filtered_path_count = reference->filtered_path_count;
    statistics.mapping_skipped_path_count = reference->mapping_skipped_path_count;
    for (size_t i = 0; i < reference->diagnostic_count; i++) {
        switch (reference->diagnostics[i].severity) {
            case CAD_IR_DIAGNOSTIC_INFO: statistics.info_count++; break;
            case CAD_IR_DIAGNOSTIC_WARNING: statistics.warning_count++; break;
            case CAD_IR_DIAGNOSTIC_ERROR: statistics.error_count++; break;
            default: break;
        }
    }
    return statistics;
}
