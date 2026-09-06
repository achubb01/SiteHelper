#include "wall_elevation_layout.h"

WallLocalPosition wall_elevation_layout_to_local_position(
    const Wall *wall,
    Vec2 layout_position
)
{
    WallLocalPosition local = {
        .u = (int)layout_position.x,
        .z = (int)layout_position.y
    };

    if (wall == NULL) {
        return local;
    }

    local.u -= wall->definition.segment.start.x;
    local.z -= wall->definition.segment.start.y;
    return local;
}
