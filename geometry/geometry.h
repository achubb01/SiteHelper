#ifndef GEOMETRY_H
#define GEOMETRY_H

/* Vec2/Rect2 are generic caller-owned geometry: neither encodes a unit or
 * coordinate space. Arguments used together must share both. */
typedef struct Vec2 {
    double x;
    double y;
} Vec2;

/* Position uses caller world units; scale is screen pixels per world unit.
 * Camera transforms map world +Y up to screen +Y down. */
typedef struct Camera2D {
    Vec2 position;
    double scale;
} Camera2D;

/* Screen-space origin and extents in pixels, independent of world units. */
typedef struct Viewport2D {
    Vec2 position;

    double width;
    double height;
} Viewport2D;

typedef struct Rect2 {
    Vec2 position;
    double width;
    double height;
} Rect2;

Vec2 camera_world_to_screen(
    const Camera2D *camera,
    Viewport2D viewport,
    Vec2 world
);

Vec2 camera_screen_to_world(
    const Camera2D *camera,
    Viewport2D viewport,
    Vec2 screen
);

Vec2 rect2_top_right(
    Rect2 rect
);

int rect2_contains_point(
    Rect2 rect,
    Vec2 point
);

Rect2 rect2_world_to_screen(
    const Camera2D *camera,
    Viewport2D viewport,
    Rect2 world_rect
);

#endif