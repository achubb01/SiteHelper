#ifndef SNAP_H
#define SNAP_H

#include "geometry.h"
#include "wall.h"

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
    int grid_enabled;
    double grid_spacing;

    int endpoint_enabled;
    double object_snap_tolerance;
} SnapSettings;

typedef struct
{
    Vec2 position;
    SnapType type;
} SnapResult;

SnapResult editor_snap(
    Vec2 world_position,
    const Wall *wall,
    const SnapSettings *settings
);

#endif