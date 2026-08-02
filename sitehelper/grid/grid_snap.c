#include <math.h>

#include "grid_snap.h"

Vec2 grid_snap_position(
    Vec2 position,
    double spacing
)
{
    if (spacing <= 0.0) {
        return position;
    }

    return (Vec2){
        .x =
            round(position.x / spacing)
            * spacing,

        .y =
            round(position.y / spacing)
            * spacing
    };
}