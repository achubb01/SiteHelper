#ifndef ROOF_TYPES_H
#define ROOF_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include "domain_id.h"
#include "position.h"

#define ROOF_SLOPE_SCALE INT64_C(1000000)

typedef enum {
    ROOF_PORTION_OPPOSING_SLOPES = 0,
    ROOF_PORTION_ALL_BOUNDARY_SLOPES,
    ROOF_PORTION_SINGLE_SLOPE
} RoofPortionGeneration;

typedef struct {
    int x;
    int y;
} RoofDirection;

typedef enum {
    ROOF_SINGLE_SLOPE_REFERENCE_LOW_EDGE = 0,
    ROOF_SINGLE_SLOPE_REFERENCE_HIGH_EDGE
} RoofSingleSlopeReference;

typedef struct {
    DomainId id;
    PlanPosition *support_vertices;
    size_t support_vertex_count, support_vertex_capacity;
    RoofPortionGeneration generation;
    int64_t slope_ppm;
    int reference_z_mm;
    RoofDirection direction;
    RoofSingleSlopeReference single_slope_reference;
} RoofPortionDefinition;

typedef enum {
    ROOF_COMPOSITION_INTERSECTS = 0,
    ROOF_COMPOSITION_ABUTS
} RoofCompositionKind;

/* Relationship authority uses stable portion IDs, never collection indexes. */
typedef struct {
    DomainId first_portion_id;
    DomainId second_portion_id;
    RoofCompositionKind kind;
} RoofComposition;

typedef enum {
    ROOF_END_NEGATIVE_AXIS = 0,
    ROOF_END_POSITIVE_AXIS
} RoofEnd;

typedef struct {
    DomainId portion_id;
    RoofEnd end;
    int termination_offset_mm;
} RoofTermination;

typedef struct {
    RoofPortionDefinition *portions;
    size_t portion_count, portion_capacity;
    RoofComposition *compositions;
    size_t composition_count, composition_capacity;
    RoofTermination *terminations;
    size_t termination_count, termination_capacity;
} RoofDefinition;

typedef struct {
    DomainId id;
    RoofDefinition definition;
} Roof;

typedef struct {
    Roof *items;
    size_t count, capacity;
} RoofCollection;

/* Borrowed creation input. The current production authority deliberately
 * promotes only the rectangle-based source contract proven by Priority 26D-E.
 * More general support geometry can evolve later without changing roof/portion
 * identity or turning derived planes/edges into authority. */
typedef struct {
    const PlanPosition *support_vertices;
    size_t support_vertex_count;
    RoofPortionGeneration generation;
    int64_t slope_ppm;
    int reference_z_mm;
    RoofDirection direction;
    RoofSingleSlopeReference single_slope_reference;
} RoofPortionSpec;

#endif
