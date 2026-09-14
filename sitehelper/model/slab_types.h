#ifndef SLAB_TYPES_H
#define SLAB_TYPES_H

#include <stddef.h>
#include "position.h"
#include "domain_id.h"

/* Ordered integer-mm plan polygon. Closure is implicit; do not repeat vertex 0.
 * Exclusively owned storage; clockwise and counter-clockwise are both valid. */
typedef struct {
    PlanPosition *vertices;
    size_t vertex_count, vertex_capacity;
} SlabOutline;

/* Subordinate slab-owned geometry; no global identity. */
typedef struct {
    SlabOutline outline;
} SlabPenetration;

typedef struct {
    SlabPenetration *items;
    size_t count, capacity;
} SlabPenetrationCollection;

/* Replacement construction, owned by Slab; no global identity. */
typedef struct {
    SlabOutline outline;
    int top_level_offset_mm; /* Relative to Storey, NOT to the slab base. */
    int thickness_mm;
} SlabRegion;

typedef struct {
    SlabRegion *items;
    size_t count, capacity;
} SlabRegionCollection;

/* Perimeter-attached subtractive profile, owned by Slab; no global identity.
 * U offsets follow outer edge edge_index from its start vertex to its end. */
typedef struct {
    size_t edge_index;
    int start_offset_mm, end_offset_mm;
    int width_mm, depth_mm;
} SlabEdgeRebate;

typedef struct {
    SlabEdgeRebate *items;
    size_t count, capacity;
} SlabEdgeRebateCollection;

typedef struct {
    SlabOutline outline;
    int thickness_mm;
    int top_level_offset_mm; /* Signed relative to the owning Storey elevation. */
    SlabPenetrationCollection penetrations;
    SlabRegionCollection regions;
    SlabEdgeRebateCollection edge_rebates;
} SlabDefinition;

typedef struct {
    DomainId id;
    SlabDefinition definition;
} Slab;

/* Owned directly by Storey, independently of BuildStructure. */
typedef struct {
    Slab *items;
    size_t count, capacity;
} SlabCollection;

#endif
