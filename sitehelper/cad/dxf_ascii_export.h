#ifndef SITEHELPER_DXF_ASCII_EXPORT_H
#define SITEHELPER_DXF_ASCII_EXPORT_H

#include <stddef.h>

#include "cad_export_ir.h"

/* Priority 30F writer for the first export contract: ASCII DXF AC1032,
 * model-space geometry, millimetres ($INSUNITS=4), unchanged Plan origin. */

typedef enum {
    DXF_ASCII_EXPORT_SUCCESS = 0,
    DXF_ASCII_EXPORT_INVALID_ARGUMENT,
    DXF_ASCII_EXPORT_INVALID_SOURCE,
    DXF_ASCII_EXPORT_ALLOCATION_FAILED,
    DXF_ASCII_EXPORT_NUMERIC_OVERFLOW,
    DXF_ASCII_EXPORT_INTERNAL_ERROR
} DxfAsciiExportCode;

typedef struct {
    char *bytes;
    size_t byte_count;
} DxfAsciiExportBuffer;

void dxf_ascii_export_buffer_init(DxfAsciiExportBuffer *buffer);
void dxf_ascii_export_buffer_destroy(DxfAsciiExportBuffer *buffer);

/* Serialize normalized export paths into a self-contained ASCII DXF buffer.
 * Two-vertex open paths become LINE. Other paths become LWPOLYLINE with
 * implicit closure. No handles or SiteHelper identities are emitted.
 *
 * output must be initialized/zero-initialized. On failure it is unchanged; on
 * success previous contents are destroyed and replaced atomically. */
DxfAsciiExportCode dxf_ascii_export_memory(const CadExportDocument *document,
    DxfAsciiExportBuffer *output);

#endif
