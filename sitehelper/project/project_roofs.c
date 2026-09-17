#include "sitehelper_project.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "roof.h"

static int reserve_roofs(RoofCollection *collection, size_t count)
{
    if (count <= collection->capacity) { return 1; }
    if (count > SIZE_MAX / sizeof *collection->items) { return 0; }
    size_t maximum = SIZE_MAX / sizeof *collection->items;
    size_t grown = collection->capacity == 0 ? 1 : collection->capacity;
    while (grown < count) {
        if (grown > maximum / 2) { grown = maximum; break; }
        grown *= 2;
    }
    Roof *items = realloc(collection->items, grown * sizeof *items);
    if (items == NULL) { return 0; }
    collection->items = items;
    collection->capacity = grown;
    return 1;
}

static int roof_candidate_valid_and_regenerates(const Roof *roof)
{
    if (roof_validate(roof) != ROOF_SUCCESS) { return 0; }
    RoofPrototypeGeometry geometry = {0};
    RoofCode code = roof_build_derived_geometry(roof, &geometry);
    roof_prototype_geometry_destroy(&geometry);
    return code == ROOF_SUCCESS;
}

static int commit_candidate(Roof *target, Roof *candidate)
{
    if (target == NULL || candidate == NULL || !roof_candidate_valid_and_regenerates(candidate)) {
        return 0;
    }
    roof_destroy(target);
    *target = *candidate;
    *candidate = (Roof){0};
    return 1;
}

DomainId sitehelper_project_add_roof(SiteHelperProject *project, DomainId storey_id,
    const RoofPortionSpec *initial_portion, DomainId *portion_id)
{
    if (portion_id != NULL) { *portion_id = DOMAIN_ID_INVALID; }
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL || initial_portion == NULL) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId roof_id = domain_id_generate(&ids);
    DomainId first_portion_id = domain_id_generate(&ids);
    if (roof_id == DOMAIN_ID_INVALID || first_portion_id == DOMAIN_ID_INVALID ||
        ids.next == DOMAIN_ID_INVALID || sitehelper_project_contains_domain_id(project, roof_id) ||
        sitehelper_project_contains_domain_id(project, first_portion_id)) { return DOMAIN_ID_INVALID; }
    Roof candidate = {.id = roof_id};
    if (roof_definition_append_portion(&candidate.definition, first_portion_id, initial_portion) != ROOF_SUCCESS ||
        roof_candidate_valid_and_regenerates(&candidate) == 0 ||
        !reserve_roofs(&storey->roofs, storey->roofs.count + 1)) {
        roof_destroy(&candidate); return DOMAIN_ID_INVALID;
    }
    storey->roofs.items[storey->roofs.count++] = candidate;
    project->domain_ids = ids;
    if (portion_id != NULL) { *portion_id = first_portion_id; }
    return roof_id;
}

DomainId sitehelper_project_add_roof_portion_composed(SiteHelperProject *project, DomainId roof_id,
    const RoofPortionSpec *spec, DomainId existing_portion_id, RoofCompositionKind kind)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    if (roof == NULL || spec == NULL || roof_find_portion_by_id(roof, existing_portion_id) == NULL) {
        return DOMAIN_ID_INVALID;
    }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id)) { return DOMAIN_ID_INVALID; }
    Roof candidate = {0};
    if (roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_append_portion(&candidate.definition, id, spec) != ROOF_SUCCESS ||
        roof_definition_set_composition(&candidate.definition,
            (RoofComposition){existing_portion_id, id, kind}) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) {
        roof_destroy(&candidate); return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_restore_roof_portion_composed(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id, const RoofPortionSpec *spec, DomainId existing_portion_id,
    RoofCompositionKind kind)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    if (roof == NULL || spec == NULL || portion_id == DOMAIN_ID_INVALID ||
        project->domain_ids.next == DOMAIN_ID_INVALID || portion_id >= project->domain_ids.next ||
        sitehelper_project_contains_domain_id(project, portion_id) ||
        roof_find_portion_by_id(roof, existing_portion_id) == NULL) { return 0; }
    Roof candidate = {0};
    if (roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_append_portion(&candidate.definition, portion_id, spec) != ROOF_SUCCESS ||
        roof_definition_set_composition(&candidate.definition,
            (RoofComposition){existing_portion_id, portion_id, kind}) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) {
        roof_destroy(&candidate); return 0;
    }
    return 1;
}

int sitehelper_project_set_roof_portion(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id, const RoofPortionSpec *spec)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    Roof candidate = {0};
    if (roof == NULL || spec == NULL || roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_replace_portion(&candidate.definition, portion_id, spec) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) { roof_destroy(&candidate); return 0; }
    return 1;
}

int sitehelper_project_remove_roof_portion(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    Roof candidate = {0};
    if (roof == NULL || roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_remove_portion(&candidate.definition, portion_id) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) { roof_destroy(&candidate); return 0; }
    return 1;
}

int sitehelper_project_set_roof_composition(SiteHelperProject *project, DomainId roof_id,
    RoofComposition composition)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    Roof candidate = {0};
    if (roof == NULL || roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_set_composition(&candidate.definition, composition) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) { roof_destroy(&candidate); return 0; }
    return 1;
}

int sitehelper_project_remove_roof_composition(SiteHelperProject *project, DomainId roof_id,
    DomainId first_portion_id, DomainId second_portion_id)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    Roof candidate = {0};
    if (roof == NULL || roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_remove_composition(&candidate.definition, first_portion_id,
            second_portion_id) != ROOF_SUCCESS || !commit_candidate(roof, &candidate)) {
        roof_destroy(&candidate); return 0;
    }
    return 1;
}

int sitehelper_project_set_roof_termination(SiteHelperProject *project, DomainId roof_id,
    RoofTermination termination)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    Roof candidate = {0};
    if (roof == NULL || roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_set_termination(&candidate.definition, termination) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) { roof_destroy(&candidate); return 0; }
    return 1;
}

int sitehelper_project_remove_roof_termination(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id, RoofEnd end)
{
    Roof *roof = sitehelper_project_find_roof_by_id(project, roof_id);
    Roof candidate = {0};
    if (roof == NULL || roof_clone(roof, &candidate) != ROOF_SUCCESS ||
        roof_definition_remove_termination(&candidate.definition, portion_id, end) != ROOF_SUCCESS ||
        !commit_candidate(roof, &candidate)) { roof_destroy(&candidate); return 0; }
    return 1;
}

int sitehelper_project_replace_roof(SiteHelperProject *project, const Roof *replacement)
{
    if (project == NULL || replacement == NULL || replacement->id == DOMAIN_ID_INVALID ||
        project->domain_ids.next == DOMAIN_ID_INVALID) { return 0; }
    Roof *current = sitehelper_project_find_roof_by_id(project, replacement->id);
    if (current == NULL || replacement->id >= project->domain_ids.next ||
        !roof_candidate_valid_and_regenerates(replacement)) { return 0; }
    for (size_t i = 0; i < replacement->definition.portion_count; i++) {
        DomainId id = replacement->definition.portions[i].id;
        if (id == DOMAIN_ID_INVALID || id >= project->domain_ids.next) { return 0; }
        if (roof_find_portion_by_id(current, id) == NULL &&
            sitehelper_project_contains_domain_id(project, id)) { return 0; }
    }
    Roof candidate = {0};
    if (roof_clone(replacement, &candidate) != ROOF_SUCCESS) { return 0; }
    roof_destroy(current);
    *current = candidate;
    return 1;
}

int sitehelper_project_add_roof_composition(SiteHelperProject *project, DomainId roof_id,
    RoofComposition composition)
{
    return sitehelper_project_set_roof_composition(project, roof_id, composition);
}

int sitehelper_project_add_roof_termination(SiteHelperProject *project, DomainId roof_id,
    RoofTermination termination)
{
    return sitehelper_project_set_roof_termination(project, roof_id, termination);
}

Roof *sitehelper_project_find_roof_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Roof *roof = roof_collection_find_by_id(&project->storeys[i].roofs, id);
        if (roof != NULL) { return roof; }
    }
    return NULL;
}

const Roof *sitehelper_project_find_roof_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Roof *roof = roof_collection_find_by_id_const(&project->storeys[i].roofs, id);
        if (roof != NULL) { return roof; }
    }
    return NULL;
}

RoofPortionDefinition *sitehelper_project_find_roof_portion_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t s = 0; s < project->storey_count; s++) {
        for (size_t r = 0; r < project->storeys[s].roofs.count; r++) {
            RoofPortionDefinition *portion = roof_find_portion_by_id(&project->storeys[s].roofs.items[r], id);
            if (portion != NULL) { return portion; }
        }
    }
    return NULL;
}

const RoofPortionDefinition *sitehelper_project_find_roof_portion_by_id_const(
    const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t s = 0; s < project->storey_count; s++) {
        for (size_t r = 0; r < project->storeys[s].roofs.count; r++) {
            const RoofPortionDefinition *portion =
                roof_find_portion_by_id_const(&project->storeys[s].roofs.items[r], id);
            if (portion != NULL) { return portion; }
        }
    }
    return NULL;
}

int sitehelper_project_insert_roof_at(SiteHelperProject *project, DomainId storey_id,
    const Roof *roof, size_t index)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL || roof == NULL || index > storey->roofs.count ||
        roof_validate(roof) != ROOF_SUCCESS ||
        sitehelper_project_contains_domain_id(project, roof->id)) { return 0; }
    for (size_t i = 0; i < roof->definition.portion_count; i++) {
        if (sitehelper_project_contains_domain_id(project, roof->definition.portions[i].id)) {
            return 0;
        }
    }
    if (!roof_candidate_valid_and_regenerates(roof)) { return 0; }
    Roof candidate = {0};
    if (roof_clone(roof, &candidate) != ROOF_SUCCESS) { return 0; }
    if (!reserve_roofs(&storey->roofs, storey->roofs.count + 1)) {
        roof_destroy(&candidate);
        return 0;
    }
    if (index < storey->roofs.count) {
        memmove(&storey->roofs.items[index + 1], &storey->roofs.items[index],
            (storey->roofs.count - index) * sizeof *storey->roofs.items);
    }
    storey->roofs.items[index] = candidate;
    storey->roofs.count++;
    return 1;
}

int sitehelper_project_remove_roof_by_id(SiteHelperProject *project, DomainId id)
{
    Storey *storey = sitehelper_project_find_owning_storey(project, id);
    if (storey == NULL) { return 0; }
    for (size_t i = 0; i < storey->roofs.count; i++) {
        if (storey->roofs.items[i].id == id) {
            roof_destroy(&storey->roofs.items[i]);
            if (i + 1 < storey->roofs.count) {
                memmove(&storey->roofs.items[i], &storey->roofs.items[i + 1],
                    (storey->roofs.count - i - 1) * sizeof *storey->roofs.items);
            }
            storey->roofs.count--;
            return 1;
        }
    }
    return 0;
}
