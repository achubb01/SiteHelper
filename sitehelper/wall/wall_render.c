#include "wall_render.h"

/*
 * Temporary elevation layout compatibility: segment start X/Y is reused as a
 * drawing offset for local U/Z before camera projection. These Rect2 positions
 * are render coordinates, not physical plan positions (plan Y is not height).
 */

static Colour timber_render_colour(
    const Timber *timber,
    const Timber *selected,
    const WallRenderStyle *style
)
{
    if (timber == selected) {
        return style->selected_colour;
    }

    return style->timber_colour;
}


static void draw_vertical_timber(
    Renderer2D *renderer,
    const Wall *wall,
    const Timber *timber,
    const Timber *selected,
    const WallRenderStyle *style
)
{
    Colour colour =
        timber_render_colour(
            timber,
            selected,
            style
        );

    Rect2 rect = {
        .position = {
            .x = (double)wall->definition.segment.start.x + timber->position.u,
            .y = (double)wall->definition.segment.start.y + timber->position.z
        },

        .width =
            (double)timber->width,

        .height =
            (double)timber->length
    };

    renderer2d_fill_rect(
        renderer,
        rect,
        colour
    );
}


static void draw_horizontal_timber(
    Renderer2D *renderer,
    const Wall *wall,
    const Timber *timber,
    const Timber *selected,
    const WallRenderStyle *style
)
{
    Colour colour =
        timber_render_colour(
            timber,
            selected,
            style
        );

    Rect2 rect = {
        .position = {
            .x = (double)wall->definition.segment.start.x + timber->position.u,
            .y = (double)wall->definition.segment.start.y + timber->position.z
        },

        .width =
            (double)timber->length,

        .height =
            (double)timber->width
    };

    renderer2d_fill_rect(
        renderer,
        rect,
        colour
    );
}


static void draw_timber_array(
    Renderer2D *renderer,
    const Wall *wall,
    const Timber *timbers,
    size_t count,
    bool vertical,
    const Timber *selected,
    const WallRenderStyle *style
)
{
    if (timbers == NULL) {
        return;
    }

    for (size_t i = 0;
         i < count;
         i++) {

        if (vertical) {

            draw_vertical_timber(
                renderer,
                wall,
                &timbers[i],
                selected,
                style
            );

        } else {

            draw_horizontal_timber(
                renderer,
                wall,
                &timbers[i],
                selected,
                style
            );
        }
    }
}


void wall_render(
    Renderer2D *renderer,
    const Wall *wall,
    const Timber *selected,
    const WallRenderStyle *style
)
{
    if (renderer == NULL ||
        wall == NULL ||
        style == NULL) {

        return;
    }

    draw_horizontal_timber(
        renderer,
        wall,
        &wall->framing.bottomplate,
        selected,
        style
    );

    draw_horizontal_timber(
        renderer,
        wall,
        &wall->framing.topplate,
        selected,
        style
    );

    draw_timber_array(
        renderer,
        wall,
        wall->framing.studs,
        wall->framing.stud_count,
        true,
        selected,
        style
    );

    draw_timber_array(
        renderer,
        wall,
        wall->framing.nogs,
        wall->framing.nog_count,
        false,
        selected,
        style
    );

    draw_timber_array(
        renderer,
        wall,
        wall->framing.members,
        wall->framing.member_count,
        false,
        selected,
        style
    );
}
