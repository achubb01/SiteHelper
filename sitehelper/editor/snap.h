#ifndef SNAP_H
#define SNAP_H

#include "geometry.h"

typedef enum
{
    SNAP_NONE,
    SNAP_GRID,
    SNAP_STUD,
    SNAP_ENDPOINT,
    SNAP_OPENING
} SnapType;

typedef struct
{
    Vec2 position;
    SnapType type;
} SnapResult;

SnapResult editor_snap_to_grid(
    Vec2 world_position,
    double spacing
);

#endif