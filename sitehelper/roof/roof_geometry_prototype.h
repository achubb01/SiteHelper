#ifndef ROOF_GEOMETRY_PROTOTYPE_H
#define ROOF_GEOMETRY_PROTOTYPE_H

#include <stddef.h>
#include <stdint.h>

#include "position.h"

#define ROOF_PROTOTYPE_SLOPE_SCALE INT64_C(1000000)

typedef enum {
    ROOF_PROTOTYPE_SUCCESS = 0,
    ROOF_PROTOTYPE_INVALID_ARGUMENT,
    ROOF_PROTOTYPE_INVALID_SUPPORT,
    ROOF_PROTOTYPE_INVALID_SLOPE,
    ROOF_PROTOTYPE_INVALID_DIRECTION,
    ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY,
    ROOF_PROTOTYPE_NUMERIC_OVERFLOW,
    ROOF_PROTOTYPE_ALLOCATION_FAILED
} RoofPrototypeCode;

/* These are generation intents for the Priority 26D prototype, not a persisted
 * global RoofType taxonomy. 26E is explicitly allowed to replace this shape if
 * compound roofs show that it forces named-style special cases. */
typedef enum {
    ROOF_PROTOTYPE_OPPOSING_SLOPES = 0, /* Simple gable for the rectangle prototype. */
    ROOF_PROTOTYPE_ALL_BOUNDARY_SLOPES, /* Equal-pitch hip for the rectangle prototype. */
    ROOF_PROTOTYPE_SINGLE_SLOPE          /* Skillion / monoslope. */
} RoofPrototypeGeneration;

/* Plan-space unit direction. 26D accepts only axis directions because its
 * frozen fixtures are axis-aligned rectangles. For OPPOSING_SLOPES this is the
 * ridge axis; for SINGLE_SLOPE it is the downhill direction; it must be zero
 * for ALL_BOUNDARY_SLOPES. */
typedef struct {
    int x;
    int y;
} RoofPrototypeDirection;

/* Borrowed authoritative prototype input. The support must contain the four
 * distinct corners of one axis-aligned rectangle; winding/start vertex do not
 * matter. Source coordinates and reference_z_mm are integer millimetres.
 * slope_ppm is positive fixed rise/run using ROOF_PROTOTYPE_SLOPE_SCALE.
 *
 * reference_z_mm means the support/eave datum for OPPOSING_SLOPES and
 * ALL_BOUNDARY_SLOPES, and the low-edge datum for SINGLE_SLOPE. No derived
 * ridge, hip, plane polygon or high-edge elevation is authoritative input. */
typedef enum {
    /* Existing 26D behaviour. For a skillion the low boundary owns
     * reference_z_mm. */
    ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_LOW_EDGE = 0,
    /* Required by F0/F1: the high boundary owns reference_z_mm instead. */
    ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE
} RoofPrototypeSingleSlopeReference;

typedef struct {
    const PlanPosition *support_vertices;
    size_t support_vertex_count;
    RoofPrototypeGeneration generation;
    int64_t slope_ppm;
    int reference_z_mm;
    RoofPrototypeDirection direction;
    /* Meaningful only for SINGLE_SLOPE. Zero preserves the 26D low-edge
     * datum contract; E3 proves that the datum-owning parallel edge must be
     * explicit for multi-level composition. */
    RoofPrototypeSingleSlopeReference single_slope_reference;
} RoofPrototypeIntent;

/* Exact reduced rational millimetres. 26D needs only fixture-scale int64_t
 * arithmetic; every operation is checked and reports NUMERIC_OVERFLOW. The
 * portable wide exact representation required for compound 26E work remains a
 * separate implementation decision. denominator is always positive. */
typedef struct {
    int64_t numerator;
    int64_t denominator;
} RoofPrototypeRational;

typedef struct {
    RoofPrototypeRational x;
    RoofPrototypeRational y;
    RoofPrototypeRational z;
} RoofPrototypePoint3;

/* Integer affine plane from Priority 26C:
 *   SCALE * Z = gradient_x_ppm*X + gradient_y_ppm*Y + offset_scaled_mm. */
typedef struct {
    int64_t gradient_x_ppm;
    int64_t gradient_y_ppm;
    int64_t offset_scaled_mm;
} RoofPrototypePlaneEquation;

typedef struct {
    RoofPrototypePlaneEquation equation;
    RoofPrototypePoint3 *vertices; /* Owned, ordered boundary of visible plane. */
    size_t vertex_count;
} RoofPrototypePlane;

typedef enum {
    ROOF_PROTOTYPE_BOUNDARY_EAVE,
    ROOF_PROTOTYPE_BOUNDARY_GABLE_VERGE,
    ROOF_PROTOTYPE_BOUNDARY_LOW_EAVE,
    ROOF_PROTOTYPE_BOUNDARY_HIGH_EDGE,
    ROOF_PROTOTYPE_BOUNDARY_SIDE_VERGE
} RoofPrototypeBoundaryKind;

typedef struct {
    PlanPosition start;
    PlanPosition end;
    RoofPrototypeBoundaryKind kind;
} RoofPrototypeBoundary;

typedef enum {
    ROOF_PROTOTYPE_INTERIOR_RIDGE,
    ROOF_PROTOTYPE_INTERIOR_HIP,
    ROOF_PROTOTYPE_INTERIOR_VALLEY,
    /* Intentional end/cut seam. The seam is derived geometry; the source
     * authority is the termination instruction that causes it. */
    ROOF_PROTOTYPE_INTERIOR_TERMINATION_CUT,
    /* Generic visible plane-plane seam that is not a ridge, hip or valley. */
    ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM
} RoofPrototypeInteriorEdgeKind;

typedef struct {
    RoofPrototypePoint3 start;
    RoofPrototypePoint3 end;
    RoofPrototypeInteriorEdgeKind kind;
} RoofPrototypeInteriorEdge;

typedef enum {
    ROOF_PROTOTYPE_INTERFACE_STEP_ABUTMENT = 0
} RoofPrototypeInterfaceKind;

/* A composition interface can contain two distinct 3D edges with one plan
 * projection. F0 needs this representation; collapsing it to one topology
 * edge would lose the vertical step. */
typedef struct {
    RoofPrototypeInterfaceKind kind;
    RoofPrototypeInteriorEdge first_edge;
    RoofPrototypeInteriorEdge second_edge;
} RoofPrototypeInterface;

/* Exclusively owned transient geometry. Array indices/borrows expire on destroy
 * or successful rebuild. No element receives a DomainId and none is project
 * authority. A failed build leaves an existing output unchanged. */
typedef struct {
    RoofPrototypePlane *planes;
    size_t plane_count;
    RoofPrototypeBoundary *boundaries;
    size_t boundary_count;
    RoofPrototypeInteriorEdge *interior_edges;
    size_t interior_edge_count;
    RoofPrototypeInterface *interfaces;
    size_t interface_count;
} RoofPrototypeGeometry;

RoofPrototypeCode roof_prototype_validate_intent(const RoofPrototypeIntent *intent);
RoofPrototypeCode roof_prototype_build(const RoofPrototypeIntent *intent,
    RoofPrototypeGeometry *output);
void roof_prototype_geometry_destroy(RoofPrototypeGeometry *geometry);

#endif
