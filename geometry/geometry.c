#include <stdio.h>
#include "geometry.h"

Vec2 camera_world_to_screen(
    const Camera2D *camera,
    Viewport2D viewport,
    Vec2 world
)
{
    if (camera == NULL) {
        return (Vec2){0.0, 0.0};
    }

    return (Vec2){
        .x =
            viewport.position.x
            + (
                world.x
                - camera->position.x
            ) * camera->scale,

        .y =
            viewport.position.y
            + viewport.height
            - (
                world.y
                - camera->position.y
            ) * camera->scale
    };
}

Vec2 camera_screen_to_world(
    const Camera2D *camera,
    Viewport2D viewport,
    Vec2 screen
)
{
    if (
        camera == NULL
        || camera->scale <= 0.0
    ) {
        return (Vec2){0.0, 0.0};
    }

    return (Vec2){
        .x =
            camera->position.x
            + (
                screen.x
                - viewport.position.x
            ) / camera->scale,

        .y =
            camera->position.y
            + (
                viewport.height
                - (
                    screen.y
                    - viewport.position.y
                )
            ) / camera->scale
    };
}

Vec2 rect2_top_right(Rect2 rect) {
    Vec2 top_right = {
        .x = rect.position.x + rect.width,
        .y = rect.position.y + rect.height
    };

    return top_right;
}

int rect2_contains_point(Rect2 rect, Vec2 point) {
    Vec2 top_right = rect2_top_right(rect);

    return (
        point.x >= rect.position.x &&
        point.x <= top_right.x &&
        point.y >= rect.position.y &&
        point.y <= top_right.y
    );
}

Rect2 rect2_world_to_screen(
    const Camera2D *camera,
    Viewport2D viewport,
    Rect2 world_rect
)
{
    Vec2 world_top_left = {
        .x = world_rect.position.x,
        .y = world_rect.position.y + world_rect.height
    };

    Rect2 screen_rect = {
        .position = camera_world_to_screen(
            camera,
            viewport,
            world_top_left
        ),

        .width = world_rect.width * camera->scale,
        .height = world_rect.height * camera->scale
    };

    return screen_rect;
}