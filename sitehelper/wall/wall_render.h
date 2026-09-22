#ifndef WALL_RENDER_H
#define WALL_RENDER_H

#include <stdbool.h>

#include "renderer2d.h"
#include "wall.h"
#include "wall_selection.h"

typedef struct {
    /* Fallback framing colour. A semantic role colour with alpha == 0 inherits
     * this value, keeping partial/legacy style initialisers useful. */
    Colour timber_colour;
    Colour bottom_plate_colour;
    Colour top_plate_colour;
    Colour common_stud_colour;
    Colour king_stud_colour;
    Colour trimmer_stud_colour;
    Colour cripple_stud_colour;
    Colour noggin_colour;
    Colour header_colour;
    Colour sill_colour;

    Colour wall_extent_colour;
    Colour opening_colour;
    Colour annotation_colour;
    Colour dimension_colour;
    Colour selected_colour;
    Colour hovered_colour;
    Colour debug_axis_colour;

    bool show_wall_extent;
    bool show_openings;
    bool show_wall_identity;
    bool show_opening_labels;
    bool show_selected_member_label;
    bool show_dimensions;
    bool show_local_axes;
} WallRenderStyle;

typedef struct {
    /* Ephemeral pointers resolved from editor by-value WallSelection values. */
    const Timber *member;
    const Timber *hovered_member;
    DomainId opening_id;
    DomainId hovered_opening_id;
    bool wall_selected;
} WallElevationRenderSelection;

typedef struct {
    Colour body_colour;
    Colour datum_colour;
    bool show_datum;
} WallPlanRenderStyle;

/* Derived physical Plan body; datum is shown only as an editor affordance. */
void wall_plan_render(Renderer2D *renderer, const Wall *wall,
    const WallPlanRenderStyle *style);

/* Generated framing in local U/Z; placement is exclusively the camera's job.
 * The renderer consumes wall_elevation_presentation rather than interpreting
 * WallFraming storage arrays directly. Effective BuildSettings are optional:
 * when absent, semantic members still render but wall/opening context is
 * skipped. Selection only changes presentation and never becomes authority. */
void wall_elevation_render(
    Renderer2D *renderer,
    const Wall *wall,
    const BuildSettings *settings,
    const WallElevationRenderSelection *selection,
    const WallRenderStyle *style
);

#endif
