#ifndef BUILD_STRUCTURE_H
#define BUILD_STRUCTURE_H

#include <stddef.h>

#include "opening.h"
#include "timber.h"

typedef struct WallDefinition {
    int length;

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
    WallDefinition definition;
    WallFraming framing;
} Wall;

typedef struct Room
{
    Wall *walls;
    size_t wall_count;
    size_t wall_capacity;
} Room;

typedef struct
{
    Room *rooms;
    size_t room_count;
    size_t room_capacity;
} BuildStructure;

#endif