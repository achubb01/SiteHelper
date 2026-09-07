#include "wall_render.h"

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
            .x = (double)timber->position.u,
            .y = (double)timber->position.z
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
            .x = (double)timber->position.u,
            .y = (double)timber->position.z
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
                &timbers[i],
                selected,
                style
            );

        } else {

            draw_horizontal_timber(
                renderer,
                &timbers[i],
                selected,
                style
            );
        }
    }
}


void wall_elevation_render(
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
        &wall->framing.bottomplate,
        selected,
        style
    );

    draw_horizontal_timber(
        renderer,
        &wall->framing.topplate,
        selected,
        style
    );

    draw_timber_array(
        renderer,
        wall->framing.studs,
        wall->framing.stud_count,
        true,
        selected,
        style
    );

    draw_timber_array(
        renderer,
        wall->framing.nogs,
        wall->framing.nog_count,
        false,
        selected,
        style
    );

    draw_timber_array(
        renderer,
        wall->framing.members,
        wall->framing.member_count,
        false,
        selected,
        style
    );
}

void wall_plan_render(
    Renderer2D *renderer,
    const Wall *wall,
    Colour colour
)
{
    if (renderer == NULL || wall == NULL) {
        return;
    }
    WallPlanSegment segment = wall->definition.segment;
    renderer2d_draw_line(renderer,
        (Vec2){segment.start.x, segment.start.y},
        (Vec2){segment.end.x, segment.end.y}, colour);
}
