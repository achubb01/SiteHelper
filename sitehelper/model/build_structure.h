#ifndef BUILD_STRUCTURE_H
#define BUILD_STRUCTURE_H

#include <stddef.h>

#include "opening.h"
#include "timber.h"
#include "wall_plan_segment.h"

typedef struct WallDefinition {
    /* Ordered endpoints are the sole physical longitudinal geometry. */
    WallPlanSegment segment;

    Opening *openings;
    size_t opening_count;
    size_t opening_capacity;
} WallDefinition;

typedef struct WallFraming {
    Timber *studs;
    size_t stud_count;
    size_t stud_capacity;

    Timber *nogs;
    size_t nog_count;
    size_t nog_capacity;

    Timber *members;
    size_t member_count;
    size_t member_capacity;

    Timber bottomplate;
    Timber topplate;
} WallFraming;

typedef struct Wall {
    DomainId id;

    WallDefinition definition;
    WallFraming framing;
} Wall;

typedef struct Room
{
    DomainId id;

    /*
     * Unordered wall membership only.  These references do not describe a
     * boundary, topology, orientation, or ownership of physical walls.
     */
    DomainId *wall_ids;
    size_t wall_count;
    size_t wall_capacity;
} Room;

typedef struct
{
    Room *rooms;
    size_t room_count;
    size_t room_capacity;

    /* BuildStructure is the sole owner of physical Wall objects. */
    Wall *walls;
    size_t wall_count;
    size_t wall_capacity;
} BuildStructure;

void build_destroy(
    BuildStructure *structure
);

#endif
