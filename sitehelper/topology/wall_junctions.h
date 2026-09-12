#ifndef WALL_JUNCTIONS_H
#define WALL_JUNCTIONS_H

#include "plan_topology.h"

typedef enum {
    WALL_JUNCTION_PARTICIPANT_START,
    WALL_JUNCTION_PARTICIPANT_END,
    WALL_JUNCTION_PARTICIPANT_INTERIOR
} WallJunctionParticipantPosition;

typedef struct {
    DomainId wall_id;
    WallJunctionParticipantPosition position;
    /* Exact dimensionless parameter on the ordered Wall source, copied unchanged.
     * t == 0 is START, t == 1 is END, otherwise INTERIOR. */
    PlanTopologyRational source_t;
} WallJunctionParticipant;

typedef enum {
    WALL_JUNCTION_CORNER,     /* Two endpoints, non-collinear (any angle). */
    WALL_JUNCTION_CONTINUOUS, /* Two endpoints, collinear. */
    WALL_JUNCTION_T,          /* One endpoint, one interior. */
    WALL_JUNCTION_CROSS,      /* Two interiors. */
    WALL_JUNCTION_MULTIWAY    /* More than two distinct physical Walls. */
} WallJunctionKind;

typedef struct {
    WallJunctionKind kind;
    PlanTopologyVertex position; /* Exact copy; may be fractional. */
    size_t first_participant, participant_count;
} WallJunction;

/* Exclusively owned contiguous arrays. Junctions follow topology's exact
 * lexicographic vertex order; each participant range is sorted by Wall ID,
 * with exactly one entry per distinct physical Wall. No borrowed pointers or
 * persistent junction identities. Do not shallow-copy into another owner. */
typedef struct {
    WallJunction *junctions;
    size_t junction_count;
    WallJunctionParticipant *participants;
    size_t participant_count;
} WallJunctionSet;

typedef enum {
    WALL_JUNCTION_SUCCESS = 0,
    WALL_JUNCTION_INVALID_ARGUMENT,
    WALL_JUNCTION_ALLOCATION_FAILED,
    WALL_JUNCTION_NUMERIC_OVERFLOW /* Array size arithmetic. */
} WallJunctionCode;

/* Requires a successful, unmodified PlanTopology builder result. NULL or
 * unbuilt/destroyed topology is INVALID_ARGUMENT; built empty input succeeds.
 * This is not a validator for hand-constructed/corrupted topology or projects.
 * Output must be zero-initialized or a previous valid result. Success replaces
 * it; failure leaves it entirely unchanged. Input is never mutated.
 *
 * Only distinct physical Wall sources count; RoomSeparators are ignored.
 * Result owns copies and can outlive both topology and project. It describes
 * that topology snapshot only: geometry edits require successful topology and
 * junction rebuilds. Check both statuses; failures preserve older snapshots.
 * No framing treatment, butt object, or internal/external side is inferred. */
WallJunctionCode wall_junctions_build(const PlanTopology *topology,
    WallJunctionSet *output);

/* Frees both arrays and zeroes the structure. NULL/zero/repeated calls safe.
 * All result pointers and participant ranges expire on destruction/rebuild. */
void wall_junctions_destroy(WallJunctionSet *junctions);

#endif
