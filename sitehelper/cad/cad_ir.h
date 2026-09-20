#ifndef SITEHELPER_CAD_IR_H
#define SITEHELPER_CAD_IR_H

#include <stddef.h>
#include <stdint.h>

/* Priority 30B is an interoperability representation, not construction
 * authority. It intentionally contains no DomainId, Project, Wall, Slab,
 * Roof, command, editor, persistence, renderer or third-party CAD types. */

typedef enum {
    CAD_IR_UNIT_UNSPECIFIED = 0,
    CAD_IR_UNIT_INCH,
    CAD_IR_UNIT_FOOT,
    CAD_IR_UNIT_MILLIMETRE,
    CAD_IR_UNIT_CENTIMETRE,
    CAD_IR_UNIT_METRE,
    CAD_IR_UNIT_OTHER
} CadIrSourceUnit;

/* Exact finite decimal: coefficient * 10^exponent10. This preserves source
 * decimal intent for later checked unit conversion without introducing an
 * early floating-point rounding decision. Parsing/normalisation belongs to the
 * format adapter; 30B merely owns the exact value. */
typedef struct {
    int64_t coefficient;
    int32_t exponent10;
} CadIrDecimal;

typedef struct {
    CadIrDecimal x;
    CadIrDecimal y;
} CadIrPoint2;

/* Owned source provenance. Strings are independent copies and never borrowed
 * from parser buffers/library objects. handle is optional and opaque; it is
 * never interpreted as SiteHelper identity. */
typedef struct {
    size_t entity_ordinal;
    char *entity_kind;
    char *layer;
    char *handle;
} CadIrProvenance;

typedef struct {
    CadIrPoint2 *vertices;
    size_t vertex_count;
    int closed;
    CadIrProvenance provenance;
} CadIrPath;

typedef enum {
    CAD_IR_DIAGNOSTIC_INFO = 1,
    CAD_IR_DIAGNOSTIC_WARNING,
    CAD_IR_DIAGNOSTIC_ERROR
} CadIrDiagnosticSeverity;

/* Diagnostic code is deliberately an owned string rather than a closed enum:
 * adapters/mapping stages can use stable contract vocabulary without forcing
 * every future CAD format into DXF-specific C enumerators. detail is optional.
 * A diagnostic may be global (has_source == 0) or tied to copied provenance. */
typedef struct {
    CadIrDiagnosticSeverity severity;
    char *code;
    char *detail;
    int has_source;
    CadIrProvenance provenance;
} CadIrDiagnostic;

/* SiteHelper-owned external-drawing snapshot. It is mutable only through the
 * 30B helpers while an adapter builds it. Do not shallow-copy into another
 * owner. decoded/skipped counts describe source entities observed by the
 * adapter; path_count is the supported straight source geometry retained in
 * source coordinates before Plan-mm mapping. */
typedef struct {
    char *source_format;
    char *source_version;
    CadIrSourceUnit declared_unit;

    CadIrPath *paths;
    size_t path_count;
    size_t path_capacity;

    CadIrDiagnostic *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;

    size_t decoded_entity_count;
    size_t skipped_entity_count;
} CadIrDocument;

typedef struct {
    size_t decoded_entity_count;
    size_t source_path_count;
    size_t skipped_entity_count;
    size_t info_count;
    size_t warning_count;
    size_t error_count;
} CadIrStatistics;

typedef enum {
    CAD_IR_SUCCESS = 0,
    CAD_IR_INVALID_ARGUMENT,
    CAD_IR_INVALID_STATE,
    CAD_IR_INVALID_METADATA,
    CAD_IR_INVALID_PATH,
    CAD_IR_INVALID_DIAGNOSTIC,
    CAD_IR_INVALID_COUNTS,
    CAD_IR_ALLOCATION_FAILED,
    CAD_IR_NUMERIC_OVERFLOW
} CadIrCode;

typedef struct {
    size_t entity_ordinal;
    const char *entity_kind;
    const char *layer;
    const char *handle; /* Optional. */
} CadIrProvenanceInput;

typedef struct {
    const CadIrPoint2 *vertices;
    size_t vertex_count;
    int closed;
    CadIrProvenanceInput provenance;
} CadIrPathInput;

typedef struct {
    CadIrDiagnosticSeverity severity;
    const char *code;
    const char *detail; /* Optional. */
    int has_source;
    CadIrProvenanceInput provenance; /* Ignored when has_source == 0. */
} CadIrDiagnosticInput;

void cad_ir_document_init(CadIrDocument *document);
void cad_ir_document_destroy(CadIrDocument *document);

/* Deep-copy/replace source metadata. format/version must both be non-empty.
 * declared_unit may be UNSPECIFIED/OTHER so adapters can preserve unresolved
 * unit state for later mapping. Failure leaves existing metadata unchanged. */
CadIrCode cad_ir_document_set_metadata(CadIrDocument *document,
    const char *source_format, const char *source_version,
    CadIrSourceUnit declared_unit);

/* Deep-copy append operations. LINE is represented as an open two-vertex path;
 * straight LWPOLYLINE is represented as an open/closed ordered path. 30B does
 * not invent entity semantics beyond that shared straight-path vocabulary.
 * Every failure leaves the document contents/counts unchanged. */
CadIrCode cad_ir_document_append_path(CadIrDocument *document,
    const CadIrPathInput *path);
CadIrCode cad_ir_document_append_diagnostic(CadIrDocument *document,
    const CadIrDiagnosticInput *diagnostic);

/* Adapter accounting. skipped may not exceed decoded. Path count is deliberately
 * separate: later formats may decode source entities that produce no retained
 * path, and 30B does not conflate parser accounting with geometry ownership. */
CadIrCode cad_ir_document_set_entity_counts(CadIrDocument *document,
    size_t decoded, size_t skipped);

/* Value-only summary. NULL returns all zeros. Diagnostic severity counts are
 * derived from owned diagnostics rather than separately mutable bookkeeping. */
CadIrStatistics cad_ir_document_statistics(const CadIrDocument *document);

#endif
