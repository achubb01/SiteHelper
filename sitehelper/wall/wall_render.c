#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#include "wall_render.h"
#include "wall_elevation_presentation.h"
#include "wall_plan_geometry.h"

static int colour_is_inherit(Colour colour)
{
    return colour.a == 0;
}

static Colour colour_or_fallback(Colour colour, Colour fallback)
{
    return colour_is_inherit(colour) ? fallback : colour;
}

static Colour semantic_colour(Colour role_colour, const WallRenderStyle *style)
{
    return colour_or_fallback(role_colour, style->timber_colour);
}

static Colour annotation_colour(const WallRenderStyle *style)
{
    return colour_or_fallback(style->annotation_colour, style->timber_colour);
}

static Colour dimension_colour(const WallRenderStyle *style)
{
    return colour_or_fallback(style->dimension_colour, annotation_colour(style));
}

static Colour hovered_colour(const WallRenderStyle *style)
{
    return colour_or_fallback(style->hovered_colour, annotation_colour(style));
}

static Colour debug_axis_colour(const WallRenderStyle *style)
{
    return colour_or_fallback(style->debug_axis_colour, annotation_colour(style));
}

static Colour member_role_colour(
    WallElevationMemberRole role,
    const WallRenderStyle *style
)
{
    switch (role) {
        case WALL_ELEVATION_MEMBER_ROLE_BOTTOM_PLATE:
            return semantic_colour(style->bottom_plate_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_TOP_PLATE:
            return semantic_colour(style->top_plate_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_COMMON_STUD:
            return semantic_colour(style->common_stud_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_KING_STUD:
            return semantic_colour(style->king_stud_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_TRIMMER_STUD:
            return semantic_colour(style->trimmer_stud_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_CRIPPLE_STUD:
            return semantic_colour(style->cripple_stud_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_NOGGIN:
            return semantic_colour(style->noggin_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_HEADER:
            return semantic_colour(style->header_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_SILL:
            return semantic_colour(style->sill_colour, style);
        case WALL_ELEVATION_MEMBER_ROLE_UNKNOWN:
        default:
            return style->timber_colour;
    }
}

static Colour member_render_colour(
    const WallElevationMember *member,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (member != NULL && selection != NULL &&
        member->source == selection->member) {
        return style->selected_colour;
    }

    return member != NULL ? member_role_colour(member->role, style) :
        style->timber_colour;
}

static Rect2 render_rect(WallElevationRect bounds)
{
    return (Rect2){
        .position = {
            .x = (double)bounds.u,
            .y = (double)bounds.z
        },
        .width = (double)bounds.width,
        .height = (double)bounds.height
    };
}


static void draw_text_at_world(
    Renderer2D *renderer,
    Vec2 world,
    Vec2 screen_offset,
    const char *text,
    Colour colour
)
{
    if (renderer == NULL || text == NULL) {
        return;
    }

    Camera2D camera = renderer2d_get_camera(renderer);
    Viewport2D viewport = renderer2d_get_viewport(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) {
        return;
    }

    Vec2 screen = camera_world_to_screen(&camera, viewport, world);
    if (!isfinite(screen.x) || !isfinite(screen.y)) {
        return;
    }

    screen.x += screen_offset.x;
    screen.y += screen_offset.y;
    renderer2d_draw_screen_text(renderer, screen, text, colour);
}

static void draw_wall_identity(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (!style->show_wall_identity || settings == NULL) {
        return;
    }

    WallElevationRect bounds;
    if (!wall_elevation_presentation_bounds(wall, settings, &bounds)) {
        return;
    }

    Colour colour = annotation_colour(style);
    if (selection != NULL && selection->wall_selected) {
        colour = style->selected_colour;
    }

    char text[64];
    snprintf(text, sizeof text, "Wall %" PRIu64, (uint64_t)wall->id);
    draw_text_at_world(renderer,
        (Vec2){bounds.u, (double)bounds.z + bounds.height},
        (Vec2){4.0, -14.0}, text, colour);
}

static void draw_opening_labels(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (!style->show_opening_labels || settings == NULL) {
        return;
    }

    size_t count = wall_elevation_presentation_opening_count(wall);
    for (size_t i = 0; i < count; i++) {
        WallElevationOpening opening;
        if (!wall_elevation_presentation_opening_at(
                wall, settings, i, &opening)) {
            continue;
        }

        int selected = selection != NULL &&
            selection->opening_id != DOMAIN_ID_INVALID &&
            selection->opening_id == opening.opening_id;
        int hovered = selection != NULL && !selected &&
            selection->hovered_opening_id != DOMAIN_ID_INVALID &&
            selection->hovered_opening_id == opening.opening_id;
        Colour colour = selected ? style->selected_colour :
            (hovered ? hovered_colour(style) : annotation_colour(style));
        Vec2 anchor = {
            (double)opening.clear_bounds.u,
            (double)opening.clear_bounds.z + opening.clear_bounds.height
        };

        char text[64];
        snprintf(text, sizeof text, "%s %" PRIu64,
            wall_elevation_opening_type_name(opening.type),
            (uint64_t)opening.opening_id);
        draw_text_at_world(renderer, anchor, (Vec2){4.0, 4.0}, text, colour);

        if (selected) {
            snprintf(text, sizeof text, "Editable | %d x %d mm",
                opening.clear_bounds.width, opening.clear_bounds.height);
            draw_text_at_world(renderer, anchor, (Vec2){4.0, 14.0}, text, colour);
        }
    }
}

static void draw_selected_member_label(
    Renderer2D *renderer,
    const Wall *wall,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (!style->show_selected_member_label || selection == NULL ||
        selection->member == NULL) {
        return;
    }

    size_t count = wall_elevation_presentation_member_count(wall);
    for (size_t i = 0; i < count; i++) {
        WallElevationMember member;
        if (!wall_elevation_presentation_member_at(wall, i, &member) ||
            member.source != selection->member) {
            continue;
        }

        char text[128];
        snprintf(text, sizeof text, "Generated | %s | %dx%d | L %d",
            wall_elevation_member_role_name(member.role),
            member.source->depth, member.source->width, member.source->length);
        draw_text_at_world(renderer,
            (Vec2){
                (double)member.bounds.u + member.bounds.width,
                (double)member.bounds.z + member.bounds.height
            },
            (Vec2){6.0, -14.0}, text, style->selected_colour);
        return;
    }
}

static void draw_horizontal_dimension(
    Renderer2D *renderer,
    const WallElevationRect *bounds,
    double offset,
    double source_gap,
    double tick,
    Colour colour
)
{
    double u0 = bounds->u;
    double u1 = (double)bounds->u + bounds->width;
    double z0 = bounds->z;
    double z_dimension = z0 - offset;

    renderer2d_draw_line(renderer, (Vec2){u0, z_dimension},
        (Vec2){u1, z_dimension}, colour);
    renderer2d_draw_line(renderer, (Vec2){u0, z0 - source_gap},
        (Vec2){u0, z_dimension - tick}, colour);
    renderer2d_draw_line(renderer, (Vec2){u1, z0 - source_gap},
        (Vec2){u1, z_dimension - tick}, colour);
    renderer2d_draw_line(renderer, (Vec2){u0, z_dimension - tick},
        (Vec2){u0, z_dimension + tick}, colour);
    renderer2d_draw_line(renderer, (Vec2){u1, z_dimension - tick},
        (Vec2){u1, z_dimension + tick}, colour);

    char text[64];
    snprintf(text, sizeof text, "%d mm", bounds->width);
    draw_text_at_world(renderer, (Vec2){(u0 + u1) * 0.5, z_dimension},
        (Vec2){4.0, -12.0}, text, colour);
}

static void draw_vertical_dimension(
    Renderer2D *renderer,
    const WallElevationRect *bounds,
    double offset,
    double source_gap,
    double tick,
    Colour colour
)
{
    double z0 = bounds->z;
    double z1 = (double)bounds->z + bounds->height;
    double u1 = (double)bounds->u + bounds->width;
    double u_dimension = u1 + offset;

    renderer2d_draw_line(renderer, (Vec2){u_dimension, z0},
        (Vec2){u_dimension, z1}, colour);
    renderer2d_draw_line(renderer, (Vec2){u1 + source_gap, z0},
        (Vec2){u_dimension + tick, z0}, colour);
    renderer2d_draw_line(renderer, (Vec2){u1 + source_gap, z1},
        (Vec2){u_dimension + tick, z1}, colour);
    renderer2d_draw_line(renderer, (Vec2){u_dimension - tick, z0},
        (Vec2){u_dimension + tick, z0}, colour);
    renderer2d_draw_line(renderer, (Vec2){u_dimension - tick, z1},
        (Vec2){u_dimension + tick, z1}, colour);

    char text[64];
    snprintf(text, sizeof text, "%d mm", bounds->height);
    draw_text_at_world(renderer, (Vec2){u_dimension, (z0 + z1) * 0.5},
        (Vec2){6.0, -12.0}, text, colour);
}

static void draw_dimensions(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallRenderStyle *style
)
{
    if (!style->show_dimensions || settings == NULL) {
        return;
    }

    WallElevationRect bounds;
    if (!wall_elevation_presentation_bounds(wall, settings, &bounds)) {
        return;
    }

    Camera2D camera = renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) {
        return;
    }

    /* Keep the annotation gap and terminal marks stable in screen pixels while
     * the measured geometry itself remains wall-local integer millimetres. */
    double offset = 28.0 / camera.scale;
    double source_gap = 5.0 / camera.scale;
    double tick = 4.0 / camera.scale;
    Colour colour = dimension_colour(style);
    draw_horizontal_dimension(renderer, &bounds, offset, source_gap, tick, colour);
    draw_vertical_dimension(renderer, &bounds, offset, source_gap, tick, colour);
}

static void draw_wall_extent(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (!style->show_wall_extent || settings == NULL) {
        return;
    }

    WallElevationRect bounds;
    if (!wall_elevation_presentation_bounds(wall, settings, &bounds)) {
        return;
    }

    Colour colour = semantic_colour(style->wall_extent_colour, style);
    if (selection != NULL && selection->wall_selected) {
        colour = style->selected_colour;
    }
    renderer2d_draw_rect(renderer, render_rect(bounds), colour);
}

static void draw_openings(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (!style->show_openings || settings == NULL) {
        return;
    }

    size_t count = wall_elevation_presentation_opening_count(wall);
    for (size_t i = 0; i < count; i++) {
        WallElevationOpening opening;
        if (!wall_elevation_presentation_opening_at(
                wall, settings, i, &opening)) {
            continue;
        }

        Colour colour = semantic_colour(style->opening_colour, style);
        if (selection != NULL &&
            selection->opening_id != DOMAIN_ID_INVALID &&
            selection->opening_id == opening.opening_id) {
            colour = style->selected_colour;
        } else if (selection != NULL &&
            selection->hovered_opening_id != DOMAIN_ID_INVALID &&
            selection->hovered_opening_id == opening.opening_id) {
            colour = hovered_colour(style);
        }
        renderer2d_draw_rect(renderer, render_rect(opening.clear_bounds), colour);
    }
}

static void draw_elevation_member(
    Renderer2D *renderer,
    const WallElevationMember *member,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (member == NULL) {
        return;
    }

    renderer2d_fill_rect(
        renderer,
        render_rect(member->bounds),
        member_render_colour(member, selection, style)
    );
    if (selection != NULL && member->source == selection->hovered_member &&
        member->source != selection->member) {
        renderer2d_draw_rect(renderer, render_rect(member->bounds),
            hovered_colour(style));
    }
}

static void draw_local_axes(
    Renderer2D *renderer, const Wall *wall, const BuildSettings *settings,
    const WallRenderStyle *style)
{
    if (!style->show_local_axes || settings == NULL) { return; }
    WallElevationRect bounds;
    if (!wall_elevation_presentation_bounds(wall, settings, &bounds)) { return; }
    Camera2D camera = renderer2d_get_camera(renderer);
    if (!(camera.scale > 0.0) || !isfinite(camera.scale)) { return; }

    /* Debug-only local basis. Keep it outside the physical wall and maintain a
     * readable fixed-pixel offset while axis length remains world geometry. */
    double inset = 18.0 / camera.scale;
    double length = bounds.width < 500 ? bounds.width : 500.0;
    if (bounds.height < length) { length = bounds.height; }
    if (length <= 0.0) { return; }
    Colour colour = debug_axis_colour(style);
    Vec2 origin = {(double)bounds.u - inset, (double)bounds.z - inset};
    renderer2d_draw_line(renderer, origin,
        (Vec2){origin.x + length, origin.y}, colour);
    renderer2d_draw_line(renderer, origin,
        (Vec2){origin.x, origin.y + length}, colour);
    draw_text_at_world(renderer, (Vec2){origin.x + length, origin.y},
        (Vec2){4.0, -10.0}, "U", colour);
    draw_text_at_world(renderer, (Vec2){origin.x, origin.y + length},
        (Vec2){4.0, -10.0}, "Z", colour);
}

void wall_elevation_render(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
)
{
    if (renderer == NULL || wall == NULL || style == NULL) {
        return;
    }

    /* Context is deliberately behind the framing so semantic members remain
     * the dominant visual layer. */
    draw_wall_extent(renderer, wall, settings, selection, style);
    draw_openings(renderer, wall, settings, selection, style);

    size_t count = wall_elevation_presentation_member_count(wall);
    for (size_t i = 0; i < count; i++) {
        WallElevationMember member;
        if (wall_elevation_presentation_member_at(wall, i, &member)) {
            draw_elevation_member(renderer, &member, selection, style);
        }
    }

    /* Identification, interaction hints and measurements are transient view
     * annotations. They never enter Wall or generated framing state. */
    draw_local_axes(renderer, wall, settings, style);
    draw_dimensions(renderer, wall, settings, style);
    draw_wall_identity(renderer, wall, settings, selection, style);
    draw_opening_labels(renderer, wall, settings, selection, style);
    draw_selected_member_label(renderer, wall, selection, style);
}

void wall_plan_render(
    Renderer2D *renderer, const Wall *wall, const WallPlanRenderStyle *style)
{
    if (renderer == NULL || wall == NULL || style == NULL) return;
    WallPlanGeometry geometry;
    if (!wall_plan_geometry_build(&wall->definition, &geometry)) return;
    renderer2d_fill_triangle(renderer,
        (Vec2){geometry.corners[0].x,geometry.corners[0].y},
        (Vec2){geometry.corners[1].x,geometry.corners[1].y},
        (Vec2){geometry.corners[2].x,geometry.corners[2].y}, style->body_colour);
    renderer2d_fill_triangle(renderer,
        (Vec2){geometry.corners[0].x,geometry.corners[0].y},
        (Vec2){geometry.corners[2].x,geometry.corners[2].y},
        (Vec2){geometry.corners[3].x,geometry.corners[3].y}, style->body_colour);
    if (style->show_datum) {
        WallPlanSegment segment=wall->definition.segment;
        renderer2d_draw_line(renderer, (Vec2){segment.start.x,segment.start.y},
            (Vec2){segment.end.x,segment.end.y}, style->datum_colour);
    }
}
