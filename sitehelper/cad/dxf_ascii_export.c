#include "dxf_ascii_export.h"

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} TextBuffer;

static int layer_valid(const char *layer)
{
    if (layer == NULL || layer[0] == '\0') { return 0; }
    for (const unsigned char *p = (const unsigned char *)layer; *p != '\0'; p++) {
        if (*p == '\r' || *p == '\n') { return 0; }
    }
    return 1;
}

static int point_equal(CadExportPoint2 a, CadExportPoint2 b)
{
    return a.x_mm == b.x_mm && a.y_mm == b.y_mm;
}

static int document_valid(const CadExportDocument *document)
{
    if (document == NULL || document->path_count > document->path_capacity ||
        (document->path_capacity != 0 && document->paths == NULL)) {
        return 0;
    }
    for (size_t i = 0; i < document->path_count; i++) {
        const CadExportPath *path = &document->paths[i];
        if (path->vertices == NULL || path->vertex_count < 2 ||
            (path->closed && path->vertex_count < 3) || !layer_valid(path->layer) ||
            (path->closed && point_equal(path->vertices[0], path->vertices[path->vertex_count - 1])) ||
            path->vertex_count > INT32_MAX) {
            return 0;
        }
    }
    return 1;
}

void dxf_ascii_export_buffer_init(DxfAsciiExportBuffer *buffer)
{
    if (buffer != NULL) { *buffer = (DxfAsciiExportBuffer){0}; }
}

void dxf_ascii_export_buffer_destroy(DxfAsciiExportBuffer *buffer)
{
    if (buffer == NULL) { return; }
    free(buffer->bytes);
    *buffer = (DxfAsciiExportBuffer){0};
}

static DxfAsciiExportCode reserve(TextBuffer *buffer, size_t additional)
{
    if (additional > SIZE_MAX - buffer->length - 1) {
        return DXF_ASCII_EXPORT_NUMERIC_OVERFLOW;
    }
    size_t required = buffer->length + additional + 1;
    if (required <= buffer->capacity) { return DXF_ASCII_EXPORT_SUCCESS; }
    size_t maximum = SIZE_MAX / sizeof *buffer->data;
    size_t grown = buffer->capacity == 0 ? 256 : buffer->capacity;
    while (grown < required) {
        if (grown > maximum / 2) {
            grown = maximum;
            break;
        }
        grown *= 2;
    }
    if (grown < required) { return DXF_ASCII_EXPORT_NUMERIC_OVERFLOW; }
    char *storage = realloc(buffer->data, grown);
    if (storage == NULL) { return DXF_ASCII_EXPORT_ALLOCATION_FAILED; }
    buffer->data = storage;
    buffer->capacity = grown;
    return DXF_ASCII_EXPORT_SUCCESS;
}

static DxfAsciiExportCode append_bytes(TextBuffer *buffer, const char *text, size_t length)
{
    DxfAsciiExportCode code = reserve(buffer, length);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    memcpy(buffer->data + buffer->length, text, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return DXF_ASCII_EXPORT_SUCCESS;
}

static DxfAsciiExportCode append_text(TextBuffer *buffer, const char *text)
{
    return append_bytes(buffer, text, strlen(text));
}

static DxfAsciiExportCode append_group_text(TextBuffer *buffer, int group, const char *value)
{
    char prefix[32];
    int count = snprintf(prefix, sizeof prefix, "%d\n", group);
    if (count < 0 || (size_t)count >= sizeof prefix) { return DXF_ASCII_EXPORT_INTERNAL_ERROR; }
    DxfAsciiExportCode code = append_bytes(buffer, prefix, (size_t)count);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_text(buffer, value);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    return append_text(buffer, "\n");
}

static DxfAsciiExportCode append_group_i64(TextBuffer *buffer, int group, int64_t value)
{
    char line[96];
    int count = snprintf(line, sizeof line, "%d\n%" PRId64 "\n", group, value);
    if (count < 0 || (size_t)count >= sizeof line) { return DXF_ASCII_EXPORT_INTERNAL_ERROR; }
    return append_bytes(buffer, line, (size_t)count);
}

static int same_layer(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static int first_layer_occurrence(const CadExportDocument *document, size_t index)
{
    for (size_t i = 0; i < index; i++) {
        if (same_layer(document->paths[i].layer, document->paths[index].layer)) { return 0; }
    }
    return 1;
}

static size_t unique_layer_count(const CadExportDocument *document)
{
    size_t count = 0;
    int has_zero = 0;
    for (size_t i = 0; i < document->path_count; i++) {
        if (!first_layer_occurrence(document, i)) { continue; }
        count++;
        if (same_layer(document->paths[i].layer, "0")) { has_zero = 1; }
    }
    return count + (has_zero ? 0u : 1u);
}

static DxfAsciiExportCode append_layer(TextBuffer *buffer, const char *layer)
{
    DxfAsciiExportCode code = append_group_text(buffer, 0, "LAYER");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 100, "AcDbSymbolTableRecord");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 100, "AcDbLayerTableRecord");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 2, layer);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 70, 0);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 62, 7);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    return append_group_text(buffer, 6, "CONTINUOUS");
}

static DxfAsciiExportCode append_line_entity(TextBuffer *buffer, const CadExportPath *path)
{
    DxfAsciiExportCode code = append_group_text(buffer, 0, "LINE");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 100, "AcDbEntity");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 8, path->layer);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 67, 0);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 410, "Model");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 100, "AcDbLine");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 10, path->vertices[0].x_mm);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 20, path->vertices[0].y_mm);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 30, 0);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 11, path->vertices[1].x_mm);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 21, path->vertices[1].y_mm);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    return append_group_i64(buffer, 31, 0);
}

static DxfAsciiExportCode append_polyline_entity(TextBuffer *buffer, const CadExportPath *path)
{
    DxfAsciiExportCode code = append_group_text(buffer, 0, "LWPOLYLINE");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 100, "AcDbEntity");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 8, path->layer);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 67, 0);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 410, "Model");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_text(buffer, 100, "AcDbPolyline");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 90, (int64_t)path->vertex_count);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    code = append_group_i64(buffer, 70, path->closed ? 1 : 0);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    for (size_t i = 0; i < path->vertex_count; i++) {
        code = append_group_i64(buffer, 10, path->vertices[i].x_mm);
        if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
        code = append_group_i64(buffer, 20, path->vertices[i].y_mm);
        if (code != DXF_ASCII_EXPORT_SUCCESS) { return code; }
    }
    return DXF_ASCII_EXPORT_SUCCESS;
}

DxfAsciiExportCode dxf_ascii_export_memory(const CadExportDocument *document,
    DxfAsciiExportBuffer *output)
{
    if (document == NULL || output == NULL) { return DXF_ASCII_EXPORT_INVALID_ARGUMENT; }
    if (!document_valid(document)) { return DXF_ASCII_EXPORT_INVALID_SOURCE; }
    size_t layer_count = unique_layer_count(document);
    if (layer_count > INT16_MAX) { return DXF_ASCII_EXPORT_NUMERIC_OVERFLOW; }

    TextBuffer candidate = {0};
    DxfAsciiExportCode code = append_group_text(&candidate, 0, "SECTION");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 2, "HEADER");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 9, "$ACADVER");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 1, "AC1032");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 9, "$INSUNITS");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_i64(&candidate, 70, 4);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 0, "ENDSEC");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }

    code = append_group_text(&candidate, 0, "SECTION");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 2, "TABLES");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 0, "TABLE");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 2, "LAYER");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_i64(&candidate, 70, (int64_t)layer_count);
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_layer(&candidate, "0");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    for (size_t i = 0; i < document->path_count; i++) {
        if (!first_layer_occurrence(document, i) || same_layer(document->paths[i].layer, "0")) {
            continue;
        }
        code = append_layer(&candidate, document->paths[i].layer);
        if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    }
    code = append_group_text(&candidate, 0, "ENDTAB");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 0, "ENDSEC");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }

    code = append_group_text(&candidate, 0, "SECTION");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 2, "ENTITIES");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    for (size_t i = 0; i < document->path_count; i++) {
        const CadExportPath *path = &document->paths[i];
        code = (!path->closed && path->vertex_count == 2) ?
            append_line_entity(&candidate, path) : append_polyline_entity(&candidate, path);
        if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    }
    code = append_group_text(&candidate, 0, "ENDSEC");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }
    code = append_group_text(&candidate, 0, "EOF");
    if (code != DXF_ASCII_EXPORT_SUCCESS) { goto fail; }

    DxfAsciiExportBuffer replacement = {candidate.data, candidate.length};
    candidate = (TextBuffer){0};
    dxf_ascii_export_buffer_destroy(output);
    *output = replacement;
    return DXF_ASCII_EXPORT_SUCCESS;

fail:
    free(candidate.data);
    return code;
}
