#include "document.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void document_model_init(DocumentModel *document)
{
    if (document != NULL) {
        *document = (DocumentModel){0};
    }
}

int document_object_ref_is_valid(DocumentObjectRef reference)
{
    return reference.id != DOMAIN_ID_INVALID &&
        reference.kind >= DOCUMENT_OBJECT_NOTE &&
        reference.kind <= DOCUMENT_OBJECT_REVISION_CLOUD;
}

DomainId document_model_object_storey_id(const DocumentModel *document,
    DocumentObjectRef reference)
{
    if (document == NULL || !document_object_ref_is_valid(reference)) {
        return DOMAIN_ID_INVALID;
    }
    switch (reference.kind) {
        case DOCUMENT_OBJECT_NOTE: {
            const DocumentAnnotation *note = document_model_find_annotation_by_id_const(
                document, reference.id);
            return note != NULL && note->kind == DOCUMENT_ANNOTATION_NOTE
                ? note->anchor.storey_id : DOMAIN_ID_INVALID;
        }
        case DOCUMENT_OBJECT_DIMENSION: {
            const DocumentPlanDimension *dimension = document_model_find_dimension_by_id_const(
                document, reference.id);
            return dimension != NULL ? dimension->storey_id : DOMAIN_ID_INVALID;
        }
        case DOCUMENT_OBJECT_SYMBOL: {
            const DocumentPlanSymbol *symbol = document_model_find_symbol_by_id_const(
                document, reference.id);
            return symbol != NULL ? symbol->storey_id : DOMAIN_ID_INVALID;
        }
        case DOCUMENT_OBJECT_CALLOUT: {
            const DocumentPlanCallout *callout = document_model_find_callout_by_id_const(
                document, reference.id);
            return callout != NULL ? callout->storey_id : DOMAIN_ID_INVALID;
        }
        case DOCUMENT_OBJECT_REVISION_CLOUD: {
            const DocumentPlanRevisionCloud *cloud =
                document_model_find_revision_cloud_by_id_const(document, reference.id);
            return cloud != NULL ? cloud->storey_id : DOMAIN_ID_INVALID;
        }
        case DOCUMENT_OBJECT_NONE:
        default:
            return DOMAIN_ID_INVALID;
    }
}

void document_annotation_destroy(DocumentAnnotation *annotation)
{
    if (annotation == NULL) {
        return;
    }
    free(annotation->text);
    *annotation = (DocumentAnnotation){0};
}

void document_model_destroy(DocumentModel *document)
{
    if (document == NULL) {
        return;
    }
    for (size_t i = 0; i < document->annotation_count; i++) {
        document_annotation_destroy(&document->annotations[i]);
    }
    free(document->annotations);
    free(document->dimensions);
    free(document->symbols);
    for (size_t i = 0; i < document->callout_count; i++) {
        document_plan_callout_destroy(&document->callouts[i]);
    }
    free(document->callouts);
    for (size_t i = 0; i < document->revision_count; i++) {
        document_revision_destroy(&document->revisions[i]);
    }
    free(document->revisions);
    for (size_t i = 0; i < document->revision_cloud_count; i++) {
        document_plan_revision_cloud_destroy(&document->revision_clouds[i]);
    }
    free(document->revision_clouds);
    *document = (DocumentModel){0};
}

int document_annotation_is_locally_valid(const DocumentAnnotation *annotation)
{
    return annotation != NULL &&
        annotation->id != DOMAIN_ID_INVALID &&
        annotation->kind == DOCUMENT_ANNOTATION_NOTE &&
        annotation->anchor.storey_id != DOMAIN_ID_INVALID &&
        annotation->text != NULL && annotation->text[0] != '\0';
}

DocumentCode document_annotation_clone(const DocumentAnnotation *source,
    DocumentAnnotation *output)
{
    if (source == NULL || output == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_annotation_is_locally_valid(source)) { return DOCUMENT_INVALID_ANNOTATION; }
    size_t length = strlen(source->text);
    char *text = malloc(length + 1);
    if (text == NULL) { return DOCUMENT_ALLOCATION_FAILED; }
    memcpy(text, source->text, length + 1);
    *output = *source;
    output->text = text;
    return DOCUMENT_SUCCESS;
}

DocumentAnnotation *document_model_find_annotation_by_id(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) {
        return NULL;
    }
    for (size_t i = 0; i < document->annotation_count; i++) {
        if (document->annotations[i].id == id) {
            return &document->annotations[i];
        }
    }
    return NULL;
}

const DocumentAnnotation *document_model_find_annotation_by_id_const(
    const DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) {
        return NULL;
    }
    for (size_t i = 0; i < document->annotation_count; i++) {
        if (document->annotations[i].id == id) {
            return &document->annotations[i];
        }
    }
    return NULL;
}

static int ensure_capacity(DocumentModel *document, size_t required)
{
    if (required <= document->annotation_capacity) {
        return 1;
    }
    const size_t maximum = SIZE_MAX / sizeof *document->annotations;
    if (required > maximum) {
        return 0;
    }
    size_t capacity = document->annotation_capacity == 0 ? 1 : document->annotation_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) {
            capacity = maximum;
            break;
        }
        capacity *= 2;
    }
    DocumentAnnotation *storage = realloc(document->annotations,
        capacity * sizeof *storage);
    if (storage == NULL) {
        return 0;
    }
    document->annotations = storage;
    document->annotation_capacity = capacity;
    return 1;
}

DocumentCode document_model_insert_annotation(
    DocumentModel *document, const DocumentAnnotation *annotation)
{
    if (document == NULL || annotation == NULL) {
        return DOCUMENT_INVALID_ARGUMENT;
    }
    if (!document_annotation_is_locally_valid(annotation)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document->annotation_count > document->annotation_capacity ||
        (document->annotation_capacity == 0 && document->annotations != NULL) ||
        (document->annotation_capacity != 0 && document->annotations == NULL)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document_model_find_annotation_by_id_const(document, annotation->id) != NULL) {
        return DOCUMENT_DUPLICATE_ID;
    }

    DocumentAnnotation copy = {0};
    DocumentCode cloned = document_annotation_clone(annotation, &copy);
    if (cloned != DOCUMENT_SUCCESS) { return cloned; }
    if (!ensure_capacity(document, document->annotation_count + 1)) {
        document_annotation_destroy(&copy);
        return DOCUMENT_ALLOCATION_FAILED;
    }

    document->annotations[document->annotation_count++] = copy;
    return DOCUMENT_SUCCESS;
}

DocumentCode document_model_remove_annotation(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) {
        return DOCUMENT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < document->annotation_count; i++) {
        if (document->annotations[i].id != id) {
            continue;
        }
        document_annotation_destroy(&document->annotations[i]);
        if (i + 1 < document->annotation_count) {
            memmove(&document->annotations[i], &document->annotations[i + 1],
                (document->annotation_count - i - 1) * sizeof *document->annotations);
        }
        document->annotation_count--;
        document->annotations[document->annotation_count] = (DocumentAnnotation){0};
        return DOCUMENT_SUCCESS;
    }
    return DOCUMENT_NOT_FOUND;
}


int document_dimension_reference_is_locally_valid(const DocumentDimensionReference *reference)
{
    if (reference == NULL) { return 0; }
    switch (reference->kind) {
        case DOCUMENT_DIMENSION_FIXED_POINT:
            return reference->target_id == DOMAIN_ID_INVALID;
        case DOCUMENT_DIMENSION_WALL_START:
        case DOCUMENT_DIMENSION_WALL_END:
            return reference->target_id != DOMAIN_ID_INVALID;
        default:
            return 0;
    }
}

int document_plan_dimension_is_locally_valid(const DocumentPlanDimension *dimension)
{
    return dimension != NULL && dimension->id != DOMAIN_ID_INVALID &&
        dimension->storey_id != DOMAIN_ID_INVALID &&
        document_dimension_reference_is_locally_valid(&dimension->first) &&
        document_dimension_reference_is_locally_valid(&dimension->second);
}

DocumentPlanDimension *document_model_find_dimension_by_id(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->dimension_count; i++) {
        if (document->dimensions[i].id == id) { return &document->dimensions[i]; }
    }
    return NULL;
}

const DocumentPlanDimension *document_model_find_dimension_by_id_const(
    const DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->dimension_count; i++) {
        if (document->dimensions[i].id == id) { return &document->dimensions[i]; }
    }
    return NULL;
}

static int ensure_dimension_capacity(DocumentModel *document, size_t required)
{
    if (required <= document->dimension_capacity) { return 1; }
    const size_t maximum = SIZE_MAX / sizeof *document->dimensions;
    if (required > maximum) { return 0; }
    size_t capacity = document->dimension_capacity == 0 ? 1 : document->dimension_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) { capacity = maximum; break; }
        capacity *= 2;
    }
    DocumentPlanDimension *storage = realloc(document->dimensions,
        capacity * sizeof *storage);
    if (storage == NULL) { return 0; }
    document->dimensions = storage;
    document->dimension_capacity = capacity;
    return 1;
}

DocumentCode document_model_insert_dimension(DocumentModel *document,
    const DocumentPlanDimension *dimension)
{
    if (document == NULL || dimension == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_plan_dimension_is_locally_valid(dimension)) { return DOCUMENT_INVALID_ANNOTATION; }
    if (document->dimension_count > document->dimension_capacity ||
        (document->dimension_capacity == 0 && document->dimensions != NULL) ||
        (document->dimension_capacity != 0 && document->dimensions == NULL)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document_model_find_dimension_by_id_const(document, dimension->id) != NULL) {
        return DOCUMENT_DUPLICATE_ID;
    }
    if (!ensure_dimension_capacity(document, document->dimension_count + 1)) {
        return DOCUMENT_ALLOCATION_FAILED;
    }
    document->dimensions[document->dimension_count++] = *dimension;
    return DOCUMENT_SUCCESS;
}

DocumentCode document_model_remove_dimension(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return DOCUMENT_INVALID_ARGUMENT; }
    for (size_t i = 0; i < document->dimension_count; i++) {
        if (document->dimensions[i].id != id) { continue; }
        if (i + 1 < document->dimension_count) {
            memmove(&document->dimensions[i], &document->dimensions[i + 1],
                (document->dimension_count - i - 1) * sizeof *document->dimensions);
        }
        document->dimension_count--;
        document->dimensions[document->dimension_count] = (DocumentPlanDimension){0};
        return DOCUMENT_SUCCESS;
    }
    return DOCUMENT_NOT_FOUND;
}


static uint64_t magnitude_int(int value)
{
    return value < 0 ? (uint64_t)(-(int64_t)value) : (uint64_t)value;
}

static uint64_t gcd_u64(uint64_t a, uint64_t b)
{
    while (b != 0) {
        uint64_t remainder=a%b;
        a=b;
        b=remainder;
    }
    return a;
}

int document_plan_direction_is_canonical(DocumentPlanDirection direction)
{
    uint64_t ax=magnitude_int(direction.dx),ay=magnitude_int(direction.dy);
    return (ax != 0 || ay != 0) && gcd_u64(ax,ay) == 1;
}

int document_plan_direction_from_points(PlanPosition anchor, PlanPosition direction_point,
    DocumentPlanDirection *direction)
{
    if (direction == NULL) { return 0; }
    int64_t dx=(int64_t)direction_point.x-anchor.x;
    int64_t dy=(int64_t)direction_point.y-anchor.y;
    if (dx == 0 && dy == 0) { return 0; }
    uint64_t ax=dx < 0 ? (uint64_t)(-dx) : (uint64_t)dx;
    uint64_t ay=dy < 0 ? (uint64_t)(-dy) : (uint64_t)dy;
    uint64_t divisor=gcd_u64(ax,ay);
    dx/=(int64_t)divisor;
    dy/=(int64_t)divisor;
    if (dx < INT32_MIN || dx > INT32_MAX || dy < INT32_MIN || dy > INT32_MAX) { return 0; }
    *direction=(DocumentPlanDirection){(int)dx,(int)dy};
    return document_plan_direction_is_canonical(*direction);
}

int document_plan_symbol_is_locally_valid(const DocumentPlanSymbol *symbol)
{
    if (symbol == NULL || symbol->id == DOMAIN_ID_INVALID ||
        symbol->storey_id == DOMAIN_ID_INVALID) { return 0; }
    if (symbol->kind == DOCUMENT_PLAN_SYMBOL_POINT_MARKER) {
        return symbol->direction.dx == 0 && symbol->direction.dy == 0;
    }
    if (symbol->kind == DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION) {
        return document_plan_direction_is_canonical(symbol->direction);
    }
    return 0;
}

DocumentPlanSymbol *document_model_find_symbol_by_id(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->symbol_count; i++) {
        if (document->symbols[i].id == id) { return &document->symbols[i]; }
    }
    return NULL;
}

const DocumentPlanSymbol *document_model_find_symbol_by_id_const(
    const DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->symbol_count; i++) {
        if (document->symbols[i].id == id) { return &document->symbols[i]; }
    }
    return NULL;
}

static int ensure_symbol_capacity(DocumentModel *document, size_t required)
{
    if (required <= document->symbol_capacity) { return 1; }
    const size_t maximum = SIZE_MAX / sizeof *document->symbols;
    if (required > maximum) { return 0; }
    size_t capacity = document->symbol_capacity == 0 ? 1 : document->symbol_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) { capacity = maximum; break; }
        capacity *= 2;
    }
    DocumentPlanSymbol *storage = realloc(document->symbols, capacity * sizeof *storage);
    if (storage == NULL) { return 0; }
    document->symbols = storage;
    document->symbol_capacity = capacity;
    return 1;
}

DocumentCode document_model_insert_symbol(DocumentModel *document,
    const DocumentPlanSymbol *symbol)
{
    if (document == NULL || symbol == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_plan_symbol_is_locally_valid(symbol)) { return DOCUMENT_INVALID_ANNOTATION; }
    if (document->symbol_count > document->symbol_capacity ||
        (document->symbol_capacity == 0 && document->symbols != NULL) ||
        (document->symbol_capacity != 0 && document->symbols == NULL)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document_model_find_symbol_by_id_const(document, symbol->id) != NULL) {
        return DOCUMENT_DUPLICATE_ID;
    }
    if (!ensure_symbol_capacity(document, document->symbol_count + 1)) {
        return DOCUMENT_ALLOCATION_FAILED;
    }
    document->symbols[document->symbol_count++] = *symbol;
    return DOCUMENT_SUCCESS;
}

DocumentCode document_model_remove_symbol(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return DOCUMENT_INVALID_ARGUMENT; }
    for (size_t i = 0; i < document->symbol_count; i++) {
        if (document->symbols[i].id != id) { continue; }
        if (i + 1 < document->symbol_count) {
            memmove(&document->symbols[i], &document->symbols[i + 1],
                (document->symbol_count - i - 1) * sizeof *document->symbols);
        }
        document->symbol_count--;
        document->symbols[document->symbol_count] = (DocumentPlanSymbol){0};
        return DOCUMENT_SUCCESS;
    }
    return DOCUMENT_NOT_FOUND;
}


int document_plan_callout_is_locally_valid(const DocumentPlanCallout *callout)
{
    return callout != NULL && callout->id != DOMAIN_ID_INVALID &&
        callout->storey_id != DOMAIN_ID_INVALID && callout->text != NULL &&
        callout->text[0] != '\0' &&
        !(callout->target.x == callout->label_anchor.x &&
          callout->target.y == callout->label_anchor.y);
}

void document_plan_callout_destroy(DocumentPlanCallout *callout)
{
    if (callout == NULL) { return; }
    free(callout->text);
    *callout=(DocumentPlanCallout){0};
}

DocumentCode document_plan_callout_clone(const DocumentPlanCallout *source,
    DocumentPlanCallout *output)
{
    if (source == NULL || output == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_plan_callout_is_locally_valid(source)) { return DOCUMENT_INVALID_ANNOTATION; }
    size_t length=strlen(source->text);
    char *text=malloc(length+1);
    if (text == NULL) { return DOCUMENT_ALLOCATION_FAILED; }
    memcpy(text,source->text,length+1);
    *output=*source;
    output->text=text;
    return DOCUMENT_SUCCESS;
}

DocumentPlanCallout *document_model_find_callout_by_id(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i=0;i<document->callout_count;i++) {
        if (document->callouts[i].id == id) { return &document->callouts[i]; }
    }
    return NULL;
}

const DocumentPlanCallout *document_model_find_callout_by_id_const(
    const DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i=0;i<document->callout_count;i++) {
        if (document->callouts[i].id == id) { return &document->callouts[i]; }
    }
    return NULL;
}

static int ensure_callout_capacity(DocumentModel *document, size_t required)
{
    if (required <= document->callout_capacity) { return 1; }
    const size_t maximum=SIZE_MAX/sizeof *document->callouts;
    if (required > maximum) { return 0; }
    size_t capacity=document->callout_capacity == 0 ? 1 : document->callout_capacity;
    while (capacity < required) {
        if (capacity > maximum/2) { capacity=maximum; break; }
        capacity*=2;
    }
    DocumentPlanCallout *storage=realloc(document->callouts,capacity*sizeof *storage);
    if (storage == NULL) { return 0; }
    document->callouts=storage;
    document->callout_capacity=capacity;
    return 1;
}

DocumentCode document_model_insert_callout(DocumentModel *document,
    const DocumentPlanCallout *callout)
{
    if (document == NULL || callout == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_plan_callout_is_locally_valid(callout) ||
        document->callout_count > document->callout_capacity ||
        (document->callout_capacity == 0 && document->callouts != NULL) ||
        (document->callout_capacity != 0 && document->callouts == NULL)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document_model_find_callout_by_id_const(document,callout->id) != NULL) {
        return DOCUMENT_DUPLICATE_ID;
    }
    DocumentPlanCallout copy={0};
    DocumentCode code=document_plan_callout_clone(callout,&copy);
    if (code != DOCUMENT_SUCCESS) { return code; }
    if (!ensure_callout_capacity(document,document->callout_count+1)) {
        document_plan_callout_destroy(&copy);
        return DOCUMENT_ALLOCATION_FAILED;
    }
    document->callouts[document->callout_count++]=copy;
    return DOCUMENT_SUCCESS;
}

DocumentCode document_model_remove_callout(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return DOCUMENT_INVALID_ARGUMENT; }
    for (size_t i=0;i<document->callout_count;i++) {
        if (document->callouts[i].id != id) { continue; }
        document_plan_callout_destroy(&document->callouts[i]);
        if (i+1 < document->callout_count) {
            memmove(&document->callouts[i],&document->callouts[i+1],
                (document->callout_count-i-1)*sizeof *document->callouts);
        }
        document->callout_count--;
        document->callouts[document->callout_count]=(DocumentPlanCallout){0};
        return DOCUMENT_SUCCESS;
    }
    return DOCUMENT_NOT_FOUND;
}


int document_revision_is_locally_valid(const DocumentRevision *revision)
{
    return revision != NULL && revision->id != DOMAIN_ID_INVALID &&
        revision->identifier != NULL && revision->identifier[0] != '\0' &&
        revision->description != NULL;
}

void document_revision_destroy(DocumentRevision *revision)
{
    if (revision == NULL) { return; }
    free(revision->identifier);
    free(revision->description);
    *revision = (DocumentRevision){0};
}

DocumentCode document_revision_clone(const DocumentRevision *source,
    DocumentRevision *output)
{
    if (source == NULL || output == NULL || source == output) {
        return DOCUMENT_INVALID_ARGUMENT;
    }
    if (!document_revision_is_locally_valid(source)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    size_t identifier_length = strlen(source->identifier);
    size_t description_length = strlen(source->description);
    char *identifier = malloc(identifier_length + 1);
    char *description = malloc(description_length + 1);
    if (identifier == NULL || description == NULL) {
        free(identifier); free(description);
        return DOCUMENT_ALLOCATION_FAILED;
    }
    memcpy(identifier, source->identifier, identifier_length + 1);
    memcpy(description, source->description, description_length + 1);
    DocumentRevision candidate = *source;
    candidate.identifier = identifier;
    candidate.description = description;
    document_revision_destroy(output);
    *output = candidate;
    return DOCUMENT_SUCCESS;
}

DocumentRevision *document_model_find_revision_by_id(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->revision_count; i++) {
        if (document->revisions[i].id == id) { return &document->revisions[i]; }
    }
    return NULL;
}

const DocumentRevision *document_model_find_revision_by_id_const(
    const DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->revision_count; i++) {
        if (document->revisions[i].id == id) { return &document->revisions[i]; }
    }
    return NULL;
}

static int ensure_revision_capacity(DocumentModel *document, size_t required)
{
    if (required <= document->revision_capacity) { return 1; }
    const size_t maximum = SIZE_MAX / sizeof *document->revisions;
    if (required > maximum) { return 0; }
    size_t capacity = document->revision_capacity == 0 ? 1 : document->revision_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) { capacity = maximum; break; }
        capacity *= 2;
    }
    DocumentRevision *storage = realloc(document->revisions, capacity * sizeof *storage);
    if (storage == NULL) { return 0; }
    document->revisions = storage;
    document->revision_capacity = capacity;
    return 1;
}

DocumentCode document_model_insert_revision(DocumentModel *document,
    const DocumentRevision *revision)
{
    if (document == NULL || revision == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_revision_is_locally_valid(revision) ||
        document->revision_count > document->revision_capacity ||
        (document->revision_capacity == 0 && document->revisions != NULL) ||
        (document->revision_capacity != 0 && document->revisions == NULL)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document_model_find_revision_by_id_const(document, revision->id) != NULL) {
        return DOCUMENT_DUPLICATE_ID;
    }
    DocumentRevision copy = {0};
    DocumentCode code = document_revision_clone(revision, &copy);
    if (code != DOCUMENT_SUCCESS) { return code; }
    if (!ensure_revision_capacity(document, document->revision_count + 1)) {
        document_revision_destroy(&copy);
        return DOCUMENT_ALLOCATION_FAILED;
    }
    document->revisions[document->revision_count++] = copy;
    return DOCUMENT_SUCCESS;
}

DocumentCode document_model_remove_revision(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return DOCUMENT_INVALID_ARGUMENT; }
    for (size_t i = 0; i < document->revision_count; i++) {
        if (document->revisions[i].id != id) { continue; }
        document_revision_destroy(&document->revisions[i]);
        if (i + 1 < document->revision_count) {
            memmove(&document->revisions[i], &document->revisions[i + 1],
                (document->revision_count - i - 1) * sizeof *document->revisions);
        }
        document->revision_count--;
        document->revisions[document->revision_count] = (DocumentRevision){0};
        return DOCUMENT_SUCCESS;
    }
    return DOCUMENT_NOT_FOUND;
}


static int plan_positions_equal(PlanPosition a, PlanPosition b)
{
    return a.x == b.x && a.y == b.y;
}

int document_plan_revision_cloud_is_locally_valid(
    const DocumentPlanRevisionCloud *cloud)
{
    if (cloud == NULL || cloud->id == DOMAIN_ID_INVALID ||
        cloud->storey_id == DOMAIN_ID_INVALID || cloud->vertices == NULL ||
        cloud->vertex_count < 3 || cloud->vertex_count > SIZE_MAX / sizeof *cloud->vertices) {
        return 0;
    }
    /* Closure is implicit. Repeating the first point as the final stored point
     * would create a zero-length closing edge and two encodings of the same
     * authority. Adjacent duplicates are likewise rejected. */
    if (plan_positions_equal(cloud->vertices[0],
            cloud->vertices[cloud->vertex_count - 1])) {
        return 0;
    }
    for (size_t i = 1; i < cloud->vertex_count; i++) {
        if (plan_positions_equal(cloud->vertices[i - 1], cloud->vertices[i])) {
            return 0;
        }
    }
    /* Require at least three distinct authored positions. Self-intersection and
     * collinearity are allowed: revision markup is not a physical polygon and
     * must not inherit slab/topology validity rules. */
    size_t distinct = 1;
    for (size_t i = 1; i < cloud->vertex_count && distinct < 3; i++) {
        int seen = 0;
        for (size_t j = 0; j < i; j++) {
            if (plan_positions_equal(cloud->vertices[i], cloud->vertices[j])) {
                seen = 1;
                break;
            }
        }
        if (!seen) { distinct++; }
    }
    return distinct >= 3;
}

void document_plan_revision_cloud_destroy(DocumentPlanRevisionCloud *cloud)
{
    if (cloud == NULL) { return; }
    free(cloud->vertices);
    *cloud = (DocumentPlanRevisionCloud){0};
}

DocumentCode document_plan_revision_cloud_clone(
    const DocumentPlanRevisionCloud *source, DocumentPlanRevisionCloud *output)
{
    if (source == NULL || output == NULL || source == output) {
        return DOCUMENT_INVALID_ARGUMENT;
    }
    if (!document_plan_revision_cloud_is_locally_valid(source)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    PlanPosition *vertices = malloc(source->vertex_count * sizeof *vertices);
    if (vertices == NULL) { return DOCUMENT_ALLOCATION_FAILED; }
    memcpy(vertices, source->vertices, source->vertex_count * sizeof *vertices);
    DocumentPlanRevisionCloud candidate = *source;
    candidate.vertices = vertices;
    document_plan_revision_cloud_destroy(output);
    *output = candidate;
    return DOCUMENT_SUCCESS;
}

DocumentPlanRevisionCloud *document_model_find_revision_cloud_by_id(
    DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->revision_cloud_count; i++) {
        if (document->revision_clouds[i].id == id) {
            return &document->revision_clouds[i];
        }
    }
    return NULL;
}

const DocumentPlanRevisionCloud *document_model_find_revision_cloud_by_id_const(
    const DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < document->revision_cloud_count; i++) {
        if (document->revision_clouds[i].id == id) {
            return &document->revision_clouds[i];
        }
    }
    return NULL;
}

static int ensure_revision_cloud_capacity(DocumentModel *document, size_t required)
{
    if (required <= document->revision_cloud_capacity) { return 1; }
    const size_t maximum = SIZE_MAX / sizeof *document->revision_clouds;
    if (required > maximum) { return 0; }
    size_t capacity = document->revision_cloud_capacity == 0
        ? 1 : document->revision_cloud_capacity;
    while (capacity < required) {
        if (capacity > maximum / 2) { capacity = maximum; break; }
        capacity *= 2;
    }
    DocumentPlanRevisionCloud *storage =
        realloc(document->revision_clouds, capacity * sizeof *storage);
    if (storage == NULL) { return 0; }
    document->revision_clouds = storage;
    document->revision_cloud_capacity = capacity;
    return 1;
}

DocumentCode document_model_insert_revision_cloud(DocumentModel *document,
    const DocumentPlanRevisionCloud *cloud)
{
    if (document == NULL || cloud == NULL) { return DOCUMENT_INVALID_ARGUMENT; }
    if (!document_plan_revision_cloud_is_locally_valid(cloud) ||
        document->revision_cloud_count > document->revision_cloud_capacity ||
        (document->revision_cloud_capacity == 0 && document->revision_clouds != NULL) ||
        (document->revision_cloud_capacity != 0 && document->revision_clouds == NULL)) {
        return DOCUMENT_INVALID_ANNOTATION;
    }
    if (document_model_find_revision_cloud_by_id_const(document, cloud->id) != NULL) {
        return DOCUMENT_DUPLICATE_ID;
    }
    DocumentPlanRevisionCloud copy = {0};
    DocumentCode code = document_plan_revision_cloud_clone(cloud, &copy);
    if (code != DOCUMENT_SUCCESS) { return code; }
    if (!ensure_revision_cloud_capacity(document, document->revision_cloud_count + 1)) {
        document_plan_revision_cloud_destroy(&copy);
        return DOCUMENT_ALLOCATION_FAILED;
    }
    document->revision_clouds[document->revision_cloud_count++] = copy;
    return DOCUMENT_SUCCESS;
}

DocumentCode document_model_remove_revision_cloud(DocumentModel *document, DomainId id)
{
    if (document == NULL || id == DOMAIN_ID_INVALID) {
        return DOCUMENT_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < document->revision_cloud_count; i++) {
        if (document->revision_clouds[i].id != id) { continue; }
        document_plan_revision_cloud_destroy(&document->revision_clouds[i]);
        if (i + 1 < document->revision_cloud_count) {
            memmove(&document->revision_clouds[i], &document->revision_clouds[i + 1],
                (document->revision_cloud_count - i - 1) *
                    sizeof *document->revision_clouds);
        }
        document->revision_cloud_count--;
        document->revision_clouds[document->revision_cloud_count] =
            (DocumentPlanRevisionCloud){0};
        return DOCUMENT_SUCCESS;
    }
    return DOCUMENT_NOT_FOUND;
}
