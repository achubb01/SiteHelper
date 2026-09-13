#ifndef SLAB_H
#define SLAB_H

#include <stdint.h>
#include "slab_types.h"

typedef enum {
    SLAB_SUCCESS = 0,
    SLAB_INVALID_ARGUMENT,
    SLAB_INVALID_ID,
    SLAB_INVALID_COLLECTION,
    SLAB_INVALID_OUTLINE,
    SLAB_INVALID_THICKNESS,
    SLAB_SELF_INTERSECTION,
    SLAB_NUMERIC_OVERFLOW,
    SLAB_ALLOCATION_FAILED
} SlabCode;

/* Exact area/volume in half-unit increments: area = area2_mm2 / 2 mm²,
 * volume = volume2_mm3 / 2 mm³. Derived diagonals/perimeter are floating-point
 * millimetres. No quantity is stored in the authoritative Slab or persisted. */
typedef struct {
    uint64_t area2_mm2, volume2_mm3;
    double perimeter_mm;
} SlabQuantities;

/* Allocation-free validation of simple convex/concave outlines in either winding.
 * Rejects all repeated vertices, zero area, crossing/touching nonadjacent edges
 * and adjacent backtracking. Collinear forward boundary vertices are allowed.
 * Predicates/area use checked int64_t intermediates; unrepresentable geometry
 * returns NUMERIC_OVERFLOW, never an approximate validity result. No topology
 * dependency. O(N²) time, O(1) workspace. Input pointers must be valid. */
SlabCode slab_definition_validate(const SlabDefinition *definition);
SlabCode slab_validate(const Slab *slab);

/* Output values are written only on success. Volume multiplication is checked
 * separately: valid geometry may have a volume outside uint64_t twice-mm³. */
SlabCode slab_measure(const SlabDefinition *definition, SlabQuantities *output);
SlabCode slab_edge_length_mm(const SlabDefinition *definition, size_t edge_index, double *output);
/* Reads only the relative level; widens before adding the two int-mm inputs. */
SlabCode slab_absolute_top_elevation_mm(const Slab *slab, int storey_elevation_mm, int64_t *output);

/* Deep-copy validated inputs into a candidate. Output must be zero or a previous
 * valid owned Slab; success replaces it, failure leaves it entirely unchanged.
 * Source vertices can be borrowed, including from the old output. */
SlabCode slab_build(DomainId id, const PlanPosition *vertices, size_t vertex_count,
    int thickness_mm, int top_level_offset_mm, Slab *output);

/* Lookup/removal reject incoherent collection metadata. Append transfers ownership and
 * zeros candidate only on success; failure changes neither owner. IDs must be
 * unique locally; the project layer enforces the global identity namespace.
 * Candidate must be an independently owned Slab, not share outline storage. */
Slab *slab_collection_find_by_id(SlabCollection *collection, DomainId id);
const Slab *slab_collection_find_by_id_const(const SlabCollection *collection, DomainId id);
SlabCode slab_collection_append(SlabCollection *collection, Slab *candidate);
int slab_collection_remove_by_id(SlabCollection *collection, DomainId id);

/* All destruction functions free owned storage and zero their object.
 * NULL/zero/repeated calls are safe. Otherwise requires valid owned storage and
 * coherent metadata. Do not shallow-copy into another owner. */
void slab_outline_destroy(SlabOutline *outline);
void slab_destroy(Slab *slab);
void slab_collection_destroy(SlabCollection *collection);

#endif
