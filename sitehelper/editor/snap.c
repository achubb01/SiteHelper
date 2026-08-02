#include "snap.h"

#include "grid_snap.h"

SnapResult editor_snap_to_grid(
    Vec2 world_position,
    double spacing
)
{
    if (spacing <= 0.0) {
        return (SnapResult){
            .position = world_position,
            .type = SNAP_NONE
        };
    }

    return (SnapResult){
        .position =
            grid_snap_position(
                world_position,
                spacing
            ),

        .type = SNAP_GRID
    };
}