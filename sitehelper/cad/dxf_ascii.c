#include "dxf_ascii.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *data;
    size_t length;
    size_t line_number;
} Span;

typedef struct {
    const char *data;
    size_t size;
    size_t offset;
    size_t next_line_number;
} LineReader;

typedef struct {
    int code;
    Span value;
    size_t code_line_number;
} DxfPair;

typedef enum {
    PAIR_OK = 0,
    PAIR_EOF,
    PAIR_MALFORMED
} PairStatus;

typedef enum {
    PARSE_NUMBER_OK = 0,
    PARSE_NUMBER_INVALID,
    PARSE_NUMBER_OVERFLOW
} ParseNumberStatus;

typedef enum {
    SECTION_NONE = 0,
    SECTION_HEADER,
    SECTION_ENTITIES,
    SECTION_OTHER
} SectionKind;

typedef struct {
    CadIrPoint2 point;
    int has_x;
    int has_y;
    int has_nonzero_width;
    int has_nonzero_bulge;
} DxfVertex;

typedef struct {
    Span kind;
    Span layer;
    Span handle;
    Span layout;
    size_t ordinal;
    int has_layer;
    int has_handle;
    int has_layout;
    int paper_space;
    int malformed;
    int numeric_invalid;
    int numeric_overflow;

    /* LINE fields. */
    CadIrDecimal line_x1;
    CadIrDecimal line_y1;
    CadIrDecimal line_z1;
    CadIrDecimal line_x2;
    CadIrDecimal line_y2;
    CadIrDecimal line_z2;
    CadIrDecimal line_thickness;
    CadIrDecimal extrusion_x;
    CadIrDecimal extrusion_y;
    CadIrDecimal extrusion_z;
    int has_line_x1;
    int has_line_y1;
    int has_line_z1;
    int has_line_x2;
    int has_line_y2;
    int has_line_z2;
    int has_line_thickness;
    int has_extrusion_x;
    int has_extrusion_y;
    int has_extrusion_z;

    /* LWPOLYLINE fields. */
    DxfVertex *vertices;
    size_t vertex_count;
    size_t vertex_capacity;
    size_t current_vertex;
    size_t declared_vertex_count;
    int has_current_vertex;
    int has_declared_vertex_count;
    int flags;
    int has_flags;
    CadIrDecimal elevation;
    CadIrDecimal lw_thickness;
    CadIrDecimal constant_width;
    int has_elevation;
    int has_lw_thickness;
    int has_constant_width;
    int has_nonzero_width;
    int has_nonzero_bulge;
} EntityBuilder;

typedef struct {
    Span version;
    size_t version_line_number;
    int has_version;
    int has_units;
    long units_code;
} HeaderState;

typedef struct {
    CadIrDocument document;
    HeaderState header;
    EntityBuilder entity;
    SectionKind section;
    int expecting_section_name;
    int saw_header;
    int saw_entities;
    int saw_eof;
    size_t decoded_count;
    size_t skipped_count;
} Decoder;

static DxfAsciiDecodeResult result(DxfAsciiDecodeCode code, size_t line_number)
{
    DxfAsciiDecodeResult value = {code, line_number};
    return value;
}

static Span span_trim(Span input)
{
    while (input.length > 0 &&
        (input.data[0] == ' ' || input.data[0] == '\t')) {
        input.data++;
        input.length--;
    }
    while (input.length > 0 &&
        (input.data[input.length - 1] == ' ' || input.data[input.length - 1] == '\t')) {
        input.length--;
    }
    return input;
}

static int span_equal(Span span, const char *text)
{
    size_t length = strlen(text);
    return span.length == length && memcmp(span.data, text, length) == 0;
}

static int span_empty(Span span)
{
    return span.length == 0;
}

static char *span_duplicate(Span span)
{
    if (span.length == SIZE_MAX) { return NULL; }
    char *copy = malloc(span.length + 1);
    if (copy == NULL) { return NULL; }
    memcpy(copy, span.data, span.length);
    copy[span.length] = '\0';
    return copy;
}

static int next_line(LineReader *reader, Span *line)
{
    if (reader->offset >= reader->size) { return 0; }
    size_t start = reader->offset;
    size_t end = start;
    while (end < reader->size && reader->data[end] != '\n') { end++; }
    size_t length = end - start;
    if (length > 0 && reader->data[start + length - 1] == '\r') { length--; }
    *line = (Span){reader->data + start, length, reader->next_line_number++};
    reader->offset = end < reader->size ? end + 1 : end;
    return 1;
}

static ParseNumberStatus parse_long_span(Span input, long *output)
{
    input = span_trim(input);
    if (span_empty(input)) { return PARSE_NUMBER_INVALID; }
    size_t i = 0;
    int negative = 0;
    if (input.data[i] == '+' || input.data[i] == '-') {
        negative = input.data[i] == '-';
        i++;
    }
    if (i == input.length) { return PARSE_NUMBER_INVALID; }
    unsigned long magnitude = 0;
    unsigned long limit = negative ? (unsigned long)LONG_MAX + 1UL : (unsigned long)LONG_MAX;
    for (; i < input.length; i++) {
        unsigned char ch = (unsigned char)input.data[i];
        if (ch < '0' || ch > '9') { return PARSE_NUMBER_INVALID; }
        unsigned long digit = (unsigned long)(ch - '0');
        if (magnitude > (limit - digit) / 10UL) { return PARSE_NUMBER_OVERFLOW; }
        magnitude = magnitude * 10UL + digit;
    }
    if (negative) {
        if (magnitude == (unsigned long)LONG_MAX + 1UL) {
            *output = LONG_MIN;
        } else {
            *output = -(long)magnitude;
        }
    } else {
        *output = (long)magnitude;
    }
    return PARSE_NUMBER_OK;
}

static ParseNumberStatus parse_size_span(Span input, size_t *output)
{
    input = span_trim(input);
    if (span_empty(input)) { return PARSE_NUMBER_INVALID; }
    size_t value = 0;
    for (size_t i = 0; i < input.length; i++) {
        unsigned char ch = (unsigned char)input.data[i];
        if (ch < '0' || ch > '9') { return PARSE_NUMBER_INVALID; }
        size_t digit = (size_t)(ch - '0');
        if (value > (SIZE_MAX - digit) / 10) { return PARSE_NUMBER_OVERFLOW; }
        value = value * 10 + digit;
    }
    *output = value;
    return PARSE_NUMBER_OK;
}

static ParseNumberStatus parse_exponent(Span input, size_t start, int64_t *output)
{
    if (start >= input.length) {
        *output = 0;
        return PARSE_NUMBER_OK;
    }
    size_t i = start;
    int negative = 0;
    if (input.data[i] == '+' || input.data[i] == '-') {
        negative = input.data[i] == '-';
        i++;
    }
    if (i == input.length) { return PARSE_NUMBER_INVALID; }
    uint64_t magnitude = 0;
    const uint64_t positive_limit = (uint64_t)INT64_MAX;
    const uint64_t negative_limit = positive_limit + UINT64_C(1);
    uint64_t limit = negative ? negative_limit : positive_limit;
    for (; i < input.length; i++) {
        unsigned char ch = (unsigned char)input.data[i];
        if (ch < '0' || ch > '9') { return PARSE_NUMBER_INVALID; }
        uint64_t digit = (uint64_t)(ch - '0');
        if (magnitude > (limit - digit) / UINT64_C(10)) {
            return PARSE_NUMBER_OVERFLOW;
        }
        magnitude = magnitude * UINT64_C(10) + digit;
    }
    if (negative) {
        if (magnitude == negative_limit) {
            *output = INT64_MIN;
        } else {
            *output = -(int64_t)magnitude;
        }
    } else {
        *output = (int64_t)magnitude;
    }
    return PARSE_NUMBER_OK;
}

static ParseNumberStatus parse_decimal_span(Span input, CadIrDecimal *output)
{
    input = span_trim(input);
    if (span_empty(input)) { return PARSE_NUMBER_INVALID; }

    size_t i = 0;
    int negative = 0;
    if (input.data[i] == '+' || input.data[i] == '-') {
        negative = input.data[i] == '-';
        i++;
    }
    if (i == input.length) { return PARSE_NUMBER_INVALID; }

    size_t mantissa_start = i;
    size_t exponent_start = input.length;
    size_t decimal_index = SIZE_MAX;
    size_t digit_count = 0;
    size_t fractional_digit_count = 0;
    size_t first_nonzero_digit = SIZE_MAX;
    size_t last_nonzero_digit = SIZE_MAX;

    for (; i < input.length; i++) {
        unsigned char ch = (unsigned char)input.data[i];
        if (ch >= '0' && ch <= '9') {
            if (first_nonzero_digit == SIZE_MAX && ch != '0') {
                first_nonzero_digit = digit_count;
            }
            if (ch != '0') { last_nonzero_digit = digit_count; }
            digit_count++;
            if (decimal_index != SIZE_MAX) { fractional_digit_count++; }
            continue;
        }
        if (ch == '.' && decimal_index == SIZE_MAX) {
            decimal_index = i;
            continue;
        }
        if (ch == 'e' || ch == 'E') {
            exponent_start = i + 1;
            break;
        }
        return PARSE_NUMBER_INVALID;
    }
    if (digit_count == 0) { return PARSE_NUMBER_INVALID; }

    int64_t explicit_exponent = 0;
    if (exponent_start != input.length) {
        ParseNumberStatus status = parse_exponent(input, exponent_start, &explicit_exponent);
        if (status != PARSE_NUMBER_OK) { return status; }
    }

    if (first_nonzero_digit == SIZE_MAX) {
        *output = (CadIrDecimal){0, 0};
        return PARSE_NUMBER_OK;
    }

    size_t trailing_zeros = digit_count - last_nonzero_digit - 1;
    if (fractional_digit_count > (size_t)INT64_MAX || trailing_zeros > (size_t)INT64_MAX) {
        return PARSE_NUMBER_OVERFLOW;
    }
    int64_t exponent10 = explicit_exponent;
    int64_t fractional = (int64_t)fractional_digit_count;
    int64_t trailing = (int64_t)trailing_zeros;
    if (exponent10 < INT64_MIN + fractional) { return PARSE_NUMBER_OVERFLOW; }
    exponent10 -= fractional;
    if (exponent10 > INT64_MAX - trailing) { return PARSE_NUMBER_OVERFLOW; }
    exponent10 += trailing;
    if (exponent10 < INT32_MIN || exponent10 > INT32_MAX) {
        return PARSE_NUMBER_OVERFLOW;
    }

    const uint64_t positive_limit = (uint64_t)INT64_MAX;
    const uint64_t negative_limit = positive_limit + UINT64_C(1);
    uint64_t limit = negative ? negative_limit : positive_limit;
    uint64_t magnitude = 0;
    size_t logical_digit = 0;
    for (i = mantissa_start; i < input.length; i++) {
        unsigned char ch = (unsigned char)input.data[i];
        if (ch == 'e' || ch == 'E') { break; }
        if (ch == '.') { continue; }
        if (logical_digit >= first_nonzero_digit && logical_digit <= last_nonzero_digit) {
            uint64_t digit = (uint64_t)(ch - '0');
            if (magnitude > (limit - digit) / UINT64_C(10)) {
                return PARSE_NUMBER_OVERFLOW;
            }
            magnitude = magnitude * UINT64_C(10) + digit;
        }
        logical_digit++;
    }

    int64_t coefficient;
    if (negative) {
        if (magnitude == negative_limit) {
            coefficient = INT64_MIN;
        } else {
            coefficient = -(int64_t)magnitude;
        }
    } else {
        coefficient = (int64_t)magnitude;
    }
    *output = (CadIrDecimal){coefficient, (int32_t)exponent10};
    return PARSE_NUMBER_OK;
}

static int decimal_is_zero(CadIrDecimal value)
{
    return value.coefficient == 0;
}

static int decimal_is_one(CadIrDecimal value)
{
    return value.coefficient == 1 && value.exponent10 == 0;
}

static int decimal_equal(CadIrDecimal a, CadIrDecimal b)
{
    return a.coefficient == b.coefficient && a.exponent10 == b.exponent10;
}

static int point_equal(CadIrPoint2 a, CadIrPoint2 b)
{
    return decimal_equal(a.x, b.x) && decimal_equal(a.y, b.y);
}

static PairStatus next_pair(LineReader *reader, DxfPair *pair, size_t *error_line)
{
    Span code_line;
    Span value_line;
    if (!next_line(reader, &code_line)) { return PAIR_EOF; }
    if (!next_line(reader, &value_line)) {
        *error_line = code_line.line_number;
        return PAIR_MALFORMED;
    }
    long code = 0;
    ParseNumberStatus status = parse_long_span(code_line, &code);
    if (status != PARSE_NUMBER_OK || code < 0 || code > 1071) {
        *error_line = code_line.line_number;
        return PAIR_MALFORMED;
    }
    pair->code = (int)code;
    pair->value = span_trim(value_line);
    pair->code_line_number = code_line.line_number;
    return PAIR_OK;
}

static void entity_builder_destroy(EntityBuilder *entity)
{
    if (entity == NULL) { return; }
    free(entity->vertices);
    *entity = (EntityBuilder){0};
}

static int entity_is(EntityBuilder *entity, const char *kind)
{
    return span_equal(entity->kind, kind);
}

static DxfAsciiDecodeCode ensure_vertex_capacity(EntityBuilder *entity, size_t required)
{
    if (required <= entity->vertex_capacity) { return DXF_ASCII_DECODE_SUCCESS; }
    if (required > SIZE_MAX / sizeof *entity->vertices) {
        return DXF_ASCII_DECODE_NUMERIC_OVERFLOW;
    }
    size_t capacity = entity->vertex_capacity == 0 ? 4 : entity->vertex_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }
    if (capacity > SIZE_MAX / sizeof *entity->vertices) {
        return DXF_ASCII_DECODE_NUMERIC_OVERFLOW;
    }
    DxfVertex *storage = realloc(entity->vertices, capacity * sizeof *storage);
    if (storage == NULL) { return DXF_ASCII_DECODE_ALLOCATION_FAILED; }
    entity->vertices = storage;
    entity->vertex_capacity = capacity;
    return DXF_ASCII_DECODE_SUCCESS;
}

static void mark_numeric(EntityBuilder *entity, ParseNumberStatus status)
{
    if (status == PARSE_NUMBER_INVALID) { entity->numeric_invalid = 1; }
    if (status == PARSE_NUMBER_OVERFLOW) { entity->numeric_overflow = 1; }
}

static void assign_decimal_once(EntityBuilder *entity, Span value,
    CadIrDecimal *target, int *present)
{
    if (*present) {
        entity->malformed = 1;
        return;
    }
    ParseNumberStatus status = parse_decimal_span(value, target);
    if (status != PARSE_NUMBER_OK) {
        mark_numeric(entity, status);
        return;
    }
    *present = 1;
}

static void parse_generic_entity_pair(EntityBuilder *entity, const DxfPair *pair)
{
    if (pair->code == 8) {
        if (entity->has_layer || span_empty(pair->value)) {
            entity->malformed = 1;
        } else {
            entity->layer = pair->value;
            entity->has_layer = 1;
        }
    } else if (pair->code == 5) {
        if (entity->has_handle || span_empty(pair->value)) {
            entity->malformed = 1;
        } else {
            entity->handle = pair->value;
            entity->has_handle = 1;
        }
    } else if (pair->code == 67) {
        long value = 0;
        ParseNumberStatus status = parse_long_span(pair->value, &value);
        if (status != PARSE_NUMBER_OK) {
            mark_numeric(entity, status);
        } else if (value != 0 && value != 1) {
            entity->malformed = 1;
        } else if (value == 1) {
            entity->paper_space = 1;
        }
    } else if (pair->code == 410) {
        if (entity->has_layout || span_empty(pair->value)) {
            entity->malformed = 1;
        } else {
            entity->layout = pair->value;
            entity->has_layout = 1;
            if (!span_equal(pair->value, "Model")) { entity->paper_space = 1; }
        }
    }
}

static void parse_line_pair(EntityBuilder *entity, const DxfPair *pair)
{
    switch (pair->code) {
        case 10: assign_decimal_once(entity, pair->value, &entity->line_x1, &entity->has_line_x1); break;
        case 20: assign_decimal_once(entity, pair->value, &entity->line_y1, &entity->has_line_y1); break;
        case 30: assign_decimal_once(entity, pair->value, &entity->line_z1, &entity->has_line_z1); break;
        case 11: assign_decimal_once(entity, pair->value, &entity->line_x2, &entity->has_line_x2); break;
        case 21: assign_decimal_once(entity, pair->value, &entity->line_y2, &entity->has_line_y2); break;
        case 31: assign_decimal_once(entity, pair->value, &entity->line_z2, &entity->has_line_z2); break;
        case 39: assign_decimal_once(entity, pair->value, &entity->line_thickness, &entity->has_line_thickness); break;
        case 210: assign_decimal_once(entity, pair->value, &entity->extrusion_x, &entity->has_extrusion_x); break;
        case 220: assign_decimal_once(entity, pair->value, &entity->extrusion_y, &entity->has_extrusion_y); break;
        case 230: assign_decimal_once(entity, pair->value, &entity->extrusion_z, &entity->has_extrusion_z); break;
        default: break;
    }
}

static void parse_lwpolyline_integer(EntityBuilder *entity, const DxfPair *pair)
{
    if (pair->code == 90) {
        if (entity->has_declared_vertex_count) {
            entity->malformed = 1;
            return;
        }
        size_t value = 0;
        ParseNumberStatus status = parse_size_span(pair->value, &value);
        if (status != PARSE_NUMBER_OK) {
            mark_numeric(entity, status);
            return;
        }
        entity->declared_vertex_count = value;
        entity->has_declared_vertex_count = 1;
    } else if (pair->code == 70) {
        if (entity->has_flags) {
            entity->malformed = 1;
            return;
        }
        long value = 0;
        ParseNumberStatus status = parse_long_span(pair->value, &value);
        if (status != PARSE_NUMBER_OK) {
            mark_numeric(entity, status);
            return;
        }
        if (value < 0 || value > INT_MAX) {
            entity->malformed = 1;
            return;
        }
        entity->flags = (int)value;
        entity->has_flags = 1;
    }
}

static DxfAsciiDecodeCode parse_lwpolyline_vertex(EntityBuilder *entity,
    const DxfPair *pair)
{
    if (pair->code == 10) {
        if (entity->has_current_vertex &&
            (!entity->vertices[entity->current_vertex].has_x ||
             !entity->vertices[entity->current_vertex].has_y)) {
            entity->malformed = 1;
        }
        if (entity->vertex_count == SIZE_MAX) {
            return DXF_ASCII_DECODE_NUMERIC_OVERFLOW;
        }
        DxfAsciiDecodeCode capacity = ensure_vertex_capacity(entity, entity->vertex_count + 1);
        if (capacity != DXF_ASCII_DECODE_SUCCESS) { return capacity; }
        DxfVertex vertex = {0};
        ParseNumberStatus status = parse_decimal_span(pair->value, &vertex.point.x);
        if (status != PARSE_NUMBER_OK) {
            mark_numeric(entity, status);
        } else {
            vertex.has_x = 1;
        }
        entity->vertices[entity->vertex_count] = vertex;
        entity->current_vertex = entity->vertex_count;
        entity->vertex_count++;
        entity->has_current_vertex = 1;
        return DXF_ASCII_DECODE_SUCCESS;
    }
    if (pair->code == 20) {
        if (!entity->has_current_vertex || entity->vertices[entity->current_vertex].has_y) {
            entity->malformed = 1;
            return DXF_ASCII_DECODE_SUCCESS;
        }
        ParseNumberStatus status = parse_decimal_span(pair->value,
            &entity->vertices[entity->current_vertex].point.y);
        if (status != PARSE_NUMBER_OK) {
            mark_numeric(entity, status);
        } else {
            entity->vertices[entity->current_vertex].has_y = 1;
        }
        return DXF_ASCII_DECODE_SUCCESS;
    }
    if (pair->code == 40 || pair->code == 41 || pair->code == 42) {
        if (!entity->has_current_vertex) {
            entity->malformed = 1;
            return DXF_ASCII_DECODE_SUCCESS;
        }
        CadIrDecimal value = {0};
        ParseNumberStatus status = parse_decimal_span(pair->value, &value);
        if (status != PARSE_NUMBER_OK) {
            mark_numeric(entity, status);
            return DXF_ASCII_DECODE_SUCCESS;
        }
        if (pair->code == 42 && !decimal_is_zero(value)) {
            entity->vertices[entity->current_vertex].has_nonzero_bulge = 1;
            entity->has_nonzero_bulge = 1;
        } else if ((pair->code == 40 || pair->code == 41) && !decimal_is_zero(value)) {
            entity->vertices[entity->current_vertex].has_nonzero_width = 1;
            entity->has_nonzero_width = 1;
        }
    }
    return DXF_ASCII_DECODE_SUCCESS;
}

static DxfAsciiDecodeCode parse_lwpolyline_pair(EntityBuilder *entity,
    const DxfPair *pair)
{
    if (pair->code == 90 || pair->code == 70) {
        parse_lwpolyline_integer(entity, pair);
        return DXF_ASCII_DECODE_SUCCESS;
    }
    if (pair->code == 10 || pair->code == 20 || pair->code == 40 ||
        pair->code == 41 || pair->code == 42) {
        return parse_lwpolyline_vertex(entity, pair);
    }
    switch (pair->code) {
        case 38: assign_decimal_once(entity, pair->value, &entity->elevation, &entity->has_elevation); break;
        case 39: assign_decimal_once(entity, pair->value, &entity->lw_thickness, &entity->has_lw_thickness); break;
        case 43:
            assign_decimal_once(entity, pair->value, &entity->constant_width, &entity->has_constant_width);
            if (entity->has_constant_width && !decimal_is_zero(entity->constant_width)) {
                entity->has_nonzero_width = 1;
            }
            break;
        case 210: assign_decimal_once(entity, pair->value, &entity->extrusion_x, &entity->has_extrusion_x); break;
        case 220: assign_decimal_once(entity, pair->value, &entity->extrusion_y, &entity->has_extrusion_y); break;
        case 230: assign_decimal_once(entity, pair->value, &entity->extrusion_z, &entity->has_extrusion_z); break;
        default: break;
    }
    return DXF_ASCII_DECODE_SUCCESS;
}

static DxfAsciiDecodeCode parse_entity_pair(EntityBuilder *entity, const DxfPair *pair)
{
    parse_generic_entity_pair(entity, pair);
    if (entity_is(entity, "LINE")) {
        parse_line_pair(entity, pair);
        return DXF_ASCII_DECODE_SUCCESS;
    }
    if (entity_is(entity, "LWPOLYLINE")) {
        return parse_lwpolyline_pair(entity, pair);
    }
    return DXF_ASCII_DECODE_SUCCESS;
}

static DxfAsciiDecodeCode provenance_strings(const EntityBuilder *entity,
    char **kind, char **layer, char **handle)
{
    *kind = NULL;
    *layer = NULL;
    *handle = NULL;
    *kind = span_duplicate(entity->kind);
    if (*kind == NULL) { goto allocation_failed; }
    if (entity->has_layer) {
        *layer = span_duplicate(entity->layer);
    } else {
        static const char default_layer[] = "0";
        Span span = {default_layer, 1, 0};
        *layer = span_duplicate(span);
    }
    if (*layer == NULL) { goto allocation_failed; }
    if (entity->has_handle) {
        *handle = span_duplicate(entity->handle);
        if (*handle == NULL) { goto allocation_failed; }
    }
    return DXF_ASCII_DECODE_SUCCESS;
allocation_failed:
    free(*kind);
    free(*layer);
    free(*handle);
    *kind = NULL;
    *layer = NULL;
    *handle = NULL;
    return DXF_ASCII_DECODE_ALLOCATION_FAILED;
}

static DxfAsciiDecodeCode map_ir_code(CadIrCode code)
{
    switch (code) {
        case CAD_IR_SUCCESS: return DXF_ASCII_DECODE_SUCCESS;
        case CAD_IR_ALLOCATION_FAILED: return DXF_ASCII_DECODE_ALLOCATION_FAILED;
        case CAD_IR_NUMERIC_OVERFLOW: return DXF_ASCII_DECODE_NUMERIC_OVERFLOW;
        default: return DXF_ASCII_DECODE_IR_FAILURE;
    }
}

static DxfAsciiDecodeCode append_entity_diagnostic(CadIrDocument *document,
    const EntityBuilder *entity, CadIrDiagnosticSeverity severity,
    const char *code, const char *detail,
    const char *kind, const char *layer, const char *handle)
{
    CadIrDiagnosticInput input = {
        .severity = severity,
        .code = code,
        .detail = detail,
        .has_source = 1,
        .provenance = {entity->ordinal, kind, layer, handle}
    };
    return map_ir_code(cad_ir_document_append_diagnostic(document, &input));
}

static int extrusion_is_default(const EntityBuilder *entity)
{
    CadIrDecimal x = entity->has_extrusion_x ? entity->extrusion_x : (CadIrDecimal){0, 0};
    CadIrDecimal y = entity->has_extrusion_y ? entity->extrusion_y : (CadIrDecimal){0, 0};
    CadIrDecimal z = entity->has_extrusion_z ? entity->extrusion_z : (CadIrDecimal){1, 0};
    return decimal_is_zero(x) && decimal_is_zero(y) && decimal_is_one(z);
}

static int line_is_2d(const EntityBuilder *entity)
{
    return (!entity->has_line_z1 || decimal_is_zero(entity->line_z1)) &&
        (!entity->has_line_z2 || decimal_is_zero(entity->line_z2)) &&
        (!entity->has_line_thickness || decimal_is_zero(entity->line_thickness)) &&
        extrusion_is_default(entity);
}

static int has_at_least_distinct_vertices(const EntityBuilder *entity, size_t required)
{
    if (required == 0) { return 1; }
    if (entity->vertex_count == 0) { return 0; }

    CadIrPoint2 first = entity->vertices[0].point;
    if (required == 1) { return 1; }
    size_t second_index = SIZE_MAX;
    for (size_t i = 1; i < entity->vertex_count; i++) {
        if (!point_equal(entity->vertices[i].point, first)) {
            second_index = i;
            break;
        }
    }
    if (second_index == SIZE_MAX) { return 0; }
    if (required == 2) { return 1; }

    CadIrPoint2 second = entity->vertices[second_index].point;
    for (size_t i = second_index + 1; i < entity->vertex_count; i++) {
        if (!point_equal(entity->vertices[i].point, first) &&
            !point_equal(entity->vertices[i].point, second)) {
            return 1;
        }
    }
    return 0;
}

static int lwpolyline_is_2d(const EntityBuilder *entity)
{
    return (!entity->has_elevation || decimal_is_zero(entity->elevation)) &&
        (!entity->has_lw_thickness || decimal_is_zero(entity->lw_thickness)) &&
        extrusion_is_default(entity);
}

static DxfAsciiDecodeCode finalize_line(Decoder *decoder, EntityBuilder *entity,
    const char *kind, const char *layer, const char *handle)
{
    const char *diagnostic_code = NULL;
    const char *detail = NULL;
    CadIrDiagnosticSeverity severity = CAD_IR_DIAGNOSTIC_WARNING;

    if (entity->numeric_overflow) {
        diagnostic_code = "NUMERIC_OVERFLOW";
        severity = CAD_IR_DIAGNOSTIC_ERROR;
    } else if (entity->numeric_invalid) {
        diagnostic_code = "NUMERIC_INVALID";
        severity = CAD_IR_DIAGNOSTIC_ERROR;
    } else if (entity->malformed || !entity->has_line_x1 || !entity->has_line_y1 ||
        !entity->has_line_x2 || !entity->has_line_y2) {
        diagnostic_code = "FORMAT_MALFORMED";
        detail = "LINE fields";
        severity = CAD_IR_DIAGNOSTIC_ERROR;
    } else if (entity->paper_space) {
        diagnostic_code = "ENTITY_PAPER_SPACE";
    } else if (!line_is_2d(entity)) {
        diagnostic_code = "ENTITY_UNSUPPORTED_3D_OR_OCS";
    } else {
        CadIrPoint2 vertices[2] = {
            {entity->line_x1, entity->line_y1},
            {entity->line_x2, entity->line_y2}
        };
        if (point_equal(vertices[0], vertices[1])) {
            diagnostic_code = "ENTITY_DEGENERATE";
        } else {
            CadIrPathInput path = {
                .vertices = vertices,
                .vertex_count = 2,
                .closed = 0,
                .provenance = {entity->ordinal, kind, layer, handle}
            };
            DxfAsciiDecodeCode status = map_ir_code(
                cad_ir_document_append_path(&decoder->document, &path));
            if (status != DXF_ASCII_DECODE_SUCCESS) { return status; }
            return DXF_ASCII_DECODE_SUCCESS;
        }
    }

    decoder->skipped_count++;
    return append_entity_diagnostic(&decoder->document, entity, severity,
        diagnostic_code, detail, kind, layer, handle);
}

static DxfAsciiDecodeCode finalize_lwpolyline(Decoder *decoder,
    EntityBuilder *entity, const char *kind, const char *layer, const char *handle)
{
    const char *diagnostic_code = NULL;
    const char *detail = NULL;
    CadIrDiagnosticSeverity severity = CAD_IR_DIAGNOSTIC_WARNING;

    if (entity->has_current_vertex &&
        (!entity->vertices[entity->current_vertex].has_x ||
         !entity->vertices[entity->current_vertex].has_y)) {
        entity->malformed = 1;
    }
    for (size_t i = 0; i < entity->vertex_count; i++) {
        if (!entity->vertices[i].has_x || !entity->vertices[i].has_y) {
            entity->malformed = 1;
        }
    }

    int closed = (entity->flags & 1) != 0;
    if (entity->numeric_overflow) {
        diagnostic_code = "NUMERIC_OVERFLOW";
        severity = CAD_IR_DIAGNOSTIC_ERROR;
    } else if (entity->numeric_invalid) {
        diagnostic_code = "NUMERIC_INVALID";
        severity = CAD_IR_DIAGNOSTIC_ERROR;
    } else if (entity->malformed || !entity->has_declared_vertex_count ||
        entity->declared_vertex_count != entity->vertex_count) {
        diagnostic_code = "FORMAT_MALFORMED";
        detail = "LWPOLYLINE fields";
        severity = CAD_IR_DIAGNOSTIC_ERROR;
    } else if (entity->paper_space) {
        diagnostic_code = "ENTITY_PAPER_SPACE";
    } else if (!lwpolyline_is_2d(entity)) {
        diagnostic_code = "ENTITY_UNSUPPORTED_3D_OR_OCS";
    } else if (entity->has_nonzero_bulge) {
        diagnostic_code = "ENTITY_UNSUPPORTED_CURVE";
    } else {
        size_t required_distinct = closed ? 3u : 2u;
        if (entity->vertex_count < 2 ||
            !has_at_least_distinct_vertices(entity, required_distinct)) {
            diagnostic_code = "ENTITY_DEGENERATE";
        } else {
            if (entity->vertex_count > SIZE_MAX / sizeof(CadIrPoint2)) {
                return DXF_ASCII_DECODE_NUMERIC_OVERFLOW;
            }
            CadIrPoint2 *points = malloc(entity->vertex_count * sizeof *points);
            if (points == NULL) { return DXF_ASCII_DECODE_ALLOCATION_FAILED; }
            for (size_t i = 0; i < entity->vertex_count; i++) {
                points[i] = entity->vertices[i].point;
            }
            CadIrPathInput path = {
                .vertices = points,
                .vertex_count = entity->vertex_count,
                .closed = closed,
                .provenance = {entity->ordinal, kind, layer, handle}
            };
            DxfAsciiDecodeCode status = map_ir_code(
                cad_ir_document_append_path(&decoder->document, &path));
            free(points);
            if (status != DXF_ASCII_DECODE_SUCCESS) { return status; }
            if (entity->has_nonzero_width) {
                status = append_entity_diagnostic(&decoder->document, entity,
                    CAD_IR_DIAGNOSTIC_WARNING, "STYLE_NOT_PRESERVED",
                    "LWPOLYLINE width", kind, layer, handle);
                if (status != DXF_ASCII_DECODE_SUCCESS) { return status; }
            }
            return DXF_ASCII_DECODE_SUCCESS;
        }
    }

    decoder->skipped_count++;
    return append_entity_diagnostic(&decoder->document, entity, severity,
        diagnostic_code, detail, kind, layer, handle);
}

static DxfAsciiDecodeCode finalize_entity(Decoder *decoder)
{
    EntityBuilder *entity = &decoder->entity;
    if (span_empty(entity->kind)) { return DXF_ASCII_DECODE_SUCCESS; }

    char *kind = NULL;
    char *layer = NULL;
    char *handle = NULL;
    DxfAsciiDecodeCode status = provenance_strings(entity, &kind, &layer, &handle);
    if (status != DXF_ASCII_DECODE_SUCCESS) { return status; }

    if (entity_is(entity, "LINE")) {
        status = finalize_line(decoder, entity, kind, layer, handle);
    } else if (entity_is(entity, "LWPOLYLINE")) {
        status = finalize_lwpolyline(decoder, entity, kind, layer, handle);
    } else {
        decoder->skipped_count++;
        status = append_entity_diagnostic(&decoder->document, entity,
            CAD_IR_DIAGNOSTIC_WARNING, "ENTITY_UNSUPPORTED_KIND", NULL,
            kind, layer, handle);
    }

    free(kind);
    free(layer);
    free(handle);
    entity_builder_destroy(entity);
    return status;
}

static DxfAsciiDecodeCode start_entity(Decoder *decoder, Span kind)
{
    DxfAsciiDecodeCode status = finalize_entity(decoder);
    if (status != DXF_ASCII_DECODE_SUCCESS) { return status; }
    if (span_empty(kind)) { return DXF_ASCII_DECODE_MALFORMED; }
    if (decoder->decoded_count == SIZE_MAX) { return DXF_ASCII_DECODE_NUMERIC_OVERFLOW; }
    decoder->decoded_count++;
    decoder->entity.kind = kind;
    decoder->entity.ordinal = decoder->decoded_count;
    return DXF_ASCII_DECODE_SUCCESS;
}

static CadIrSourceUnit unit_from_insunits(long value)
{
    switch (value) {
        case 0: return CAD_IR_UNIT_UNSPECIFIED;
        case 1: return CAD_IR_UNIT_INCH;
        case 2: return CAD_IR_UNIT_FOOT;
        case 4: return CAD_IR_UNIT_MILLIMETRE;
        case 5: return CAD_IR_UNIT_CENTIMETRE;
        case 6: return CAD_IR_UNIT_METRE;
        default: return CAD_IR_UNIT_OTHER;
    }
}

static int supported_version(Span version)
{
    static const char *const supported[] = {
        "AC1015", "AC1018", "AC1021", "AC1024", "AC1027", "AC1032"
    };
    for (size_t i = 0; i < sizeof supported / sizeof supported[0]; i++) {
        if (span_equal(version, supported[i])) { return 1; }
    }
    return 0;
}

static DxfAsciiDecodeCode parse_header_pair(HeaderState *header,
    Span *current_variable, const DxfPair *pair)
{
    if (pair->code == 9) {
        *current_variable = pair->value;
        return span_empty(pair->value) ? DXF_ASCII_DECODE_MALFORMED : DXF_ASCII_DECODE_SUCCESS;
    }
    if (span_equal(*current_variable, "$ACADVER")) {
        if (pair->code != 1 || header->has_version || span_empty(pair->value)) {
            return DXF_ASCII_DECODE_MALFORMED;
        }
        header->version = pair->value;
        header->version_line_number = pair->value.line_number;
        header->has_version = 1;
        *current_variable = (Span){0};
    } else if (span_equal(*current_variable, "$INSUNITS")) {
        if (pair->code != 70 || header->has_units) {
            return DXF_ASCII_DECODE_MALFORMED;
        }
        long value = 0;
        ParseNumberStatus number = parse_long_span(pair->value, &value);
        if (number != PARSE_NUMBER_OK || value < 0) {
            return number == PARSE_NUMBER_OVERFLOW ?
                DXF_ASCII_DECODE_NUMERIC_OVERFLOW : DXF_ASCII_DECODE_MALFORMED;
        }
        header->units_code = value;
        header->has_units = 1;
        *current_variable = (Span){0};
    }
    return DXF_ASCII_DECODE_SUCCESS;
}

static int is_binary_dxf(const unsigned char *bytes, size_t byte_count)
{
    static const unsigned char sentinel[] = {
        'A','u','t','o','C','A','D',' ','B','i','n','a','r','y',' ','D','X','F',
        '\r','\n',0x1A,0x00
    };
    return byte_count >= sizeof sentinel && memcmp(bytes, sentinel, sizeof sentinel) == 0;
}

static DxfAsciiDecodeResult decode_pairs(const char *bytes, size_t byte_count,
    CadIrDocument *output)
{
    Decoder decoder = {0};
    cad_ir_document_init(&decoder.document);
    LineReader reader = {bytes, byte_count, 0, 1};
    Span current_header_variable = {0};
    size_t error_line = 0;

    for (;;) {
        DxfPair pair;
        PairStatus pair_status = next_pair(&reader, &pair, &error_line);
        if (pair_status == PAIR_EOF) { break; }
        if (pair_status == PAIR_MALFORMED) {
            cad_ir_document_destroy(&decoder.document);
            entity_builder_destroy(&decoder.entity);
            return result(DXF_ASCII_DECODE_MALFORMED, error_line);
        }
        if (decoder.saw_eof) {
            cad_ir_document_destroy(&decoder.document);
            entity_builder_destroy(&decoder.entity);
            return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
        }
        /* DXF group 999 is an ASCII comment and carries no structure. Accept it
         * at any pre-EOF location, including between SECTION and its name. */
        if (pair.code == 999) { continue; }

        if (decoder.expecting_section_name) {
            if (pair.code != 2 || span_empty(pair.value) || decoder.section != SECTION_NONE) {
                cad_ir_document_destroy(&decoder.document);
                entity_builder_destroy(&decoder.entity);
                return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
            }
            if (span_equal(pair.value, "HEADER")) {
                if (decoder.saw_header) {
                    cad_ir_document_destroy(&decoder.document);
                    entity_builder_destroy(&decoder.entity);
                    return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
                }
                decoder.section = SECTION_HEADER;
                decoder.saw_header = 1;
            } else if (span_equal(pair.value, "ENTITIES")) {
                if (decoder.saw_entities) {
                    cad_ir_document_destroy(&decoder.document);
                    entity_builder_destroy(&decoder.entity);
                    return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
                }
                decoder.section = SECTION_ENTITIES;
                decoder.saw_entities = 1;
            } else {
                decoder.section = SECTION_OTHER;
            }
            decoder.expecting_section_name = 0;
            continue;
        }

        if (pair.code == 0 && span_equal(pair.value, "SECTION")) {
            if (decoder.section != SECTION_NONE) {
                cad_ir_document_destroy(&decoder.document);
                entity_builder_destroy(&decoder.entity);
                return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
            }
            decoder.expecting_section_name = 1;
            continue;
        }

        if (pair.code == 0 && span_equal(pair.value, "ENDSEC")) {
            if (decoder.section == SECTION_NONE) {
                cad_ir_document_destroy(&decoder.document);
                entity_builder_destroy(&decoder.entity);
                return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
            }
            if (decoder.section == SECTION_ENTITIES) {
                DxfAsciiDecodeCode status = finalize_entity(&decoder);
                if (status != DXF_ASCII_DECODE_SUCCESS) {
                    cad_ir_document_destroy(&decoder.document);
                    entity_builder_destroy(&decoder.entity);
                    return result(status, pair.code_line_number);
                }
            }
            decoder.section = SECTION_NONE;
            current_header_variable = (Span){0};
            continue;
        }

        if (pair.code == 0 && span_equal(pair.value, "EOF")) {
            if (decoder.section != SECTION_NONE || decoder.expecting_section_name) {
                cad_ir_document_destroy(&decoder.document);
                entity_builder_destroy(&decoder.entity);
                return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
            }
            decoder.saw_eof = 1;
            continue;
        }

        if (decoder.section == SECTION_NONE) {
            cad_ir_document_destroy(&decoder.document);
            entity_builder_destroy(&decoder.entity);
            return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
        }

        if (decoder.section == SECTION_HEADER) {
            DxfAsciiDecodeCode status = parse_header_pair(&decoder.header,
                &current_header_variable, &pair);
            if (status != DXF_ASCII_DECODE_SUCCESS) {
                cad_ir_document_destroy(&decoder.document);
                entity_builder_destroy(&decoder.entity);
                return result(status, pair.code_line_number);
            }
        } else if (decoder.section == SECTION_ENTITIES) {
            if (pair.code == 0) {
                DxfAsciiDecodeCode status = start_entity(&decoder, pair.value);
                if (status != DXF_ASCII_DECODE_SUCCESS) {
                    cad_ir_document_destroy(&decoder.document);
                    entity_builder_destroy(&decoder.entity);
                    return result(status, pair.code_line_number);
                }
            } else if (span_empty(decoder.entity.kind)) {
                cad_ir_document_destroy(&decoder.document);
                entity_builder_destroy(&decoder.entity);
                return result(DXF_ASCII_DECODE_MALFORMED, pair.code_line_number);
            } else {
                DxfAsciiDecodeCode status = parse_entity_pair(&decoder.entity, &pair);
                if (status != DXF_ASCII_DECODE_SUCCESS) {
                    cad_ir_document_destroy(&decoder.document);
                    entity_builder_destroy(&decoder.entity);
                    return result(status, pair.code_line_number);
                }
            }
        }
    }

    if (!decoder.saw_eof || decoder.section != SECTION_NONE || decoder.expecting_section_name ||
        !decoder.saw_header || !decoder.saw_entities) {
        cad_ir_document_destroy(&decoder.document);
        entity_builder_destroy(&decoder.entity);
        return result(DXF_ASCII_DECODE_MALFORMED, 0);
    }
    if (!decoder.header.has_version) {
        cad_ir_document_destroy(&decoder.document);
        entity_builder_destroy(&decoder.entity);
        return result(DXF_ASCII_DECODE_MISSING_VERSION, 0);
    }
    if (!supported_version(decoder.header.version)) {
        size_t line = decoder.header.version_line_number;
        cad_ir_document_destroy(&decoder.document);
        entity_builder_destroy(&decoder.entity);
        return result(DXF_ASCII_DECODE_UNSUPPORTED_VERSION, line);
    }

    char *version = span_duplicate(decoder.header.version);
    if (version == NULL) {
        cad_ir_document_destroy(&decoder.document);
        entity_builder_destroy(&decoder.entity);
        return result(DXF_ASCII_DECODE_ALLOCATION_FAILED, decoder.header.version_line_number);
    }
    CadIrSourceUnit unit = decoder.header.has_units ?
        unit_from_insunits(decoder.header.units_code) : CAD_IR_UNIT_UNSPECIFIED;
    DxfAsciiDecodeCode status = map_ir_code(cad_ir_document_set_metadata(
        &decoder.document, "DXF", version, unit));
    free(version);
    if (status == DXF_ASCII_DECODE_SUCCESS) {
        status = map_ir_code(cad_ir_document_set_entity_counts(&decoder.document,
            decoder.decoded_count, decoder.skipped_count));
    }
    if (status != DXF_ASCII_DECODE_SUCCESS) {
        cad_ir_document_destroy(&decoder.document);
        entity_builder_destroy(&decoder.entity);
        return result(status, 0);
    }

    entity_builder_destroy(&decoder.entity);
    cad_ir_document_destroy(output);
    *output = decoder.document;
    decoder.document = (CadIrDocument){0};
    return result(DXF_ASCII_DECODE_SUCCESS, 0);
}

DxfAsciiDecodeResult dxf_ascii_decode_memory(const void *bytes, size_t byte_count,
    CadIrDocument *output)
{
    if (bytes == NULL || output == NULL || byte_count == 0) {
        return result(DXF_ASCII_DECODE_INVALID_ARGUMENT, 0);
    }
    const unsigned char *raw = bytes;
    if (is_binary_dxf(raw, byte_count)) {
        return result(DXF_ASCII_DECODE_UNSUPPORTED_REPRESENTATION, 1);
    }
    if (memchr(bytes, '\0', byte_count) != NULL) {
        return result(DXF_ASCII_DECODE_UNSUPPORTED_REPRESENTATION, 0);
    }
    return decode_pairs((const char *)bytes, byte_count, output);
}
