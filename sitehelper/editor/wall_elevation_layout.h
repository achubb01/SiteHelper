#ifndef WALL_ELEVATION_LAYOUT_H
#define WALL_ELEVATION_LAYOUT_H

#include "wall.h"
#include "geometry.h"

/*
 * Convert a point in the temporary shared wall-elevation drawing layout,
 * before camera projection, into wall-local U/Z coordinates.
 * The layout reuses segment start X/Y as drawing offsets for U/Z;
 * this is not a physical plan-to-elevation transform. NULL wall means no offset.
 * Preserve existing input quantization: truncate layout coordinates to integer
 * millimetres before subtracting the offset.
 */
WallLocalPosition wall_elevation_layout_to_local_position(
    const Wall *wall,
    Vec2 layout_position
);

#endif
