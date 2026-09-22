#ifndef WALL_ELEVATION_PRESENTATION_H
#define WALL_ELEVATION_PRESENTATION_H

#include <stddef.h>

#include "wall.h"

/* Read-only framing/elevation interpretation. This module does not own Wall
 * state, allocate member identity, or expose renderer types. All geometry is
 * wall-local integer millimetres (U/Z). */
typedef enum
{
    WALL_ELEVATION_MEMBER_ROLE_UNKNOWN = 0,
    WALL_ELEVATION_MEMBER_ROLE_BOTTOM_PLATE,
    WALL_ELEVATION_MEMBER_ROLE_TOP_PLATE,
    WALL_ELEVATION_MEMBER_ROLE_COMMON_STUD,
    WALL_ELEVATION_MEMBER_ROLE_KING_STUD,
    WALL_ELEVATION_MEMBER_ROLE_TRIMMER_STUD,
    WALL_ELEVATION_MEMBER_ROLE_CRIPPLE_STUD,
    WALL_ELEVATION_MEMBER_ROLE_NOGGIN,
    WALL_ELEVATION_MEMBER_ROLE_HEADER,
    WALL_ELEVATION_MEMBER_ROLE_SILL
} WallElevationMemberRole;

typedef enum
{
    WALL_ELEVATION_MEMBER_HORIZONTAL,
    WALL_ELEVATION_MEMBER_VERTICAL
} WallElevationMemberOrientation;

typedef struct
{
    int u;
    int z;
    int width;
    int height;
} WallElevationRect;

typedef struct
{
    WallMemberKind selection_kind;
    WallElevationMemberRole role;
    WallElevationMemberOrientation orientation;
    WallElevationRect bounds;

    /* Borrowed only for the duration of the current synchronous query/render.
     * Never store this pointer in editor/project state; generated framing may
     * be replaced by any Wall regeneration. */
    const Timber *source;
} WallElevationMember;

typedef struct
{
    DomainId opening_id;
    OpeningType type;
    WallElevationRect clear_bounds;
} WallElevationOpening;

/* Stable human-facing semantic names for transient elevation annotations.
 * These are presentation strings only; they are not persisted object names. */
const char *wall_elevation_member_role_name(WallElevationMemberRole role);
const char *wall_elevation_opening_type_name(OpeningType type);

/* Full generated framing extents, including the top plate thickness. */
int wall_elevation_presentation_bounds(
    const Wall *wall,
    const BuildSettings *settings,
    WallElevationRect *bounds
);

/* Deterministic semantic member stream. Order intentionally preserves the old
 * elevation draw/hit precedence: bottom plate, top plate, studs, noggins, then
 * generated opening members. */
size_t wall_elevation_presentation_member_count(const Wall *wall);
int wall_elevation_presentation_member_at(
    const Wall *wall,
    size_t index,
    WallElevationMember *member
);
int wall_elevation_presentation_find_member(
    const Wall *wall,
    WallLocalPosition position,
    WallElevationMember *member
);

/* Openings remain authoritative Wall definitions. Presentation exposes their
 * effective clear frame rectangles and stable IDs without requiring framing. */
size_t wall_elevation_presentation_opening_count(const Wall *wall);
int wall_elevation_presentation_opening_at(
    const Wall *wall,
    const BuildSettings *settings,
    size_t index,
    WallElevationOpening *opening
);
int wall_elevation_presentation_find_opening(
    const Wall *wall,
    const BuildSettings *settings,
    WallLocalPosition position,
    WallElevationOpening *opening
);

#endif
