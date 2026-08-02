#include <math.h>
#include <stdlib.h>

#include "grid_render.h"

void grid_render(
    Renderer2D *renderer,
    const GridRenderStyle *style
)
{
    if (
        renderer == NULL || style == NULL
    ) {
        return;
    }   

    if (style->minimum_screen_spacing <= 0.0) {
        return;
    }

    Camera2D camera =
        renderer2d_get_camera(
            renderer
        );

    Viewport2D viewport =
        renderer2d_get_viewport(
            renderer
        );

    Vec2 world_top_left =
        camera_screen_to_world(
            &camera,
            viewport,
            (Vec2){0.0, 0.0}
        );

    Vec2 world_bottom_right =
        camera_screen_to_world(
            &camera,
            viewport,
            (Vec2){
                viewport.width,
                viewport.height
            }
        );

    double grid_spacing =
        grid_choose_spacing(
            camera.scale,
            style->minimum_screen_spacing
        );
    double major_grid_spacing =
        grid_spacing * 10.0;

    double world_left = world_top_left.x;
    double world_right = world_bottom_right.x;

    double world_bottom = world_bottom_right.y;
    double world_top = world_top_left.y;

    double first_x =
        floor(world_left / grid_spacing)
        * grid_spacing;

    double first_y =
        floor(world_bottom / grid_spacing)
        * grid_spacing;

    for (
        double x = first_x;
        x <= world_right;
        x += grid_spacing
    ) {
        long long world_x =
            (long long)x;

        long long major_spacing =
            (long long)major_grid_spacing;

        Colour line_colour;

        if (world_x == 0) {
            line_colour = style->axis_colour;
        }
        else if (
            world_x % major_spacing == 0
        ) {
            line_colour = style->major_colour;
        }
        else {
            line_colour = style->minor_colour;
        }

        renderer2d_draw_line(
            renderer,
            (Vec2){
                .x = x,
                .y = world_bottom
            },
            (Vec2){
                .x = x,
                .y = world_top
            },
            line_colour
        );
    }

    for (
        double y = first_y;
        y <= world_top;
        y += grid_spacing
    ) {
        long long world_y =
            (long long)y;

        long long major_spacing =
            (long long)major_grid_spacing;

        Colour line_colour;

        if (world_y == 0) {
            line_colour = style->axis_colour;
        }
        else if (
            world_y % major_spacing == 0
        ) {
            line_colour = style->major_colour;
        }
        else {
            line_colour = style->minor_colour;
        }

        renderer2d_draw_line(
            renderer,
            (Vec2){
                .x = world_left,
                .y = y
            },
            (Vec2){
                .x = world_right,
                .y = y
            },
            line_colour
        );
    }
}

double grid_choose_spacing(
    double camera_scale,
    double minimum_screen_spacing
)
{
    if (
        camera_scale <= 0.0
        || minimum_screen_spacing <= 0.0
    ) {
        return 100.0;
    }

    double spacing = 100.0;

    while (
        spacing * camera_scale
        < minimum_screen_spacing
    ) {
        if (spacing == 100.0) {
            spacing = 500.0;
        }
        else if (spacing == 500.0) {
            spacing = 1000.0;
        }
        else {
            spacing *= 5.0;
        }
    }

    return spacing;
}