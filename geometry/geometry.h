#ifndef GEOMETRY_H
#define GEOMETRY_H

typedef struct Vec2 {
    double x;
    double y;
} Vec2;

typedef struct Camera2D {
    Vec2 position;
    double scale;
} Camera2D;

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