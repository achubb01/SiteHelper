#ifndef SITEHELPER_DXF_ASCII_H
#define SITEHELPER_DXF_ASCII_H

#include <stddef.h>

#include "cad_ir.h"

/* Priority 30C is a format adapter only. It decodes the 30A ASCII-DXF subset
 * into source-space CadIrDocument data. It performs no Plan-mm conversion,
 * semantic building-object creation, persistence, rendering or project edits. */

typedef enum {
    DXF_ASCII_DECODE_SUCCESS = 0,
    DXF_ASCII_DECODE_INVALID_ARGUMENT,
    DXF_ASCII_DECODE_UNSUPPORTED_REPRESENTATION,
    DXF_ASCII_DECODE_MALFORMED,
    DXF_ASCII_DECODE_MISSING_VERSION,
    DXF_ASCII_DECODE_UNSUPPORTED_VERSION,
    DXF_ASCII_DECODE_ALLOCATION_FAILED,
    DXF_ASCII_DECODE_NUMERIC_OVERFLOW,
    DXF_ASCII_DECODE_IR_FAILURE
} DxfAsciiDecodeCode;

typedef struct {
    DxfAsciiDecodeCode code;
    /* One-based source line where a file-level error was detected when known.
     * Zero means no specific source line (including success). */
    size_t line_number;
} DxfAsciiDecodeResult;

/* Decode one complete in-memory DXF byte sequence.
 *
 * `output` must have been initialized with cad_ir_document_init() (or be a
 * zero-initialized CadIrDocument). On success, its previous owned contents are
 * destroyed and replaced atomically by the decoded document. On any failure,
 * `output` is unchanged.
 *
 * The input memory is borrowed only for the duration of this call; all retained
 * strings/geometry are deep-owned by the resulting CadIrDocument. */
DxfAsciiDecodeResult dxf_ascii_decode_memory(const void *bytes, size_t byte_count,
    CadIrDocument *output);

#endif
