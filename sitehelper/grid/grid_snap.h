#ifndef GRID_SNAP_H
#define GRID_SNAP_H

#include "geometry.h"

/* Position and spacing share the caller's geometry space/unit; the result
 * remains in that space. SiteHelper supplies millimetres after unprojection. */
Vec2 grid_snap_position(
    Vec2 position,
    double spacing
);

#endif