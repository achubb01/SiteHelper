#ifndef ROOF_STRUCTURAL_LAYOUT_PROTOTYPE_H
#define ROOF_STRUCTURAL_LAYOUT_PROTOTYPE_H

#include <stddef.h>

#include "roof_geometry_prototype.h"

typedef enum {
    ROOF_PROTOTYPE_STRUCTURE_CONVENTIONAL_RAFTERS = 0,
    ROOF_PROTOTYPE_STRUCTURE_PREFABRICATED_TRUSSES
} RoofPrototypeStructuralStrategy;

typedef enum {
    /* Rafters meet at the geometric ridge, but the ridge is not declared as a
     * bearing/support line by this structural intent. */
    ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING = 0,
    /* The structural intent requires support at the geometric ridge. The
     * actual supporting member/load path remains outside this prototype. */
    ROOF_PROTOTYPE_RIDGE_REQUIRES_BEARING_SUPPORT
} RoofPrototypeRidgeRole;

/* Authored structural support intent. These plan lines are deliberately
 * separate from RoofPrototypeBoundary: a geometric eave is not automatically
 * a structural bearing line. The prototype validates the supplied lines
 * against the derived geometry instead of inventing support from it. */
typedef struct {
    PlanPosition start;
    PlanPosition end;
} RoofPrototypeSupportLine;

/* Structural intent/layout is support topology, not member sizing, design
 * verification or engineering approval. Later calculations consume this kind
 * of state through the boundary documented in ../structural/README.md. */
typedef struct {
    RoofPrototypeStructuralStrategy strategy;
    const RoofPrototypeSupportLine *bearing_lines;
    size_t bearing_line_count;

    /* Axis along which the spanning structural system travels between the two
     * authored bearing lines. 26F deliberately supports only axis-aligned A0
     * geometry. */
    RoofPrototypeDirection span_direction;

    /* Meaningful only for CONVENTIONAL_RAFTERS. Truss layouts do not acquire
     * a ridge support merely because the envelope has a ridge. */
    RoofPrototypeRidgeRole conventional_ridge_role;
} RoofPrototypeStructuralIntent;

typedef enum {
    ROOF_PROTOTYPE_LAYOUT_RAFTER_FIELD = 0,
    ROOF_PROTOTYPE_LAYOUT_TRUSS_RUN
} RoofPrototypeStructuralFieldKind;

/* A structural field is layout topology, not an individual physical member.
 * plane_indices and interior_edge_index are transient references into the
 * specific RoofPrototypeGeometry snapshot consumed by the build. They must be
 * discarded/rebuilt when that geometry is rebuilt. */
typedef struct {
    RoofPrototypeStructuralFieldKind kind;
    size_t plane_indices[2];
    size_t plane_count;
    size_t bearing_line_indices[2];
    size_t bearing_line_count;
    size_t interior_edge_index; /* SIZE_MAX when no derived edge terminates the field. */
} RoofPrototypeStructuralField;

typedef struct {
    RoofPrototypeStructuralStrategy strategy;
    RoofPrototypeDirection span_direction;
    RoofPrototypeRidgeRole conventional_ridge_role;

    RoofPrototypeSupportLine *bearing_lines; /* owned copy of authored intent */
    size_t bearing_line_count;
    RoofPrototypeStructuralField *fields;    /* owned transient resolved layout */
    size_t field_count;
} RoofPrototypeStructuralLayout;

RoofPrototypeCode roof_prototype_build_structural_layout(
    const RoofPrototypeGeometry *geometry,
    const RoofPrototypeStructuralIntent *intent,
    RoofPrototypeStructuralLayout *output);

void roof_prototype_structural_layout_destroy(
    RoofPrototypeStructuralLayout *layout);

#endif
