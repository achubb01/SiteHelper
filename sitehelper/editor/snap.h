#ifndef SNAP_H
#define SNAP_H

#include <stddef.h>

#include "geometry.h"

typedef enum
{
    SNAP_NONE,
    SNAP_GRID,
    SNAP_STUD,
    SNAP_ENDPOINT,
    SNAP_OPENING,
    SNAP_INTERSECTION
} SnapType;

typedef struct
{
    Vec2 position;
    SnapType type;
} SnapCandidate;

typedef struct
{
    int grid_enabled;
    double grid_spacing;

    int endpoint_enabled;
    int intersection_enabled;

    double object_snap_tolerance;
} SnapSettings;

typedef struct
{
    Vec2 position;
    SnapType type;
} SnapResult;

SnapResult editor_snap(
    Vec2 world_position,
    const SnapCandidate *candidates,
    size_t candidate_count,
    const SnapSettings *settings
);

#endif