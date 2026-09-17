#include "roof.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "roof_compound_prototype.h"

static int metadata_valid(size_t count, size_t capacity, const void *items, size_t item_size)
{
    return count <= capacity && (capacity == 0 ? items == NULL : items != NULL) &&
        (item_size == 0 || capacity <= SIZE_MAX / item_size);
}

static int reserve(void **items, size_t *capacity, size_t count, size_t item_size)
{
    if (count <= *capacity) { return 1; }
    if (count > SIZE_MAX / item_size) { return 0; }
    size_t maximum = SIZE_MAX / item_size;
    size_t grown = *capacity == 0 ? 1 : *capacity;
    while (grown < count) {
        if (grown > maximum / 2) { grown = maximum; break; }
        grown *= 2;
    }
    void *storage = realloc(*items, grown * item_size);
    if (storage == NULL) { return 0; }
    *items = storage;
    *capacity = grown;
    return 1;
}

static int portion_index(const RoofDefinition *definition, DomainId id, size_t *index);

static void portion_destroy(RoofPortionDefinition *portion)
{
    if (portion == NULL) { return; }
    free(portion->support_vertices);
    *portion = (RoofPortionDefinition){0};
}

void roof_destroy(Roof *roof)
{
    if (roof == NULL) { return; }
    for (size_t i = 0; i < roof->definition.portion_count; i++) {
        portion_destroy(&roof->definition.portions[i]);
    }
    free(roof->definition.portions);
    free(roof->definition.compositions);
    free(roof->definition.terminations);
    *roof = (Roof){0};
}

void roof_collection_destroy(RoofCollection *collection)
{
    if (collection == NULL) { return; }
    for (size_t i = 0; i < collection->count; i++) { roof_destroy(&collection->items[i]); }
    free(collection->items);
    *collection = (RoofCollection){0};
}

Roof *roof_collection_find_by_id(RoofCollection *collection, DomainId id)
{
    if (collection == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < collection->count; i++) {
        if (collection->items[i].id == id) { return &collection->items[i]; }
    }
    return NULL;
}

const Roof *roof_collection_find_by_id_const(const RoofCollection *collection, DomainId id)
{
    if (collection == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < collection->count; i++) {
        if (collection->items[i].id == id) { return &collection->items[i]; }
    }
    return NULL;
}

RoofPortionDefinition *roof_find_portion_by_id(Roof *roof, DomainId id)
{
    if (roof == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < roof->definition.portion_count; i++) {
        if (roof->definition.portions[i].id == id) { return &roof->definition.portions[i]; }
    }
    return NULL;
}

const RoofPortionDefinition *roof_find_portion_by_id_const(const Roof *roof, DomainId id)
{
    if (roof == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < roof->definition.portion_count; i++) {
        if (roof->definition.portions[i].id == id) { return &roof->definition.portions[i]; }
    }
    return NULL;
}

RoofCode roof_definition_append_portion(RoofDefinition *definition, DomainId id,
    const RoofPortionSpec *spec)
{
    if (definition == NULL || spec == NULL || id == DOMAIN_ID_INVALID ||
        spec->support_vertices == NULL || spec->support_vertex_count == 0 ||
        roof_find_portion_by_id_const(&(Roof){.definition = *definition}, id) != NULL) {
        return ROOF_INVALID_ARGUMENT;
    }
    if (spec->support_vertex_count > SIZE_MAX / sizeof(PlanPosition)) { return ROOF_ALLOCATION_FAILED; }
    PlanPosition *vertices = malloc(spec->support_vertex_count * sizeof *vertices);
    if (vertices == NULL) { return ROOF_ALLOCATION_FAILED; }
    memcpy(vertices, spec->support_vertices, spec->support_vertex_count * sizeof *vertices);
    if (!reserve((void **)&definition->portions, &definition->portion_capacity,
            definition->portion_count + 1, sizeof *definition->portions)) {
        free(vertices); return ROOF_ALLOCATION_FAILED;
    }
    RoofPortionDefinition portion = {
        .id = id,
        .support_vertices = vertices,
        .support_vertex_count = spec->support_vertex_count,
        .support_vertex_capacity = spec->support_vertex_count,
        .generation = spec->generation,
        .slope_ppm = spec->slope_ppm,
        .reference_z_mm = spec->reference_z_mm,
        .direction = spec->direction,
        .single_slope_reference = spec->single_slope_reference
    };
    definition->portions[definition->portion_count++] = portion;
    return ROOF_SUCCESS;
}

RoofCode roof_definition_append_composition(RoofDefinition *definition, RoofComposition composition)
{
    if (definition == NULL || composition.first_portion_id == DOMAIN_ID_INVALID ||
        composition.second_portion_id == DOMAIN_ID_INVALID ||
        composition.first_portion_id == composition.second_portion_id) { return ROOF_INVALID_ARGUMENT; }
    if (!reserve((void **)&definition->compositions, &definition->composition_capacity,
            definition->composition_count + 1, sizeof *definition->compositions)) {
        return ROOF_ALLOCATION_FAILED;
    }
    definition->compositions[definition->composition_count++] = composition;
    return ROOF_SUCCESS;
}

RoofCode roof_definition_append_termination(RoofDefinition *definition, RoofTermination termination)
{
    if (definition == NULL || termination.portion_id == DOMAIN_ID_INVALID ||
        termination.termination_offset_mm <= 0) { return ROOF_INVALID_ARGUMENT; }
    if (!reserve((void **)&definition->terminations, &definition->termination_capacity,
            definition->termination_count + 1, sizeof *definition->terminations)) {
        return ROOF_ALLOCATION_FAILED;
    }
    definition->terminations[definition->termination_count++] = termination;
    return ROOF_SUCCESS;
}

static int composition_pair_matches(const RoofComposition *composition,
    DomainId first_portion_id, DomainId second_portion_id)
{
    return composition != NULL &&
        ((composition->first_portion_id == first_portion_id &&
          composition->second_portion_id == second_portion_id) ||
         (composition->first_portion_id == second_portion_id &&
          composition->second_portion_id == first_portion_id));
}

RoofCode roof_definition_replace_portion(RoofDefinition *definition, DomainId portion_id,
    const RoofPortionSpec *spec)
{
    if (definition == NULL || spec == NULL || portion_id == DOMAIN_ID_INVALID ||
        spec->support_vertices == NULL || spec->support_vertex_count == 0 ||
        spec->support_vertex_count > SIZE_MAX / sizeof *spec->support_vertices) {
        return ROOF_INVALID_ARGUMENT;
    }
    size_t index;
    if (!portion_index(definition, portion_id, &index)) { return ROOF_INVALID_ID; }
    PlanPosition *vertices = malloc(spec->support_vertex_count * sizeof *vertices);
    if (vertices == NULL) { return ROOF_ALLOCATION_FAILED; }
    memcpy(vertices, spec->support_vertices, spec->support_vertex_count * sizeof *vertices);
    RoofPortionDefinition replacement = {
        .id = portion_id,
        .support_vertices = vertices,
        .support_vertex_count = spec->support_vertex_count,
        .support_vertex_capacity = spec->support_vertex_count,
        .generation = spec->generation,
        .slope_ppm = spec->slope_ppm,
        .reference_z_mm = spec->reference_z_mm,
        .direction = spec->direction,
        .single_slope_reference = spec->single_slope_reference
    };
    portion_destroy(&definition->portions[index]);
    definition->portions[index] = replacement;
    return ROOF_SUCCESS;
}

RoofCode roof_definition_remove_portion(RoofDefinition *definition, DomainId portion_id)
{
    if (definition == NULL || portion_id == DOMAIN_ID_INVALID) { return ROOF_INVALID_ARGUMENT; }
    size_t index;
    if (!portion_index(definition, portion_id, &index)) { return ROOF_INVALID_ID; }
    if (definition->portion_count <= 1) { return ROOF_INVALID_PORTION; }
    portion_destroy(&definition->portions[index]);
    if (index + 1 < definition->portion_count) {
        memmove(&definition->portions[index], &definition->portions[index + 1],
            (definition->portion_count - index - 1) * sizeof *definition->portions);
    }
    definition->portion_count--;

    size_t write = 0;
    for (size_t i = 0; i < definition->composition_count; i++) {
        RoofComposition composition = definition->compositions[i];
        if (composition.first_portion_id == portion_id ||
            composition.second_portion_id == portion_id) { continue; }
        definition->compositions[write++] = composition;
    }
    definition->composition_count = write;
    write = 0;
    for (size_t i = 0; i < definition->termination_count; i++) {
        RoofTermination termination = definition->terminations[i];
        if (termination.portion_id == portion_id) { continue; }
        definition->terminations[write++] = termination;
    }
    definition->termination_count = write;
    return ROOF_SUCCESS;
}

RoofCode roof_definition_set_composition(RoofDefinition *definition, RoofComposition composition)
{
    if (definition == NULL || composition.first_portion_id == DOMAIN_ID_INVALID ||
        composition.second_portion_id == DOMAIN_ID_INVALID ||
        composition.first_portion_id == composition.second_portion_id) {
        return ROOF_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < definition->composition_count; i++) {
        if (composition_pair_matches(&definition->compositions[i], composition.first_portion_id,
                composition.second_portion_id)) {
            definition->compositions[i] = composition;
            return ROOF_SUCCESS;
        }
    }
    return roof_definition_append_composition(definition, composition);
}

RoofCode roof_definition_remove_composition(RoofDefinition *definition, DomainId first_portion_id,
    DomainId second_portion_id)
{
    if (definition == NULL || first_portion_id == DOMAIN_ID_INVALID ||
        second_portion_id == DOMAIN_ID_INVALID || first_portion_id == second_portion_id) {
        return ROOF_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < definition->composition_count; i++) {
        if (!composition_pair_matches(&definition->compositions[i], first_portion_id,
                second_portion_id)) { continue; }
        if (i + 1 < definition->composition_count) {
            memmove(&definition->compositions[i], &definition->compositions[i + 1],
                (definition->composition_count - i - 1) * sizeof *definition->compositions);
        }
        definition->composition_count--;
        return ROOF_SUCCESS;
    }
    return ROOF_INVALID_COMPOSITION;
}

RoofCode roof_definition_set_termination(RoofDefinition *definition, RoofTermination termination)
{
    if (definition == NULL || termination.portion_id == DOMAIN_ID_INVALID ||
        termination.termination_offset_mm <= 0) { return ROOF_INVALID_ARGUMENT; }
    for (size_t i = 0; i < definition->termination_count; i++) {
        RoofTermination *current = &definition->terminations[i];
        if (current->portion_id == termination.portion_id && current->end == termination.end) {
            *current = termination;
            return ROOF_SUCCESS;
        }
    }
    return roof_definition_append_termination(definition, termination);
}

RoofCode roof_definition_remove_termination(RoofDefinition *definition, DomainId portion_id,
    RoofEnd end)
{
    if (definition == NULL || portion_id == DOMAIN_ID_INVALID ||
        (end != ROOF_END_NEGATIVE_AXIS && end != ROOF_END_POSITIVE_AXIS)) {
        return ROOF_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < definition->termination_count; i++) {
        if (definition->terminations[i].portion_id != portion_id ||
            definition->terminations[i].end != end) { continue; }
        if (i + 1 < definition->termination_count) {
            memmove(&definition->terminations[i], &definition->terminations[i + 1],
                (definition->termination_count - i - 1) * sizeof *definition->terminations);
        }
        definition->termination_count--;
        return ROOF_SUCCESS;
    }
    return ROOF_INVALID_TERMINATION;
}

int roof_authority_equal(const Roof *a, const Roof *b)
{
    if (a == NULL || b == NULL || a->id != b->id ||
        a->definition.portion_count != b->definition.portion_count ||
        a->definition.composition_count != b->definition.composition_count ||
        a->definition.termination_count != b->definition.termination_count) { return 0; }
    for (size_t i = 0; i < a->definition.portion_count; i++) {
        const RoofPortionDefinition *x = &a->definition.portions[i];
        const RoofPortionDefinition *y = &b->definition.portions[i];
        if (x->id != y->id || x->support_vertex_count != y->support_vertex_count ||
            x->support_vertex_count > SIZE_MAX / sizeof *x->support_vertices ||
            x->generation != y->generation || x->slope_ppm != y->slope_ppm ||
            x->reference_z_mm != y->reference_z_mm || x->direction.x != y->direction.x ||
            x->direction.y != y->direction.y ||
            x->single_slope_reference != y->single_slope_reference ||
            x->support_vertices == NULL || y->support_vertices == NULL ||
            memcmp(x->support_vertices, y->support_vertices,
                x->support_vertex_count * sizeof *x->support_vertices) != 0) { return 0; }
    }
    for (size_t i = 0; i < a->definition.composition_count; i++) {
        const RoofComposition *x = &a->definition.compositions[i];
        const RoofComposition *y = &b->definition.compositions[i];
        if (x->first_portion_id != y->first_portion_id ||
            x->second_portion_id != y->second_portion_id || x->kind != y->kind) { return 0; }
    }
    for (size_t i = 0; i < a->definition.termination_count; i++) {
        const RoofTermination *x = &a->definition.terminations[i];
        const RoofTermination *y = &b->definition.terminations[i];
        if (x->portion_id != y->portion_id || x->end != y->end ||
            x->termination_offset_mm != y->termination_offset_mm) { return 0; }
    }
    return 1;
}

RoofCode roof_clone(const Roof *source, Roof *output)
{
    if (source == NULL || output == NULL || source == output) { return ROOF_INVALID_ARGUMENT; }
    Roof candidate = {.id = source->id};
    for (size_t i = 0; i < source->definition.portion_count; i++) {
        const RoofPortionDefinition *p = &source->definition.portions[i];
        RoofPortionSpec spec = {p->support_vertices, p->support_vertex_count, p->generation,
            p->slope_ppm, p->reference_z_mm, p->direction, p->single_slope_reference};
        RoofCode code = roof_definition_append_portion(&candidate.definition, p->id, &spec);
        if (code != ROOF_SUCCESS) { roof_destroy(&candidate); return code; }
    }
    for (size_t i = 0; i < source->definition.composition_count; i++) {
        RoofCode code = roof_definition_append_composition(&candidate.definition,
            source->definition.compositions[i]);
        if (code != ROOF_SUCCESS) { roof_destroy(&candidate); return code; }
    }
    for (size_t i = 0; i < source->definition.termination_count; i++) {
        RoofCode code = roof_definition_append_termination(&candidate.definition,
            source->definition.terminations[i]);
        if (code != ROOF_SUCCESS) { roof_destroy(&candidate); return code; }
    }
    roof_destroy(output);
    *output = candidate;
    return ROOF_SUCCESS;
}

static int portion_index(const RoofDefinition *definition, DomainId id, size_t *index)
{
    for (size_t i = 0; i < definition->portion_count; i++) {
        if (definition->portions[i].id == id) { if (index) *index = i; return 1; }
    }
    return 0;
}

static RoofPrototypeGeneration prototype_generation(RoofPortionGeneration generation)
{
    return generation == ROOF_PORTION_OPPOSING_SLOPES ? ROOF_PROTOTYPE_OPPOSING_SLOPES :
        generation == ROOF_PORTION_ALL_BOUNDARY_SLOPES ? ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES :
        ROOF_PROTOTYPE_SINGLE_SLOPE;
}

RoofCode roof_build_derived_geometry(const Roof *roof, RoofPrototypeGeometry *output)
{
    if (roof == NULL || output == NULL || roof->id == DOMAIN_ID_INVALID ||
        roof->definition.portion_count == 0) { return ROOF_INVALID_ARGUMENT; }
    const RoofDefinition *d = &roof->definition;
    RoofPrototypeIntent *portions = calloc(d->portion_count, sizeof *portions);
    RoofPrototypeComposition *compositions = d->composition_count ?
        calloc(d->composition_count, sizeof *compositions) : NULL;
    RoofPrototypeTermination *terminations = d->termination_count ?
        calloc(d->termination_count, sizeof *terminations) : NULL;
    if (portions == NULL || (d->composition_count && compositions == NULL) ||
        (d->termination_count && terminations == NULL)) {
        free(portions); free(compositions); free(terminations); return ROOF_ALLOCATION_FAILED;
    }
    for (size_t i = 0; i < d->portion_count; i++) {
        const RoofPortionDefinition *p = &d->portions[i];
        portions[i] = (RoofPrototypeIntent){p->support_vertices, p->support_vertex_count,
            prototype_generation(p->generation), p->slope_ppm, p->reference_z_mm,
            {p->direction.x, p->direction.y},
            p->single_slope_reference == ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE ?
                ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE :
                ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
    }
    for (size_t i = 0; i < d->composition_count; i++) {
        size_t a, b;
        if (!portion_index(d, d->compositions[i].first_portion_id, &a) ||
            !portion_index(d, d->compositions[i].second_portion_id, &b)) {
            free(portions); free(compositions); free(terminations); return ROOF_INVALID_COMPOSITION;
        }
        compositions[i] = (RoofPrototypeComposition){a, b,
            d->compositions[i].kind == ROOF_COMPOSITION_ABUTS ?
                ROOF_PROTOTYPE_COMPOSITION_ABUTS : ROOF_PROTOTYPE_COMPOSITION_INTERSECTS};
    }
    for (size_t i = 0; i < d->termination_count; i++) {
        size_t portion;
        if (!portion_index(d, d->terminations[i].portion_id, &portion)) {
            free(portions); free(compositions); free(terminations); return ROOF_INVALID_TERMINATION;
        }
        terminations[i] = (RoofPrototypeTermination){portion,
            d->terminations[i].end == ROOF_END_POSITIVE_AXIS ?
                ROOF_PROTOTYPE_END_POSITIVE_AXIS : ROOF_PROTOTYPE_END_NEGATIVE_AXIS,
            d->terminations[i].termination_offset_mm};
    }
    RoofPrototypeCompoundIntent intent = {portions, d->portion_count, compositions,
        d->composition_count, terminations, d->termination_count};
    RoofPrototypeCode result;
    if (d->portion_count == 1 && d->composition_count == 0 && d->termination_count == 0)
        result = roof_prototype_build(&portions[0], output);
    else
        result = roof_prototype_build_compound(&intent, output);
    free(portions); free(compositions); free(terminations);
    return result == ROOF_PROTOTYPE_SUCCESS ? ROOF_SUCCESS : ROOF_GEOMETRY_FAILED;
}

RoofCode roof_validate(const Roof *roof)
{
    if (roof == NULL) { return ROOF_INVALID_ARGUMENT; }
    const RoofDefinition *d = &roof->definition;
    if (roof->id == DOMAIN_ID_INVALID) { return ROOF_INVALID_ID; }
    if (!metadata_valid(d->portion_count, d->portion_capacity, d->portions, sizeof *d->portions) ||
        !metadata_valid(d->composition_count, d->composition_capacity, d->compositions, sizeof *d->compositions) ||
        !metadata_valid(d->termination_count, d->termination_capacity, d->terminations, sizeof *d->terminations)) {
        return ROOF_INVALID_COLLECTION;
    }
    if (d->portion_count == 0) { return ROOF_INVALID_PORTION; }
    for (size_t i = 0; i < d->portion_count; i++) {
        const RoofPortionDefinition *p = &d->portions[i];
        if (p->id == DOMAIN_ID_INVALID) { return ROOF_INVALID_ID; }
        if (p->generation != ROOF_PORTION_OPPOSING_SLOPES &&
            p->generation != ROOF_PORTION_ALL_BOUNDARY_SLOPES &&
            p->generation != ROOF_PORTION_SINGLE_SLOPE) { return ROOF_INVALID_PORTION; }
        if (p->single_slope_reference != ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE &&
            p->single_slope_reference != ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE) {
            return ROOF_INVALID_PORTION;
        }
        if (!metadata_valid(p->support_vertex_count, p->support_vertex_capacity,
                p->support_vertices, sizeof *p->support_vertices)) { return ROOF_INVALID_COLLECTION; }
        for (size_t j = 0; j < i; j++) if (d->portions[j].id == p->id) return ROOF_DUPLICATE_PORTION_ID;
    }
    for (size_t i = 0; i < d->composition_count; i++) {
        const RoofComposition *c = &d->compositions[i];
        if (c->first_portion_id == c->second_portion_id ||
            !portion_index(d, c->first_portion_id, NULL) || !portion_index(d, c->second_portion_id, NULL) ||
            (c->kind != ROOF_COMPOSITION_INTERSECTS && c->kind != ROOF_COMPOSITION_ABUTS)) {
            return ROOF_INVALID_COMPOSITION;
        }
        for (size_t j = 0; j < i; j++) {
            if (composition_pair_matches(&d->compositions[j], c->first_portion_id,
                    c->second_portion_id)) { return ROOF_INVALID_COMPOSITION; }
        }
    }
    for (size_t i = 0; i < d->termination_count; i++) {
        const RoofTermination *t = &d->terminations[i];
        if (!portion_index(d, t->portion_id, NULL) || t->termination_offset_mm <= 0 ||
            (t->end != ROOF_END_NEGATIVE_AXIS && t->end != ROOF_END_POSITIVE_AXIS)) {
            return ROOF_INVALID_TERMINATION;
        }
        for (size_t j = 0; j < i; j++) {
            if (d->terminations[j].portion_id == t->portion_id &&
                d->terminations[j].end == t->end) { return ROOF_INVALID_TERMINATION; }
        }
    }
    for (size_t i = 0; i < d->portion_count; i++) {
        const RoofPortionDefinition *p = &d->portions[i];
        RoofPrototypeIntent intent = {p->support_vertices, p->support_vertex_count,
            prototype_generation(p->generation), p->slope_ppm, p->reference_z_mm,
            {p->direction.x, p->direction.y},
            p->single_slope_reference == ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE ?
                ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE :
                ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_LOW_EDGE};
        if (roof_prototype_validate_intent(&intent) != ROOF_PROTOTYPE_SUCCESS) {
            return ROOF_INVALID_PORTION;
        }
    }
    return ROOF_SUCCESS;
}
