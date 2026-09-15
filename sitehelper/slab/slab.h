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
    SLAB_PENETRATION_OVERLAP,
    SLAB_INVALID_REGION_COLLECTION,
    SLAB_INVALID_REGION_OUTLINE_COLLECTION,
    SLAB_INVALID_REGION_OUTLINE,
    SLAB_INVALID_REGION_THICKNESS,
    SLAB_REGION_OUTSIDE,
    SLAB_REGION_OVERLAP,
    SLAB_REGION_PENETRATION_INTERSECTION,
    SLAB_INVALID_EDGE_REBATE_COLLECTION,
    SLAB_EDGE_REBATE_INVALID_EDGE,
    SLAB_EDGE_REBATE_INVALID_INTERVAL,
    SLAB_EDGE_REBATE_INVALID_DIMENSIONS,
    SLAB_EDGE_REBATE_OVERLAP
} SlabCode;

/* Gross outer-outline quantities (penetrations are NOT subtracted).
 * Exact area/volume in half-unit increments: area = area2_mm2 / 2 mm²,
 * volume = volume2_mm3 / 2 mm³. Derived diagonals/perimeter are floating-point
 * millimetres. No quantity is stored in the authoritative Slab or persisted. */
typedef struct {
    uint64_t area2_mm2, volume2_mm3;
    double perimeter_mm;
} SlabQuantities;

/* Legacy base-thickness quantities: region overrides are NOT applied.
 * All values are exact doubled units. Net = gross - void; volumes = area *
 * base thickness. These derived quantities are never stored in SlabDefinition. */
typedef struct {
    uint64_t gross_area2_mm2, void_area2_mm2, net_area2_mm2;
    uint64_t gross_volume2_mm3, void_volume2_mm3, net_volume2_mm3;
} SlabMaterialQuantities;

/* Replacement construction before edge-rebate deduction. Plan area is unaffected
 * by region levels. Edge-rebate volume is deliberately excluded. */
typedef struct {
    uint64_t net_area2_mm2, base_material_area2_mm2, region_material_area2_mm2;
    uint64_t total_volume2_mm3;
} SlabConstructionQuantities;
typedef struct {
    uint64_t polygon_area2_mm2, void_area2_mm2, material_area2_mm2, volume2_mm3;
    int thickness_mm;
} SlabRegionQuantities;

typedef enum {
    SLAB_POINT_OUTSIDE, SLAB_POINT_OUTER_BOUNDARY,
    SLAB_POINT_PENETRATION, SLAB_POINT_PENETRATION_BOUNDARY,
    SLAB_POINT_BASE, SLAB_POINT_REGION, SLAB_POINT_REGION_BOUNDARY
} SlabPointKind;
typedef struct {
    SlabPointKind kind;
    size_t region_index; /* SIZE_MAX unless kind == SLAB_POINT_REGION. */
    int thickness_mm;
    int top_level_offset_mm;
    int64_t bottom_level_offset_mm;
} SlabPointProperties;

/* Allocation-free validation of simple convex/concave outlines in either winding.
 * Rejects all repeated vertices, zero area, crossing/touching nonadjacent edges
 * and adjacent backtracking. Collinear forward boundary vertices are allowed.
 * Predicates/area use checked int64_t intermediates; unrepresentable geometry
 * returns NUMERIC_OVERFLOW, never an approximate validity result. No topology
 * dependency. Penetrations must be strictly inside the exterior and pairwise
 * disjoint with no touching/nesting. Regions allow exterior sharing and adjacency
 * but no interior overlap. Exact interval classification handles vertex contacts
 * and concavity; worst-case O(V³) with regions, O(V²) otherwise, O(1) workspace.
 * Input pointers must be valid. */
SlabCode slab_definition_validate(const SlabDefinition *definition);
SlabCode slab_validate(const Slab *slab);

/* Output values are written only on success. Volume multiplication is checked
 * separately: valid geometry may have a volume outside uint64_t twice-mm³.
 * These APIs do not deduct edge rebates; see README.md for the exact limitation. */
SlabCode slab_measure(const SlabDefinition *definition, SlabQuantities *output);
SlabCode slab_measure_material(const SlabDefinition *definition, SlabMaterialQuantities *output);
/* Construction volume applying region thickness, before edge-rebate deduction. */
SlabCode slab_measure_construction(const SlabDefinition *definition, SlabConstructionQuantities *output);
SlabCode slab_region_measure(const SlabDefinition *definition, size_t index, SlabRegionQuantities *output);
/* Exact integer-mm point query. Boundary classifications carry zero properties,
 * even if adjacent properties agree. Exterior boundary takes precedence, then
 * penetration interior/boundary, then region boundary/interior, then base.
 * A penetration remains void even where its boundary touches a region boundary.
 * Outside/void/boundary are successful classifications. Only BASE/REGION have
 * physical properties. Top offset is Storey-relative; bottom = top - thickness,
 * widened to int64_t. Add Storey elevation to either offset for absolute level.
 * Output remains unchanged on failure. */
SlabCode slab_properties_at_plan_position(const Slab *slab, PlanPosition point,
    SlabPointProperties *output);
SlabCode slab_edge_length_mm(const SlabDefinition *definition, size_t edge_index, double *output);
/* Integer-mm edge-local length, matching wall segment round(hypot(dx,dy)). */
SlabCode slab_edge_local_length_mm(const SlabDefinition *definition, size_t edge_index, int *output);
/* Reads only the relative level; widens before adding the two int-mm inputs. */
SlabCode slab_absolute_top_elevation_mm(const Slab *slab, int storey_elevation_mm, int64_t *output);

/* Deep-copy validated inputs into a candidate. Output must be zero or a previous
 * valid owned Slab; success replaces it, failure leaves it entirely unchanged.
 * Source vertices can be borrowed, including from the old output. The new Slab
 * has zero penetrations, regions and edge rebates; use slab_clone() to copy an
 * entire existing Slab. */
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
/* Indexed forms preserve authoritative source order for history restoration.
 * They accept index == current count for append, deep-copy polygon storage and
 * leave the complete Slab unchanged on every failure. */
SlabCode slab_insert_penetration_at(Slab *slab, size_t index,
    const PlanPosition *vertices, size_t vertex_count);
SlabCode slab_remove_penetration(Slab *slab, size_t index);
/* Borrowed read-only access; NULL for invalid index/collection metadata. Count is available
 * as definition.penetrations.count. Successful mutations may invalidate borrows. */
const SlabPenetration *slab_penetration_at(const Slab *slab, size_t index);

/* Same ownership/transactional contracts as penetration APIs. Levels are
 * Storey-relative absolute offsets. Regions replace, rather than add to, base
 * construction. A region wholly inside/coincident with a penetration is invalid:
 * it must describe some existing slab material. Contained penetrations subtract
 * from region material area. Boundary contact alone is allowed; partial interior
 * overlap is rejected. Invalid relationships use REGION_PENETRATION_INTERSECTION.
 * Count/order are definition.regions.count/items; no stable IDs. */
SlabCode slab_region_validate(const SlabRegion *region);
SlabCode slab_add_region(Slab *slab, const PlanPosition *vertices, size_t count,
    int top_level_offset_mm, int thickness_mm);
SlabCode slab_insert_region_at(Slab *slab, size_t index,
    const PlanPosition *vertices, size_t count, int top_level_offset_mm,
    int thickness_mm);
SlabCode slab_remove_region(Slab *slab, size_t index);
const SlabRegion *slab_region_at(const Slab *slab, size_t index);

/* Standalone validation below checks the outer outline and one rebate's host /
 * values. Same-edge collection relationships belong to slab_validate().
 * A rebate is attached only to outer edge i: vertex i -> vertex (i+1)%N. U=0
 * is the first vertex and increases toward the second using the rounded integer
 * edge length. Width is inward in plan; depth is downward from the applicable
 * local slab top. Neither direction nor local top is redundantly stored.
 * Same-edge interiors cannot overlap; exact adjacency is valid. */
SlabCode slab_edge_rebate_validate(const SlabDefinition *definition,
    const SlabEdgeRebate *rebate);
SlabCode slab_add_edge_rebate(Slab *slab, size_t edge_index,
    int start_offset_mm, int end_offset_mm, int width_mm, int depth_mm);
SlabCode slab_insert_edge_rebate_at(Slab *slab, size_t index,
    size_t edge_index, int start_offset_mm, int end_offset_mm,
    int width_mm, int depth_mm);
SlabCode slab_remove_edge_rebate(Slab *slab, size_t index);
const SlabEdgeRebate *slab_edge_rebate_at(const Slab *slab, size_t index);
SlabCode slab_edge_rebate_length_mm(const SlabEdgeRebate *rebate, int *output);

/* Property-only mutation. Geometry/identity/collection order are preserved.
 * Each operation validates the complete resulting Slab and restores the old
 * values on failure, so callers never observe a partially applied property edit. */
SlabCode slab_set_base_properties(Slab *slab, int thickness_mm,
    int top_level_offset_mm);
SlabCode slab_set_region_properties(Slab *slab, size_t index,
    int top_level_offset_mm, int thickness_mm);
SlabCode slab_set_edge_rebate_properties(Slab *slab, size_t index,
    int start_offset_mm, int end_offset_mm, int width_mm, int depth_mm);

/* Geometry-only vertex mutation. Vertex counts/order are preserved. Each
 * operation validates the complete resulting Slab, including subordinate
 * relationships and edge-rebate host intervals, and restores the old vertex
 * on failure. This means outer-outline edits never silently remap rebates:
 * existing edge_index/U references must remain valid or the edit is rejected. */
SlabCode slab_set_outline_vertex(Slab *slab, size_t vertex_index,
    PlanPosition position);
SlabCode slab_set_penetration_vertex(Slab *slab, size_t feature_index,
    size_t vertex_index, PlanPosition position);
SlabCode slab_set_region_vertex(Slab *slab, size_t feature_index,
    size_t vertex_index, PlanPosition position);

/* Lookup/removal reject incoherent collection metadata. Append transfers ownership and
 * zeros candidate only on success; failure changes neither owner. IDs must be
 * unique locally; the project layer enforces the global identity namespace.
 * Candidate must be an independently owned Slab, not share outline storage. */
Slab *slab_collection_find_by_id(SlabCollection *collection, DomainId id);
const Slab *slab_collection_find_by_id_const(const SlabCollection *collection, DomainId id);
SlabCode slab_collection_append(SlabCollection *collection, Slab *candidate);
/* Inserts at a specified source-order position and transfers candidate ownership
 * only on success. Used by transactional history restoration. */
SlabCode slab_collection_insert(SlabCollection *collection, Slab *candidate, size_t index);
int slab_collection_remove_by_id(SlabCollection *collection, DomainId id);

/* All destruction functions free owned storage and zero their object.
 * NULL/zero/repeated calls are safe. Otherwise requires valid owned storage and
 * coherent metadata. Do not shallow-copy into another owner. */
void slab_outline_destroy(SlabOutline *outline);
void slab_destroy(Slab *slab);
void slab_collection_destroy(SlabCollection *collection);

#endif
