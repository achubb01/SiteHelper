#ifndef BUILD_STRUCTURE_H
#define BUILD_STRUCTURE_H

#include <stddef.h>
#include <stdbool.h>

#include "opening.h"
#include "timber.h"
#include "wall_plan_segment.h"
#include "room_separator.h"

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

    /* Intended region contains this authoritative plan point. This is neither
     * a boundary nor label/layout geometry. When false, location is ignored. */
    bool has_location;
    PlanPosition location;
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

    /* Globally owned virtual inputs, separate from physical walls. Stored
     * order is preserved by persistence; it has no topology meaning. */
    RoomSeparator *room_separators;
    size_t room_separator_count;
    size_t room_separator_capacity;
} BuildStructure;

/* Identity queries assume coherent authoritative collection metadata. */
int build_contains_domain_id(const BuildStructure *structure, DomainId id);
RoomSeparator *build_find_room_separator_by_id(BuildStructure *structure, DomainId id);
const RoomSeparator *build_find_room_separator_by_id_const(const BuildStructure *structure, DomainId id);
/* Copies an existing identity at an explicit storage position; for loading and
 * restoration. Rejects all live identity collisions without allocating IDs. */
int build_insert_room_separator(BuildStructure *structure,
    const RoomSeparator *separator, size_t index);
int build_remove_room_separator_by_id(BuildStructure *structure, DomainId id);

void build_destroy(
    BuildStructure *structure
);

#endif
