#include <stdlib.h>
#include "sitehelper_project.h"

#include "wall.h"
#include "slab.h"
#include "roof.h"

void sitehelper_project_init(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return;
    }

    *project = (SiteHelperProject){0};

    project->settings = (BuildSettings){
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,

        .stud_spacing = 600,
        .nog_spacing = 1200,

        .opening_width_allowance = 0,
        .opening_height_allowance = 0,

        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    document_model_init(&project->document);

    domain_id_generator_init(
        &project->domain_ids
    );
}

void sitehelper_project_destroy(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return;
    }

    for (size_t i = 0; i < project->storey_count; i++) {
        build_destroy(&project->storeys[i].structure);
        slab_collection_destroy(&project->storeys[i].slabs);
        roof_collection_destroy(&project->storeys[i].roofs);
    }
    free(project->storeys);
    document_model_destroy(&project->document);

    *project = (SiteHelperProject){0};
}

DomainId sitehelper_project_add_room(SiteHelperProject *project, DomainId storey_id)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id) ||
        !build_add_room(&storey->structure, id)) { return DOMAIN_ID_INVALID; }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_set_room_location(SiteHelperProject *project,
    DomainId room_id, PlanPosition location)
{
    if (project == NULL) {
        return 0;
    }
    Room *room = sitehelper_project_find_room_by_id(project, room_id);
    if (room == NULL) {
        return 0;
    }
    room->location = location;
    room->has_location = true;
    return 1;
}

int sitehelper_project_clear_room_location(SiteHelperProject *project,
    DomainId room_id)
{
    if (project == NULL) {
        return 0;
    }
    Room *room = sitehelper_project_find_room_by_id(project, room_id);
    if (room == NULL) {
        return 0;
    }
    room->has_location = false;
    room->location = (PlanPosition){0};
    return 1;
}

DomainId sitehelper_project_add_wall(
    SiteHelperProject *project, DomainId storey_id,
    WallPlanSegment segment
)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL) {

        return DOMAIN_ID_INVALID;
    }

    DomainIdGenerator candidate_ids = project->domain_ids;
    DomainId wall_id = domain_id_generate(&candidate_ids);

    if (wall_id == DOMAIN_ID_INVALID || candidate_ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, wall_id)) {
        return DOMAIN_ID_INVALID;
    }

    Wall wall = { .id = wall_id };
    if (!wall_set_plan_segment(&wall, segment)) {
        return DOMAIN_ID_INVALID;
    }

    if (!build_append_wall(&storey->structure, &wall)) {

        return DOMAIN_ID_INVALID;
    }

    project->domain_ids = candidate_ids;

    return wall_id;
}

Storey *sitehelper_project_find_storey_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        if (project->storeys[i].id == id) { return &project->storeys[i]; }
    }
    return NULL;
}

const Storey *sitehelper_project_find_storey_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        if (project->storeys[i].id == id) { return &project->storeys[i]; }
    }
    return NULL;
}

Storey *sitehelper_project_find_owning_storey(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Storey *s = &project->storeys[i];
        if (s->id == id || build_contains_domain_id(&s->structure, id) ||
            slab_collection_find_by_id_const(&s->slabs, id) != NULL ||
            roof_collection_find_by_id_const(&s->roofs, id) != NULL) { return s; }
        for (size_t r = 0; r < s->roofs.count; r++) {
            if (roof_find_portion_by_id_const(&s->roofs.items[r], id) != NULL) { return s; }
        }
    }
    return NULL;
}

const Storey *sitehelper_project_find_owning_storey_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Storey *s = &project->storeys[i];
        if (s->id == id || build_contains_domain_id(&s->structure, id) ||
            slab_collection_find_by_id_const(&s->slabs, id) != NULL ||
            roof_collection_find_by_id_const(&s->roofs, id) != NULL) { return s; }
        for (size_t r = 0; r < s->roofs.count; r++) {
            if (roof_find_portion_by_id_const(&s->roofs.items[r], id) != NULL) { return s; }
        }
    }
    return NULL;
}

int sitehelper_project_contains_domain_id(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return 0; }
    return sitehelper_project_find_owning_storey_const(project, id) != NULL ||
        document_model_find_annotation_by_id_const(&project->document, id) != NULL ||
        document_model_find_dimension_by_id_const(&project->document, id) != NULL ||
        document_model_find_symbol_by_id_const(&project->document, id) != NULL ||
        document_model_find_callout_by_id_const(&project->document, id) != NULL ||
        document_model_find_revision_by_id_const(&project->document, id) != NULL ||
        document_model_find_revision_cloud_by_id_const(&project->document, id) != NULL;
}

int sitehelper_project_insert_storey(SiteHelperProject *project, DomainId id, int elevation_mm)
{
    if (project == NULL || id == DOMAIN_ID_INVALID ||
        project->storey_count > project->storey_capacity ||
        (project->storey_capacity != 0 && project->storeys == NULL) ||
        sitehelper_project_contains_domain_id(project, id)) { return 0; }
    if (project->storey_count == project->storey_capacity) {
        size_t maximum = SIZE_MAX / sizeof *project->storeys;
        size_t capacity = project->storey_capacity;
        if (capacity >= maximum) { return 0; }
        size_t grown = capacity == 0 ? 1 : capacity > maximum / 2 ? maximum : capacity * 2;
        Storey *storage = realloc(project->storeys, grown * sizeof *storage);
        if (storage == NULL) { return 0; }
        project->storeys = storage;
        project->storey_capacity = grown;
    }
    project->storeys[project->storey_count++] = (Storey){.id = id, .elevation_mm = elevation_mm};
    return 1;
}

DomainId sitehelper_project_add_storey(SiteHelperProject *project, int elevation_mm)
{
    if (project == NULL) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        !sitehelper_project_insert_storey(project, id, elevation_mm)) { return DOMAIN_ID_INVALID; }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_remove_wall_by_id(SiteHelperProject *project, DomainId id)
{
    Storey *storey = sitehelper_project_find_owning_storey(project, id);
    return storey != NULL && build_remove_wall_by_id(&storey->structure, id);
}

static int annotation_reference_valid(const SiteHelperProject *project,
    DomainId storey_id, DomainId target_id, int require_live_target)
{
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, storey_id);
    if (storey == NULL) { return 0; }
    if (target_id == DOMAIN_ID_INVALID) { return 1; }
    const Storey *owner = sitehelper_project_find_owning_storey_const(project, target_id);
    if (owner != NULL) { return owner == storey; }
    if (document_model_find_annotation_by_id_const(&project->document, target_id) != NULL ||
        document_model_find_dimension_by_id_const(&project->document, target_id) != NULL ||
        document_model_find_symbol_by_id_const(&project->document, target_id) != NULL ||
        document_model_find_callout_by_id_const(&project->document, target_id) != NULL ||
        document_model_find_revision_by_id_const(&project->document, target_id) != NULL ||
        document_model_find_revision_cloud_by_id_const(&project->document, target_id) != NULL) {
        return 0;
    }
    /* A persisted/restored note may retain a weak stable-ID association after
     * its physical target is deleted. Ordinary live creation still requires a
     * currently resolvable target. */
    return !require_live_target;
}


static int dimension_reference_valid(const SiteHelperProject *project, DomainId storey_id,
    DocumentDimensionReference reference, int require_live_target)
{
    const Storey *scope = sitehelper_project_find_storey_by_id_const(project, storey_id);
    if (scope == NULL || !document_dimension_reference_is_locally_valid(&reference)) { return 0; }
    if (reference.kind == DOCUMENT_DIMENSION_FIXED_POINT) { return 1; }
    const Storey *owner = sitehelper_project_find_owning_storey_const(project, reference.target_id);
    if (owner != NULL) {
        if (owner != scope) { return 0; }
        return sitehelper_project_find_wall_by_id_const(project, reference.target_id) != NULL;
    }
    if (document_model_find_annotation_by_id_const(&project->document, reference.target_id) != NULL ||
        document_model_find_dimension_by_id_const(&project->document, reference.target_id) != NULL ||
        document_model_find_symbol_by_id_const(&project->document, reference.target_id) != NULL ||
        document_model_find_callout_by_id_const(&project->document, reference.target_id) != NULL ||
        document_model_find_revision_by_id_const(&project->document, reference.target_id) != NULL ||
        document_model_find_revision_cloud_by_id_const(&project->document, reference.target_id) != NULL) {
        return 0;
    }
    return !require_live_target;
}

static int resolve_dimension_reference(const SiteHelperProject *project, DomainId storey_id,
    DocumentDimensionReference reference, PlanPosition *output)
{
    if (project == NULL || output == NULL ||
        !dimension_reference_valid(project, storey_id, reference, 0)) { return 0; }
    if (reference.kind == DOCUMENT_DIMENSION_FIXED_POINT) {
        *output = reference.position;
        return 1;
    }
    const Wall *wall = sitehelper_project_find_wall_by_id_const(project, reference.target_id);
    const Storey *owner = sitehelper_project_find_owning_storey_const(project, reference.target_id);
    if (wall == NULL || owner == NULL || owner->id != storey_id) { return 0; }
    *output = reference.kind == DOCUMENT_DIMENSION_WALL_START
        ? wall->definition.segment.start : wall->definition.segment.end;
    return 1;
}

DomainId sitehelper_project_add_plan_dimension(SiteHelperProject *project, DomainId storey_id,
    DocumentDimensionReference first, DocumentDimensionReference second, int offset_mm)
{
    if (project == NULL || !dimension_reference_valid(project, storey_id, first, 1) ||
        !dimension_reference_valid(project, storey_id, second, 1)) { return DOMAIN_ID_INVALID; }
    PlanPosition a, b;
    if (!resolve_dimension_reference(project, storey_id, first, &a) ||
        !resolve_dimension_reference(project, storey_id, second, &b) ||
        wall_plan_segment_length_mm((WallPlanSegment){a,b}) == 0) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id)) { return DOMAIN_ID_INVALID; }
    DocumentPlanDimension dimension = {id, storey_id, first, second, offset_mm};
    if (document_model_insert_dimension(&project->document, &dimension) != DOCUMENT_SUCCESS) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

DocumentPlanDimension *sitehelper_project_find_dimension_by_id(SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_dimension_by_id(&project->document, id);
}

const DocumentPlanDimension *sitehelper_project_find_dimension_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_dimension_by_id_const(&project->document, id);
}

int sitehelper_project_remove_dimension_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL && document_model_remove_dimension(&project->document, id) == DOCUMENT_SUCCESS;
}

int sitehelper_project_insert_dimension(SiteHelperProject *project,
    const DocumentPlanDimension *dimension)
{
    if (project == NULL || dimension == NULL || sitehelper_project_contains_domain_id(project, dimension->id) ||
        !dimension_reference_valid(project, dimension->storey_id, dimension->first, 0) ||
        !dimension_reference_valid(project, dimension->storey_id, dimension->second, 0)) { return 0; }
    return document_model_insert_dimension(&project->document, dimension) == DOCUMENT_SUCCESS;
}

int sitehelper_project_update_plan_dimension(SiteHelperProject *project, DomainId id,
    DomainId storey_id, DocumentDimensionReference first,
    DocumentDimensionReference second, int offset_mm)
{
    if (project == NULL || id == DOMAIN_ID_INVALID ||
        !dimension_reference_valid(project, storey_id, first, 1) ||
        !dimension_reference_valid(project, storey_id, second, 1)) { return 0; }
    DocumentPlanDimension *current = sitehelper_project_find_dimension_by_id(project, id);
    if (current == NULL) { return 0; }
    PlanPosition a, b;
    if (!resolve_dimension_reference(project, storey_id, first, &a) ||
        !resolve_dimension_reference(project, storey_id, second, &b) ||
        (a.x == b.x && a.y == b.y)) { return 0; }
    *current = (DocumentPlanDimension){id, storey_id, first, second, offset_mm};
    return 1;
}

int sitehelper_project_replace_dimension(SiteHelperProject *project,
    const DocumentPlanDimension *dimension)
{
    if (project == NULL || dimension == NULL) { return 0; }
    DocumentPlanDimension *current = sitehelper_project_find_dimension_by_id(project, dimension->id);
    if (current == NULL || !dimension_reference_valid(project, dimension->storey_id, dimension->first, 0) ||
        !dimension_reference_valid(project, dimension->storey_id, dimension->second, 0)) { return 0; }
    PlanPosition a, b;
    if (!resolve_dimension_reference(project, dimension->storey_id, dimension->first, &a) ||
        !resolve_dimension_reference(project, dimension->storey_id, dimension->second, &b) ||
        (a.x == b.x && a.y == b.y)) {
        /* Existing unresolved weak references may be restored/edited without becoming invalid. */
        if ((dimension->first.kind == DOCUMENT_DIMENSION_FIXED_POINT &&
             dimension->second.kind == DOCUMENT_DIMENSION_FIXED_POINT) ||
            current->storey_id != dimension->storey_id) { return 0; }
    }
    *current = *dimension;
    return 1;
}

int sitehelper_project_resolve_plan_dimension(const SiteHelperProject *project, DomainId id,
    PlanPosition *first, PlanPosition *second, int *distance_mm)
{
    if (project == NULL || first == NULL || second == NULL || distance_mm == NULL) { return 0; }
    const DocumentPlanDimension *dimension = sitehelper_project_find_dimension_by_id_const(project, id);
    if (dimension == NULL) { return 0; }
    PlanPosition a, b;
    if (!resolve_dimension_reference(project, dimension->storey_id, dimension->first, &a) ||
        !resolve_dimension_reference(project, dimension->storey_id, dimension->second, &b)) { return 0; }
    int distance = wall_plan_segment_length_mm((WallPlanSegment){a,b});
    if (distance == 0) { return 0; }
    *first = a; *second = b; *distance_mm = distance;
    return 1;
}

DomainId sitehelper_project_add_plan_symbol(SiteHelperProject *project, DomainId storey_id,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction)
{
    if (project == NULL || sitehelper_project_find_storey_by_id_const(project, storey_id) == NULL) {
        return DOMAIN_ID_INVALID;
    }
    DocumentPlanSymbol candidate = {.id = 1, .storey_id = storey_id, .kind = kind, .anchor = anchor,
        .direction = direction};
    if (!document_plan_symbol_is_locally_valid(&candidate)) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id)) { return DOMAIN_ID_INVALID; }
    candidate.id = id;
    if (document_model_insert_symbol(&project->document, &candidate) != DOCUMENT_SUCCESS) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

DocumentPlanSymbol *sitehelper_project_find_symbol_by_id(SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_symbol_by_id(&project->document, id);
}

const DocumentPlanSymbol *sitehelper_project_find_symbol_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_symbol_by_id_const(&project->document, id);
}

int sitehelper_project_remove_symbol_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL && document_model_remove_symbol(&project->document, id) == DOCUMENT_SUCCESS;
}

int sitehelper_project_insert_symbol(SiteHelperProject *project, const DocumentPlanSymbol *symbol)
{
    if (project == NULL || symbol == NULL || sitehelper_project_contains_domain_id(project, symbol->id) ||
        sitehelper_project_find_storey_by_id_const(project, symbol->storey_id) == NULL ||
        !document_plan_symbol_is_locally_valid(symbol)) { return 0; }
    return document_model_insert_symbol(&project->document, symbol) == DOCUMENT_SUCCESS;
}

int sitehelper_project_update_plan_symbol(SiteHelperProject *project, DomainId id,
    DomainId storey_id, DocumentPlanSymbolKind kind, PlanPosition anchor,
    DocumentPlanDirection direction)
{
    if (project == NULL || id == DOMAIN_ID_INVALID ||
        sitehelper_project_find_storey_by_id_const(project, storey_id) == NULL) { return 0; }
    DocumentPlanSymbol *current = sitehelper_project_find_symbol_by_id(project, id);
    DocumentPlanSymbol replacement = {.id=id,.storey_id=storey_id,.kind=kind,.anchor=anchor,
        .direction=direction};
    if (current == NULL || !document_plan_symbol_is_locally_valid(&replacement)) { return 0; }
    *current = replacement;
    return 1;
}

int sitehelper_project_replace_symbol(SiteHelperProject *project, const DocumentPlanSymbol *symbol)
{
    if (project == NULL || symbol == NULL ||
        sitehelper_project_find_storey_by_id_const(project, symbol->storey_id) == NULL ||
        !document_plan_symbol_is_locally_valid(symbol)) { return 0; }
    DocumentPlanSymbol *current = sitehelper_project_find_symbol_by_id(project, symbol->id);
    if (current == NULL) { return 0; }
    *current = *symbol;
    return 1;
}

DomainId sitehelper_project_add_plan_callout(SiteHelperProject *project, DomainId storey_id,
    PlanPosition target, PlanPosition label_anchor, const char *text)
{
    if (project == NULL || sitehelper_project_find_storey_by_id_const(project,storey_id) == NULL ||
        text == NULL || text[0] == '\0') { return DOMAIN_ID_INVALID; }
    DocumentPlanCallout candidate={.id=1,.storey_id=storey_id,.target=target,
        .label_anchor=label_anchor,.text=(char *)text};
    if (!document_plan_callout_is_locally_valid(&candidate)) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids=project->domain_ids;
    DomainId id=domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project,id)) { return DOMAIN_ID_INVALID; }
    candidate.id=id;
    if (document_model_insert_callout(&project->document,&candidate) != DOCUMENT_SUCCESS) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids=ids;
    return id;
}

DocumentPlanCallout *sitehelper_project_find_callout_by_id(SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_callout_by_id(&project->document,id);
}

const DocumentPlanCallout *sitehelper_project_find_callout_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_callout_by_id_const(&project->document,id);
}

int sitehelper_project_remove_callout_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL && document_model_remove_callout(&project->document,id) == DOCUMENT_SUCCESS;
}

int sitehelper_project_insert_callout(SiteHelperProject *project, const DocumentPlanCallout *callout)
{
    if (project == NULL || callout == NULL ||
        sitehelper_project_contains_domain_id(project,callout->id) ||
        sitehelper_project_find_storey_by_id_const(project,callout->storey_id) == NULL ||
        !document_plan_callout_is_locally_valid(callout)) { return 0; }
    return document_model_insert_callout(&project->document,callout) == DOCUMENT_SUCCESS;
}

int sitehelper_project_update_plan_callout(SiteHelperProject *project, DomainId id,
    DomainId storey_id, PlanPosition target, PlanPosition label_anchor, const char *text)
{
    if (project == NULL || id == DOMAIN_ID_INVALID || text == NULL || text[0] == '\0' ||
        sitehelper_project_find_storey_by_id_const(project,storey_id) == NULL) { return 0; }
    DocumentPlanCallout candidate={.id=id,.storey_id=storey_id,.target=target,
        .label_anchor=label_anchor,.text=(char *)text};
    if (!document_plan_callout_is_locally_valid(&candidate)) { return 0; }
    DocumentPlanCallout copy={0};
    if (document_plan_callout_clone(&candidate,&copy) != DOCUMENT_SUCCESS) { return 0; }
    DocumentPlanCallout *current=sitehelper_project_find_callout_by_id(project,id);
    if (current == NULL) { document_plan_callout_destroy(&copy); return 0; }
    document_plan_callout_destroy(current);
    *current=copy;
    return 1;
}

int sitehelper_project_replace_callout(SiteHelperProject *project,
    const DocumentPlanCallout *callout)
{
    if (project == NULL || callout == NULL ||
        sitehelper_project_find_storey_by_id_const(project,callout->storey_id) == NULL ||
        !document_plan_callout_is_locally_valid(callout)) { return 0; }
    DocumentPlanCallout copy={0};
    if (document_plan_callout_clone(callout,&copy) != DOCUMENT_SUCCESS) { return 0; }
    DocumentPlanCallout *current=sitehelper_project_find_callout_by_id(project,callout->id);
    if (current == NULL) { document_plan_callout_destroy(&copy); return 0; }
    document_plan_callout_destroy(current);
    *current=copy;
    return 1;
}


DomainId sitehelper_project_add_revision(SiteHelperProject *project,
    const char *identifier, const char *description)
{
    if (project == NULL || identifier == NULL || identifier[0] == '\0' ||
        description == NULL) {
        return DOMAIN_ID_INVALID;
    }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id)) {
        return DOMAIN_ID_INVALID;
    }
    DocumentRevision revision = {
        .id = id, .identifier = (char *)identifier, .description = (char *)description
    };
    if (document_model_insert_revision(&project->document, &revision) != DOCUMENT_SUCCESS) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

DocumentRevision *sitehelper_project_find_revision_by_id(
    SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL : document_model_find_revision_by_id(&project->document, id);
}

const DocumentRevision *sitehelper_project_find_revision_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL :
        document_model_find_revision_by_id_const(&project->document, id);
}

int sitehelper_project_remove_revision_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL &&
        document_model_remove_revision(&project->document, id) == DOCUMENT_SUCCESS;
}

int sitehelper_project_insert_revision(SiteHelperProject *project,
    const DocumentRevision *revision)
{
    if (project == NULL || revision == NULL ||
        sitehelper_project_contains_domain_id(project, revision->id) ||
        !document_revision_is_locally_valid(revision)) {
        return 0;
    }
    return document_model_insert_revision(&project->document, revision) == DOCUMENT_SUCCESS;
}

int sitehelper_project_update_revision(SiteHelperProject *project, DomainId id,
    const char *identifier, const char *description)
{
    if (project == NULL || id == DOMAIN_ID_INVALID || identifier == NULL ||
        identifier[0] == '\0' || description == NULL) {
        return 0;
    }
    DocumentRevision candidate = {
        .id = id, .identifier = (char *)identifier, .description = (char *)description
    };
    DocumentRevision copy = {0};
    if (document_revision_clone(&candidate, &copy) != DOCUMENT_SUCCESS) { return 0; }
    DocumentRevision *current = sitehelper_project_find_revision_by_id(project, id);
    if (current == NULL) { document_revision_destroy(&copy); return 0; }
    document_revision_destroy(current);
    *current = copy;
    return 1;
}

int sitehelper_project_replace_revision(SiteHelperProject *project,
    const DocumentRevision *revision)
{
    if (project == NULL || revision == NULL || !document_revision_is_locally_valid(revision)) {
        return 0;
    }
    DocumentRevision copy = {0};
    if (document_revision_clone(revision, &copy) != DOCUMENT_SUCCESS) { return 0; }
    DocumentRevision *current = sitehelper_project_find_revision_by_id(project, revision->id);
    if (current == NULL) { document_revision_destroy(&copy); return 0; }
    document_revision_destroy(current);
    *current = copy;
    return 1;
}


DomainId sitehelper_project_add_plan_revision_cloud(SiteHelperProject *project,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count)
{
    if (project == NULL || vertices == NULL ||
        sitehelper_project_find_storey_by_id_const(project, storey_id) == NULL) {
        return DOMAIN_ID_INVALID;
    }
    DocumentPlanRevisionCloud candidate = {
        .id = 1, .storey_id = storey_id,
        .vertices = (PlanPosition *)vertices, .vertex_count = vertex_count
    };
    if (!document_plan_revision_cloud_is_locally_valid(&candidate)) {
        return DOMAIN_ID_INVALID;
    }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id)) {
        return DOMAIN_ID_INVALID;
    }
    candidate.id = id;
    if (document_model_insert_revision_cloud(&project->document, &candidate) !=
        DOCUMENT_SUCCESS) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

DocumentPlanRevisionCloud *sitehelper_project_find_revision_cloud_by_id(
    SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL :
        document_model_find_revision_cloud_by_id(&project->document, id);
}

const DocumentPlanRevisionCloud *sitehelper_project_find_revision_cloud_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL :
        document_model_find_revision_cloud_by_id_const(&project->document, id);
}

int sitehelper_project_remove_revision_cloud_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL &&
        document_model_remove_revision_cloud(&project->document, id) == DOCUMENT_SUCCESS;
}

int sitehelper_project_insert_revision_cloud(SiteHelperProject *project,
    const DocumentPlanRevisionCloud *cloud)
{
    if (project == NULL || cloud == NULL ||
        sitehelper_project_contains_domain_id(project, cloud->id) ||
        sitehelper_project_find_storey_by_id_const(project, cloud->storey_id) == NULL ||
        !document_plan_revision_cloud_is_locally_valid(cloud)) {
        return 0;
    }
    if (cloud->revision_id != DOMAIN_ID_INVALID &&
        sitehelper_project_contains_domain_id(project, cloud->revision_id) &&
        sitehelper_project_find_revision_by_id_const(project, cloud->revision_id) == NULL) {
        return 0;
    }
    return document_model_insert_revision_cloud(&project->document, cloud) ==
        DOCUMENT_SUCCESS;
}

int sitehelper_project_update_plan_revision_cloud(SiteHelperProject *project, DomainId id,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count)
{
    if (project == NULL || id == DOMAIN_ID_INVALID || vertices == NULL ||
        sitehelper_project_find_storey_by_id_const(project, storey_id) == NULL) {
        return 0;
    }
    DocumentPlanRevisionCloud candidate = {
        .id = id, .storey_id = storey_id,
        .revision_id = DOMAIN_ID_INVALID,
        .vertices = (PlanPosition *)vertices, .vertex_count = vertex_count
    };
    DocumentPlanRevisionCloud *existing =
        sitehelper_project_find_revision_cloud_by_id(project, id);
    if (existing == NULL) { return 0; }
    candidate.revision_id = existing->revision_id;
    if (!document_plan_revision_cloud_is_locally_valid(&candidate)) { return 0; }
    DocumentPlanRevisionCloud copy = {0};
    if (document_plan_revision_cloud_clone(&candidate, &copy) != DOCUMENT_SUCCESS) {
        return 0;
    }
    DocumentPlanRevisionCloud *current =
        sitehelper_project_find_revision_cloud_by_id(project, id);
    if (current == NULL) {
        document_plan_revision_cloud_destroy(&copy);
        return 0;
    }
    document_plan_revision_cloud_destroy(current);
    *current = copy;
    return 1;
}

int sitehelper_project_replace_revision_cloud(SiteHelperProject *project,
    const DocumentPlanRevisionCloud *cloud)
{
    if (project == NULL || cloud == NULL ||
        sitehelper_project_find_storey_by_id_const(project, cloud->storey_id) == NULL ||
        !document_plan_revision_cloud_is_locally_valid(cloud)) {
        return 0;
    }
    if (cloud->revision_id != DOMAIN_ID_INVALID &&
        sitehelper_project_contains_domain_id(project, cloud->revision_id) &&
        sitehelper_project_find_revision_by_id_const(project, cloud->revision_id) == NULL) {
        return 0;
    }
    DocumentPlanRevisionCloud copy = {0};
    if (document_plan_revision_cloud_clone(cloud, &copy) != DOCUMENT_SUCCESS) { return 0; }
    DocumentPlanRevisionCloud *current =
        sitehelper_project_find_revision_cloud_by_id(project, cloud->id);
    if (current == NULL) {
        document_plan_revision_cloud_destroy(&copy);
        return 0;
    }
    document_plan_revision_cloud_destroy(current);
    *current = copy;
    return 1;
}


int sitehelper_project_set_revision_cloud_revision(SiteHelperProject *project,
    DomainId revision_cloud_id, DomainId revision_id)
{
    if (project == NULL || revision_cloud_id == DOMAIN_ID_INVALID) { return 0; }
    if (revision_id != DOMAIN_ID_INVALID &&
        sitehelper_project_find_revision_by_id_const(project, revision_id) == NULL) {
        return 0;
    }
    DocumentPlanRevisionCloud *cloud =
        sitehelper_project_find_revision_cloud_by_id(project, revision_cloud_id);
    if (cloud == NULL) { return 0; }
    cloud->revision_id = revision_id;
    return 1;
}

DomainId sitehelper_project_add_plan_note(SiteHelperProject *project, DomainId storey_id,
    PlanPosition position, DomainId target_id, const char *text)
{
    if (project == NULL || text == NULL || text[0] == '\0' ||
        !annotation_reference_valid(project, storey_id, target_id, 1)) {
        return DOMAIN_ID_INVALID;
    }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id)) {
        return DOMAIN_ID_INVALID;
    }
    DocumentAnnotation annotation = {
        .id = id,
        .kind = DOCUMENT_ANNOTATION_NOTE,
        .anchor = {.storey_id = storey_id, .position = position},
        .target_id = target_id,
        .text = (char *)text
    };
    if (document_model_insert_annotation(&project->document, &annotation) != DOCUMENT_SUCCESS) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

DocumentAnnotation *sitehelper_project_find_annotation_by_id(
    SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL :
        document_model_find_annotation_by_id(&project->document, id);
}

const DocumentAnnotation *sitehelper_project_find_annotation_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    return project == NULL ? NULL :
        document_model_find_annotation_by_id_const(&project->document, id);
}

int sitehelper_project_remove_annotation_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL &&
        document_model_remove_annotation(&project->document, id) == DOCUMENT_SUCCESS;
}

int sitehelper_project_update_plan_note(SiteHelperProject *project, DomainId id,
    DomainId storey_id, PlanPosition position, DomainId target_id, const char *text)
{
    if (project == NULL || id == DOMAIN_ID_INVALID || text == NULL || text[0] == '\0') {
        return 0;
    }
    DocumentAnnotation *existing = sitehelper_project_find_annotation_by_id(project, id);
    if (existing == NULL || existing->kind != DOCUMENT_ANNOTATION_NOTE) { return 0; }
    int require_live_target = target_id != existing->target_id ||
        storey_id != existing->anchor.storey_id;
    if (!annotation_reference_valid(project, storey_id, target_id, require_live_target)) {
        return 0;
    }
    DocumentAnnotation replacement = {
        .id = id, .kind = DOCUMENT_ANNOTATION_NOTE,
        .anchor = {.storey_id = storey_id, .position = position},
        .target_id = target_id, .text = (char *)text
    };
    DocumentAnnotation owned = {0};
    if (document_annotation_clone(&replacement, &owned) != DOCUMENT_SUCCESS) { return 0; }
    document_annotation_destroy(existing);
    *existing = owned;
    return 1;
}

int sitehelper_project_insert_annotation(SiteHelperProject *project,
    const DocumentAnnotation *annotation)
{
    if (project == NULL || annotation == NULL ||
        sitehelper_project_contains_domain_id(project, annotation->id) ||
        !annotation_reference_valid(project, annotation->anchor.storey_id,
            annotation->target_id, 0)) {
        return 0;
    }
    return document_model_insert_annotation(&project->document, annotation) == DOCUMENT_SUCCESS;
}

int sitehelper_project_replace_annotation(SiteHelperProject *project,
    const DocumentAnnotation *annotation)
{
    if (project == NULL || annotation == NULL ||
        !document_annotation_is_locally_valid(annotation) ||
        !annotation_reference_valid(project, annotation->anchor.storey_id,
            annotation->target_id, 0)) { return 0; }
    DocumentAnnotation *existing = sitehelper_project_find_annotation_by_id(project, annotation->id);
    if (existing == NULL || existing->kind != annotation->kind) { return 0; }
    DocumentAnnotation owned = {0};
    if (document_annotation_clone(annotation, &owned) != DOCUMENT_SUCCESS) { return 0; }
    document_annotation_destroy(existing);
    *existing = owned;
    return 1;
}

Room *sitehelper_project_find_room_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Room *found = build_find_room_by_id(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

const Room *sitehelper_project_find_room_by_id_const(const SiteHelperProject *project, DomainId id)
{
    return sitehelper_project_find_room_with_owner_const(project, id, NULL);
}

const Room *sitehelper_project_find_room_with_owner_const(const SiteHelperProject *project,
    DomainId id, const Storey **owner)
{
    if (owner != NULL) { *owner = NULL; }
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Room *found = build_find_room_by_id_const(&project->storeys[i].structure, id);
        if (found != NULL) {
            if (owner != NULL) { *owner = &project->storeys[i]; }
            return found;
        }
    }
    return NULL;
}

Wall *sitehelper_project_find_wall_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Wall *found = build_find_wall_by_id(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

const Wall *sitehelper_project_find_wall_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Wall *found = build_find_wall_by_id_const(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

RoomSeparator *sitehelper_project_find_room_separator_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        RoomSeparator *found = build_find_room_separator_by_id(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

const RoomSeparator *sitehelper_project_find_room_separator_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const RoomSeparator *found = build_find_room_separator_by_id_const(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}
