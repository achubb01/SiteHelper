#ifndef ROOF_COMPOUND_PROTOTYPE_H
#define ROOF_COMPOUND_PROTOTYPE_H

#include "roof_geometry_prototype.h"

typedef enum {
    ROOF_PROTOTYPE_COMPOSITION_INTERSECTS = 0,
    ROOF_PROTOTYPE_COMPOSITION_ABUTS
} RoofPrototypeCompositionKind;

typedef struct {
    size_t first_portion;
    size_t second_portion;
    RoofPrototypeCompositionKind kind;
} RoofPrototypeComposition;

typedef enum {
    ROOF_PROTOTYPE_END_NEGATIVE_AXIS = 0,
    ROOF_PROTOTYPE_END_POSITIVE_AXIS
} RoofPrototypeEnd;

/* Primitive end-treatment authority introduced by Priority 26E2. It says that
 * one end boundary slopes inward as a hip-end surface only as far as an
 * explicit station measured from that end. The resulting cut edge, hip edges
 * and vertical gablet closure are all derived. This is deliberately not a
 * DUTCH_GABLE roof type. */
typedef struct {
    size_t portion;
    RoofPrototypeEnd end;
    int termination_offset_mm;
} RoofPrototypeTermination;

/* Priority 26E compound source authority. Individual portions retain the 26D
 * source contract. Relationships and end terminations are explicit authority;
 * plan overlap alone never causes portions to interact. */
typedef struct {
    const RoofPrototypeIntent *portions;
    size_t portion_count;
    const RoofPrototypeComposition *compositions;
    size_t composition_count;
    const RoofPrototypeTermination *terminations;
    size_t termination_count;
} RoofPrototypeCompoundIntent;

RoofPrototypeCode roof_prototype_build_compound(
    const RoofPrototypeCompoundIntent *intent,
    RoofPrototypeGeometry *output);

#endif
