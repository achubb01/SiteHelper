#include "cad_export_ir.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int collection_valid(const CadExportDocument *document)
{
    return document != NULL && document->path_count <= document->path_capacity &&
        (document->path_capacity == 0 || document->paths != NULL);
}

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

void cad_export_document_init(CadExportDocument *document)
{
    if (document != NULL) { *document = (CadExportDocument){0}; }
}

void cad_export_document_destroy(CadExportDocument *document)
{
    if (document == NULL) { return; }
    if (document->paths != NULL) {
        for (size_t i = 0; i < document->path_count; i++) {
            free(document->paths[i].vertices);
            free(document->paths[i].layer);
        }
    }
    free(document->paths);
    *document = (CadExportDocument){0};
}

static CadExportIrCode grow_paths(CadExportDocument *document)
{
    if (document->path_count < document->path_capacity) { return CAD_EXPORT_IR_SUCCESS; }
    size_t maximum = SIZE_MAX / sizeof *document->paths;
    if (document->path_capacity >= maximum) { return CAD_EXPORT_IR_NUMERIC_OVERFLOW; }
    size_t grown = document->path_capacity == 0 ? 1 :
        document->path_capacity > maximum / 2 ? maximum : document->path_capacity * 2;
    CadExportPath *storage = realloc(document->paths, grown * sizeof *storage);
    if (storage == NULL) { return CAD_EXPORT_IR_ALLOCATION_FAILED; }
    document->paths = storage;
    document->path_capacity = grown;
    return CAD_EXPORT_IR_SUCCESS;
}

CadExportIrCode cad_export_document_append_path(CadExportDocument *document,
    const CadExportPathInput *path)
{
    if (document == NULL || path == NULL) { return CAD_EXPORT_IR_INVALID_ARGUMENT; }
    if (!collection_valid(document)) { return CAD_EXPORT_IR_INVALID_STATE; }
    if (path->vertices == NULL || path->vertex_count < 2 ||
        (path->closed && path->vertex_count < 3) || !layer_valid(path->layer)) {
        return CAD_EXPORT_IR_INVALID_PATH;
    }
    if (path->closed && point_equal(path->vertices[0], path->vertices[path->vertex_count - 1])) {
        return CAD_EXPORT_IR_INVALID_PATH;
    }
    if (path->vertex_count > SIZE_MAX / sizeof *path->vertices) {
        return CAD_EXPORT_IR_NUMERIC_OVERFLOW;
    }

    CadExportPoint2 *vertices = malloc(path->vertex_count * sizeof *vertices);
    if (vertices == NULL) { return CAD_EXPORT_IR_ALLOCATION_FAILED; }
    memcpy(vertices, path->vertices, path->vertex_count * sizeof *vertices);

    size_t layer_length = strlen(path->layer);
    if (layer_length == SIZE_MAX) {
        free(vertices);
        return CAD_EXPORT_IR_NUMERIC_OVERFLOW;
    }
    char *layer = malloc(layer_length + 1);
    if (layer == NULL) {
        free(vertices);
        return CAD_EXPORT_IR_ALLOCATION_FAILED;
    }
    memcpy(layer, path->layer, layer_length + 1);

    CadExportIrCode code = grow_paths(document);
    if (code != CAD_EXPORT_IR_SUCCESS) {
        free(layer);
        free(vertices);
        return code;
    }

    document->paths[document->path_count++] = (CadExportPath){
        .vertices = vertices,
        .vertex_count = path->vertex_count,
        .closed = path->closed != 0,
        .layer = layer
    };
    return CAD_EXPORT_IR_SUCCESS;
}
