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
    SLAB_ALLOCATION_FAILED,
    SLAB_INVALID_PENETRATION_COLLECTION,
    SLAB_INVALID_PENETRATION_OUTLINE_COLLECTION,
    SLAB_INVALID_PENETRATION_OUTLINE,
    /* Outside, crossing or touching the exterior boundary. */
    SLAB_PENETRATION_OUTSIDE,
    /* Crossing, touching, duplicate or nested/enclosing penetrations. */
    SLAB_PENETRATION_OVERLAP
} SlabCode;

/* Gross outer-outline quantities (penetrations are NOT subtracted).
 * Exact area/volume in half-unit increments: area = area2_mm2 / 2 mm²,
 * volume = volume2_mm3 / 2 mm³. Derived diagonals/perimeter are floating-point
 * millimetres. No quantity is stored in the authoritative Slab or persisted. */
typedef struct {
    uint64_t area2_mm2, volume2_mm3;
    double perimeter_mm;
} SlabQuantities;

/* All values are exact doubled units. Net = gross - void; volumes = area *
 * thickness. These derived quantities are never stored in SlabDefinition. */
typedef struct {
    uint64_t gross_area2_mm2, void_area2_mm2, net_area2_mm2;
    uint64_t gross_volume2_mm3, void_volume2_mm3, net_volume2_mm3;
} SlabMaterialQuantities;

/* Allocation-free validation of simple convex/concave outlines in either winding.
 * Rejects all repeated vertices, zero area, crossing/touching nonadjacent edges
 * and adjacent backtracking. Collinear forward boundary vertices are allowed.
 * Predicates/area use checked int64_t intermediates; unrepresentable geometry
 * returns NUMERIC_OVERFLOW, never an approximate validity result. No topology
 * dependency. Penetrations must be strictly inside the exterior and pairwise
 * disjoint with no touching/nesting. O(V²) time for total vertices, O(1)
 * workspace. Input pointers must be valid. */
SlabCode slab_definition_validate(const SlabDefinition *definition);
SlabCode slab_validate(const Slab *slab);

/* Output values are written only on success. Volume multiplication is checked
 * separately: valid geometry may have a volume outside uint64_t twice-mm³. */
SlabCode slab_measure(const SlabDefinition *definition, SlabQuantities *output);
SlabCode slab_measure_material(const SlabDefinition *definition, SlabMaterialQuantities *output);
SlabCode slab_edge_length_mm(const SlabDefinition *definition, size_t edge_index, double *output);
/* Reads only the relative level; widens before adding the two int-mm inputs. */
SlabCode slab_absolute_top_elevation_mm(const Slab *slab, int storey_elevation_mm, int64_t *output);

/* Deep-copy validated inputs into a candidate. Output must be zero or a previous
 * valid owned Slab; success replaces it, failure leaves it entirely unchanged.
 * Source vertices can be borrowed, including from the old output. The new Slab
 * has zero penetrations; use slab_clone() to copy an entire existing Slab. */
SlabCode slab_build(DomainId id, const PlanPosition *vertices, size_t vertex_count,
    int thickness_mm, int top_level_offset_mm, Slab *output);

/* Deep-copy the complete definition and identity. Same output contract as build;
 * source == output is supported. No borrowed geometry survives success. */
SlabCode slab_clone(const Slab *source, Slab *output);

/* Standalone outline validity; relationship validity belongs to slab_validate(). */
SlabCode slab_penetration_validate(const SlabPenetration *penetration);
/* Both operations require a valid identified Slab and preserve it on any failure.
 * Add copies vertices and appends in source order. Remove preserves remaining
 * order. Index is a transient collection position, not a persistent identity. */
SlabCode slab_add_penetration(Slab *slab, const PlanPosition *vertices, size_t vertex_count);
SlabCode slab_remove_penetration(Slab *slab, size_t index);
/* Borrowed read-only access; NULL for invalid index/collection metadata. Count is available
 * as definition.penetrations.count. Successful mutations may invalidate borrows. */
const SlabPenetration *slab_penetration_at(const Slab *slab, size_t index);

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
