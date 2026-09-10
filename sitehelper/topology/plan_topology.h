#ifndef PLAN_TOPOLOGY_H
#define PLAN_TOPOLOGY_H

#include "sitehelper_project.h"

/* Exact reduced rational: (-1)^negative * numerator / denominator. Limbs
 * encode lo + hi * 2^64; denominator is positive, zero is nonnegative / 1.
 * These are derived coordinates, never rounded authoritative PlanPositions. */
typedef struct { uint64_t lo, hi; } PlanTopologyMagnitude;
typedef struct {
    PlanTopologyMagnitude numerator, denominator;
    bool negative;
} PlanTopologyRational;

typedef enum {
    PLAN_TOPOLOGY_SOURCE_WALL,
    PLAN_TOPOLOGY_SOURCE_ROOM_SEPARATOR
} PlanTopologySourceKind;

typedef struct {
    PlanTopologySourceKind kind;
    DomainId source_id;
    PlanSegment segment;
} PlanTopologySource;

typedef struct { PlanTopologyRational x, y; } PlanTopologyVertex;

typedef struct {
    size_t start_vertex, end_vertex; /* Canonical lexicographic direction. */
    PlanTopologySourceKind source_kind;
    DomainId source_id;
    /* Owned copy of the original ordered source, retained for exact integer
     * supporting-line predicates. Derived provenance, never project state. */
    PlanSegment source_segment;
    /* Exact parameters on the original ordered source: P(t) = A + t(B-A).
     * Can descend when source order opposes canonical edge direction. For a
     * Wall, U = t * wall_length_mm(wall) matches its existing transform policy. */
    PlanTopologyRational source_t_start, source_t_end;
    /* Face on the left of each traversal, not Room-side assignments. */
    size_t forward_face, reverse_face;
} PlanTopologyEdge;

typedef struct { size_t edge; bool reversed; } PlanTopologyBoundaryStep;
typedef struct { size_t first_step, step_count; } PlanTopologyBoundary;
typedef struct {
    bool bounded;
    size_t first_boundary, boundary_count;
} PlanTopologyFace;

/* Exclusively owned arrays; indices are valid until destruction/rebuild.
 * Face 0 is always the single unbounded exterior (even for empty input).
 * A bounded face has one simple CCW boundary. Exterior boundaries can retrace
 * bridges/open segments. Step successors wrap within their boundary range. */
typedef struct {
    PlanTopologyVertex *vertices;
    size_t vertex_count;
    PlanTopologyEdge *edges;
    size_t edge_count;
    PlanTopologyFace *faces;
    size_t face_count;
    PlanTopologyBoundary *boundaries;
    size_t boundary_count;
    PlanTopologyBoundaryStep *steps;
    size_t step_count;
} PlanTopology;

typedef enum {
    PLAN_TOPOLOGY_SUCCESS = 0,
    PLAN_TOPOLOGY_INVALID_ARGUMENT,
    PLAN_TOPOLOGY_INVALID_SOURCE,
    PLAN_TOPOLOGY_ALLOCATION_FAILED,
    PLAN_TOPOLOGY_NUMERIC_OVERFLOW,
    PLAN_TOPOLOGY_UNSUPPORTED_OVERLAP,
    PLAN_TOPOLOGY_UNSUPPORTED_NESTING,
    PLAN_TOPOLOGY_UNSUPPORTED_NON_SIMPLE_FACE
} PlanTopologyCode;

typedef struct {
    PlanTopologyCode code;
    DomainId source_id, related_source_id; /* Zero if not applicable. */
} PlanTopologyResult;

/* Output must be zero-initialized or a previous successful result. Success
 * replaces it; failure leaves it entirely unchanged. Input is never mutated.
 * Sources need distinct nonzero IDs and nonzero segments. Positive-length
 * collinear overlaps (including duplicates), nested disconnected components,
 * and non-simple bounded walks are explicitly unsupported in this version. */
PlanTopologyResult plan_topology_build(const PlanTopologySource *sources,
    size_t source_count, PlanTopology *output);

/* Reads only Wall/separator collections, IDs and ordered plan endpoints.
 * Does not read Rooms, openings, framing, settings, or allocator state and
 * does not call whole-project validation. */
PlanTopologyResult plan_topology_build_from_project(const SiteHelperProject *project,
    PlanTopology *output);
void plan_topology_destroy(PlanTopology *topology);

typedef enum {
    PLAN_TOPOLOGY_POINT_UNCLASSIFIED = 0, /* Inspect code; no spatial answer. */
    PLAN_TOPOLOGY_POINT_BOUNDED,
    PLAN_TOPOLOGY_POINT_UNBOUNDED,
    PLAN_TOPOLOGY_POINT_ON_BOUNDARY
} PlanTopologyPointState;

typedef struct {
    PlanTopologyCode code;
    PlanTopologyPointState state;
    size_t face_index; /* 0 for exterior; SIZE_MAX for boundary or failure. */
} PlanTopologyPointResult;

/* Allocation-free exact query. Requires a successful, unmodified builder
 * result; this is not a validator for hand-constructed/corrupted topology.
 * NULL/unbuilt output is INVALID_ARGUMENT, not the exterior. Any edge/vertex
 * takes precedence over containment, including isolated/open segments.
 * Face indices are local to this topology and expire on destruction/rebuild.
 * Callers must check build status before querying: a failed rebuild preserves
 * the old topology, which still describes the old source geometry. */
PlanTopologyPointResult plan_topology_find_face_at_plan_position(
    const PlanTopology *topology, PlanPosition position);

#endif
