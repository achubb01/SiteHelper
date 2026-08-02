#include "wall_render.h"
#include "wall_selection.h"


static Colour timber_render_colour(
    const Timber *timber,
    const WallSelection *selection,
    const WallRenderStyle *style
)
{
    if (wall_selection_contains(
        selection,
        timber)) {

        return style->selected_colour;
    }

    return style->timber_colour;
}


static void draw_vertical_timber(
    Renderer2D *renderer,
    const Timber *timber,
    const WallSelection *selection,
    const WallRenderStyle *style
)
{
    Colour colour =
        timber_render_colour(
            timber,
            selection,
            style
        );

    Rect2 rect = {
        .position = {
            .x = (double)timber->position.x,
            .y = (double)timber->position.y
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
    const WallSelection *selection,
    const WallRenderStyle *style
)
{
    Colour colour =
        timber_render_colour(
            timber,
            selection,
            style
        );

    Rect2 rect = {
        .position = {
            .x = (double)timber->position.x,
            .y = (double)timber->position.y
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
    const WallSelection *selection,
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
                selection,
                style
            );

        } else {

            draw_horizontal_timber(
                renderer,
                &timbers[i],
                selection,
                style
            );
        }
    }
}


void wall_render(
    Renderer2D *renderer,
    const Wall *wall,
    const WallSelection *selection,
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
        &wall->bottomplate,
        selection,
        style
    );

    draw_horizontal_timber(
        renderer,
        &wall->topplate,
        selection,
        style
    );

    draw_timber_array(
        renderer,
        wall->studs,
        wall->stud_count,
        true,
        selection,
        style
    );

    draw_timber_array(
        renderer,
        wall->nogs,
        wall->nog_count,
        false,
        selection,
        style
    );

    draw_timber_array(
        renderer,
        wall->members,
        wall->member_count,
        false,
        selection,
        style
    );
}