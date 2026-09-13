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

typedef struct {
    SlabOutline outline;
    int thickness_mm;
    int top_level_offset_mm; /* Signed relative to the owning Storey elevation. */
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
