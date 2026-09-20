#ifndef SITEHELPER_CAD_EXPORT_IR_H
#define SITEHELPER_CAD_EXPORT_IR_H

#include <stddef.h>
#include <stdint.h>

/* Priority 30F output-side interoperability representation. It is normalized
 * SiteHelper Plan geometry prepared for external-format adapters, not project
 * authority and not an external CAD object model. In particular it contains no
 * DomainId, Project, Wall, Slab, DXF handle or third-party library types. */

typedef struct {
    int64_t x_mm;
    int64_t y_mm;
} CadExportPoint2;

typedef struct {
    CadExportPoint2 *vertices;
    size_t vertex_count;
    int closed;
    char *layer;
} CadExportPath;

typedef struct {
    CadExportPath *paths;
    size_t path_count;
    size_t path_capacity;
} CadExportDocument;

typedef enum {
    CAD_EXPORT_IR_SUCCESS = 0,
    CAD_EXPORT_IR_INVALID_ARGUMENT,
    CAD_EXPORT_IR_INVALID_STATE,
    CAD_EXPORT_IR_INVALID_PATH,
    CAD_EXPORT_IR_ALLOCATION_FAILED,
    CAD_EXPORT_IR_NUMERIC_OVERFLOW
} CadExportIrCode;

typedef struct {
    const CadExportPoint2 *vertices;
    size_t vertex_count;
    int closed;
    const char *layer;
} CadExportPathInput;

void cad_export_document_init(CadExportDocument *document);
void cad_export_document_destroy(CadExportDocument *document);

/* Deep-copy one normalized integer-millimetre path. Open paths require at
 * least two vertices; closed paths require at least three. Closure is implicit
 * and the first vertex must not be repeated solely to close a polygon. Layer is
 * an owned logical export layer name; external adapters decide how to encode it.
 * Failure leaves the document unchanged. */
CadExportIrCode cad_export_document_append_path(CadExportDocument *document,
    const CadExportPathInput *path);

#endif
