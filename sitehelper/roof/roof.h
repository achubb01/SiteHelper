#ifndef ROOF_H
#define ROOF_H

#include "roof_types.h"
#include "roof_geometry_prototype.h"

typedef enum {
    ROOF_SUCCESS = 0,
    ROOF_INVALID_ARGUMENT,
    ROOF_INVALID_COLLECTION,
    ROOF_INVALID_ID,
    ROOF_DUPLICATE_PORTION_ID,
    ROOF_INVALID_PORTION,
    ROOF_INVALID_COMPOSITION,
    ROOF_INVALID_TERMINATION,
    ROOF_GEOMETRY_FAILED,
    ROOF_ALLOCATION_FAILED
} RoofCode;

void roof_destroy(Roof *roof);
RoofCode roof_clone(const Roof *source, Roof *output);
void roof_collection_destroy(RoofCollection *collection);
Roof *roof_collection_find_by_id(RoofCollection *collection, DomainId id);
const Roof *roof_collection_find_by_id_const(const RoofCollection *collection, DomainId id);
RoofPortionDefinition *roof_find_portion_by_id(Roof *roof, DomainId id);
const RoofPortionDefinition *roof_find_portion_by_id_const(const Roof *roof, DomainId id);
RoofCode roof_definition_append_portion(RoofDefinition *definition, DomainId id,
    const RoofPortionSpec *spec);
RoofCode roof_definition_append_composition(RoofDefinition *definition,
    RoofComposition composition);
RoofCode roof_definition_append_termination(RoofDefinition *definition,
    RoofTermination termination);
/* Priority 26G2B source-edit primitives. These mutate an isolated definition;
 * callers are responsible for validating/regenerating the containing Roof
 * transactionally before committing it to a Project. */
RoofCode roof_definition_replace_portion(RoofDefinition *definition, DomainId portion_id,
    const RoofPortionSpec *spec);
RoofCode roof_definition_remove_portion(RoofDefinition *definition, DomainId portion_id);
RoofCode roof_definition_set_composition(RoofDefinition *definition, RoofComposition composition);
RoofCode roof_definition_remove_composition(RoofDefinition *definition, DomainId first_portion_id,
    DomainId second_portion_id);
RoofCode roof_definition_set_termination(RoofDefinition *definition, RoofTermination termination);
RoofCode roof_definition_remove_termination(RoofDefinition *definition, DomainId portion_id,
    RoofEnd end);
int roof_authority_equal(const Roof *a, const Roof *b);
RoofCode roof_validate(const Roof *roof);

/* Transitional 26G1 bridge: authoritative production RoofDefinition is real,
 * while the still-experimental exact derived topology stays in the proven
 * prototype snapshot until its rational-boundary representation is promoted.
 * output is replaced only on success. */
RoofCode roof_build_derived_geometry(const Roof *roof, RoofPrototypeGeometry *output);

#endif
