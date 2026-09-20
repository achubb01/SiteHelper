#ifndef SITEHELPER_DOCUMENT_H
#define SITEHELPER_DOCUMENT_H

#include <stddef.h>

#include "domain_id.h"
#include "position.h"

/* Non-physical authored information. Notes, dimensions, concrete Plan symbols
 * and callouts remain distinct payload families; revision graphics, sheets and
 * styling are separate semantics rather than variants guessed into a universal
 * annotation base type. */
typedef enum
{
    DOCUMENT_ANNOTATION_NOTE = 1
} DocumentAnnotationKind;

typedef struct
{
    DomainId storey_id;
    PlanPosition position;
} DocumentPlanAnchor;

typedef struct
{
    DomainId id;
    DocumentAnnotationKind kind;
    DocumentPlanAnchor anchor;
    /* Optional association to a physical model object on anchor.storey_id.
     * DOMAIN_ID_INVALID means the note is free-standing. This is a durable,
     * weak ID association rather than a borrowed pointer: deletion of the
     * target may leave the ID unresolved until history restores it. */
    DomainId target_id;
    char *text;
} DocumentAnnotation;

typedef enum
{
    DOCUMENT_DIMENSION_FIXED_POINT = 1,
    DOCUMENT_DIMENSION_WALL_START,
    DOCUMENT_DIMENSION_WALL_END
} DocumentDimensionReferenceKind;

typedef struct
{
    DocumentDimensionReferenceKind kind;
    /* FIXED_POINT uses position and requires target_id == 0. Wall endpoint
     * references use target_id and ignore position. Associations are weak stable
     * IDs so physical deletion does not destroy authored document authority. */
    DomainId target_id;
    PlanPosition position;
} DocumentDimensionReference;

typedef struct
{
    DomainId id;
    DomainId storey_id;
    DocumentDimensionReference first;
    DocumentDimensionReference second;
    /* Signed perpendicular placement offset for later rendering/authoring. It is
     * document presentation authority, never part of the measured value. */
    int offset_mm;
} DocumentPlanDimension;

/* Concrete Plan symbol family. Point markers carry only an anchor; view-direction
 * markers add exact orientation without yet predicting section/detail identifiers
 * or sheet-reference semantics. */
typedef enum
{
    DOCUMENT_PLAN_SYMBOL_POINT_MARKER = 1,
    /* Oriented construction/document marker. The direction is an exact
     * primitive integer vector and represents viewing/reference direction,
     * not a persisted presentation length. */
    DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION = 2
} DocumentPlanSymbolKind;

typedef struct
{
    int dx;
    int dy;
} DocumentPlanDirection;

typedef struct
{
    DomainId id;
    DomainId storey_id;
    DocumentPlanSymbolKind kind;
    PlanPosition anchor;
    /* POINT_MARKER requires {0,0}. VIEW_DIRECTION requires a non-zero,
     * primitive vector (gcd(abs(dx), abs(dy)) == 1). */
    DocumentPlanDirection direction;
} DocumentPlanSymbol;

/* A leader/callout is deliberately separate from notes and symbols. The two
 * authored Plan points define the referenced target and the text/leader anchor;
 * line caps/arrowheads are presentation geometry, not persisted authority. */
typedef struct
{
    DomainId id;
    DomainId storey_id;
    PlanPosition target;
    PlanPosition label_anchor;
    char *text;
} DocumentPlanCallout;

/* A project-level revision record groups markup without pretending that issue
 * dates, authorship or sheet-publication state already exist. The identifier is
 * authored human-facing authority (for example "A" or "P2"); description is
 * optional supporting text. Revision records are not Plan-selectable objects. */
typedef struct
{
    DomainId id;
    char *identifier;
    char *description;
} DocumentRevision;

/* Revision markup is project-owned document authority, but remains a distinct
 * payload family from permanent notes/dimensions/symbols/callouts. The closed
 * boundary is authoritative; scallops/arc presentation are derived. The first
 * vertex is not repeated at the end. revision_id is an optional weak reference
 * to a project-level DocumentRevision; DOMAIN_ID_INVALID means unassigned. */
typedef struct
{
    DomainId id;
    DomainId storey_id;
    DomainId revision_id;
    PlanPosition *vertices;
    size_t vertex_count;
} DocumentPlanRevisionCloud;

typedef enum
{
    DOCUMENT_OBJECT_NONE = 0,
    DOCUMENT_OBJECT_NOTE,
    DOCUMENT_OBJECT_DIMENSION,
    DOCUMENT_OBJECT_SYMBOL,
    DOCUMENT_OBJECT_CALLOUT,
    DOCUMENT_OBJECT_REVISION_CLOUD
} DocumentObjectKind;

typedef struct
{
    DocumentObjectKind kind;
    DomainId id;
} DocumentObjectRef;

typedef struct
{
    DocumentAnnotation *annotations;
    size_t annotation_count;
    size_t annotation_capacity;
    DocumentPlanDimension *dimensions;
    size_t dimension_count;
    size_t dimension_capacity;
    DocumentPlanSymbol *symbols;
    size_t symbol_count;
    size_t symbol_capacity;
    DocumentPlanCallout *callouts;
    size_t callout_count;
    size_t callout_capacity;
    DocumentRevision *revisions;
    size_t revision_count;
    size_t revision_capacity;
    DocumentPlanRevisionCloud *revision_clouds;
    size_t revision_cloud_count;
    size_t revision_cloud_capacity;
} DocumentModel;

typedef enum
{
    DOCUMENT_SUCCESS = 0,
    DOCUMENT_INVALID_ARGUMENT,
    DOCUMENT_INVALID_ANNOTATION,
    DOCUMENT_DUPLICATE_ID,
    DOCUMENT_ALLOCATION_FAILED,
    DOCUMENT_NOT_FOUND
} DocumentCode;

void document_model_init(DocumentModel *document);
void document_model_destroy(DocumentModel *document);

int document_object_ref_is_valid(DocumentObjectRef reference);
/* Returns the object's explicit Plan Storey scope, or DOMAIN_ID_INVALID if the
 * reference is malformed or no longer resolves. This is document context, not
 * physical ownership. */
DomainId document_model_object_storey_id(const DocumentModel *document,
    DocumentObjectRef reference);

/* Local/value validation only. Project-level validation additionally resolves
 * the Storey scope and optional target relationship. */
int document_annotation_is_locally_valid(const DocumentAnnotation *annotation);
/* Owned-value helpers for command history and transactional replacement. */
DocumentCode document_annotation_clone(const DocumentAnnotation *source,
    DocumentAnnotation *output);
void document_annotation_destroy(DocumentAnnotation *annotation);

DocumentAnnotation *document_model_find_annotation_by_id(DocumentModel *document, DomainId id);
const DocumentAnnotation *document_model_find_annotation_by_id_const(
    const DocumentModel *document, DomainId id);

/* Deep-copy insertion with an existing identity. Failure leaves the collection
 * unchanged. The caller owns the source text and retains it after the call. */
DocumentCode document_model_insert_annotation(
    DocumentModel *document, const DocumentAnnotation *annotation);
DocumentCode document_model_remove_annotation(DocumentModel *document, DomainId id);

int document_dimension_reference_is_locally_valid(const DocumentDimensionReference *reference);
int document_plan_dimension_is_locally_valid(const DocumentPlanDimension *dimension);
DocumentPlanDimension *document_model_find_dimension_by_id(DocumentModel *document, DomainId id);
const DocumentPlanDimension *document_model_find_dimension_by_id_const(
    const DocumentModel *document, DomainId id);
DocumentCode document_model_insert_dimension(DocumentModel *document,
    const DocumentPlanDimension *dimension);
DocumentCode document_model_remove_dimension(DocumentModel *document, DomainId id);

int document_plan_direction_is_canonical(DocumentPlanDirection direction);
int document_plan_direction_from_points(PlanPosition anchor, PlanPosition direction_point,
    DocumentPlanDirection *direction);
int document_plan_symbol_is_locally_valid(const DocumentPlanSymbol *symbol);
DocumentPlanSymbol *document_model_find_symbol_by_id(DocumentModel *document, DomainId id);
const DocumentPlanSymbol *document_model_find_symbol_by_id_const(
    const DocumentModel *document, DomainId id);
DocumentCode document_model_insert_symbol(DocumentModel *document,
    const DocumentPlanSymbol *symbol);
DocumentCode document_model_remove_symbol(DocumentModel *document, DomainId id);

int document_plan_callout_is_locally_valid(const DocumentPlanCallout *callout);
DocumentCode document_plan_callout_clone(const DocumentPlanCallout *source,
    DocumentPlanCallout *output);
void document_plan_callout_destroy(DocumentPlanCallout *callout);
DocumentPlanCallout *document_model_find_callout_by_id(DocumentModel *document, DomainId id);
const DocumentPlanCallout *document_model_find_callout_by_id_const(
    const DocumentModel *document, DomainId id);
DocumentCode document_model_insert_callout(DocumentModel *document,
    const DocumentPlanCallout *callout);
DocumentCode document_model_remove_callout(DocumentModel *document, DomainId id);

int document_revision_is_locally_valid(const DocumentRevision *revision);
DocumentCode document_revision_clone(const DocumentRevision *source,
    DocumentRevision *output);
void document_revision_destroy(DocumentRevision *revision);
DocumentRevision *document_model_find_revision_by_id(DocumentModel *document, DomainId id);
const DocumentRevision *document_model_find_revision_by_id_const(
    const DocumentModel *document, DomainId id);
DocumentCode document_model_insert_revision(DocumentModel *document,
    const DocumentRevision *revision);
DocumentCode document_model_remove_revision(DocumentModel *document, DomainId id);

int document_plan_revision_cloud_is_locally_valid(
    const DocumentPlanRevisionCloud *cloud);
DocumentCode document_plan_revision_cloud_clone(
    const DocumentPlanRevisionCloud *source, DocumentPlanRevisionCloud *output);
void document_plan_revision_cloud_destroy(DocumentPlanRevisionCloud *cloud);
DocumentPlanRevisionCloud *document_model_find_revision_cloud_by_id(
    DocumentModel *document, DomainId id);
const DocumentPlanRevisionCloud *document_model_find_revision_cloud_by_id_const(
    const DocumentModel *document, DomainId id);
DocumentCode document_model_insert_revision_cloud(DocumentModel *document,
    const DocumentPlanRevisionCloud *cloud);
DocumentCode document_model_remove_revision_cloud(DocumentModel *document, DomainId id);

#endif
