#include <stdlib.h>

#include "wall.h"
#include "wall_internal.h"

static int stud_overlaps_range(const Timber *stud, int range_start, int range_end, const BuildSettings *settings);
static int wall_frame_door(Wall *wall, const BuildSettings *settings, const Opening *opening, int left_trimmer_position, int right_trimmer_position);
static int wall_frame_window(Wall *wall, const BuildSettings *settings, const Opening *opening, int left_trimmer_position, int right_trimmer_position);
static int wall_generate_lower_cripples(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int wall_generate_upper_cripples(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int opening_assembly_start(const Opening *opening, const BuildSettings *settings);
static int opening_assembly_end(const Opening *opening, const BuildSettings *settings);
static int openings_conflict(const Opening *a, const Opening *b, const BuildSettings *settings);
static int wall_span_is_opening(const Wall *wall, const BuildSettings *settings, const Timber *left, const Timber *right);

int opening_frame_width(
    const Opening *opening,
    const BuildSettings *settings
)
{
    if (opening == NULL ||
        settings == NULL) {
        return 0;
    }

    int allowance =
        opening->custom_allowance
            ? opening->width_allowance
            : settings->opening_width_allowance;

    return opening->width + allowance;
}

int opening_frame_height(
    const Opening *opening,
    const BuildSettings *settings
)
{
    if (opening == NULL ||
        settings == NULL) {
        return 0;
    }

    int allowance =
        opening->custom_allowance
            ? opening->height_allowance
            : settings->opening_height_allowance;

    return opening->height + allowance;
}

int wall_add_opening(
    Wall *wall,
    const BuildSettings *settings,
    OpeningType type,
    int frame_position,
    int frame_bottom,
    int width,
    int height
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (type != OPENING_DOOR &&
        type != OPENING_WINDOW) {
        return 0;
    }

    if (frame_position < 0 ||
        frame_bottom < 0 ||
        width <= 0 ||
        height <= 0) {
        return 0;
    }

    Opening opening = {
        .type = type,
        .frame_position = frame_position,
        .frame_bottom = frame_bottom,
        .width = width,
        .height = height,
        .width_allowance = 0,
        .height_allowance = 0,
        .custom_allowance = false
    };

    if (!wall_opening_fits(
            wall,
            &opening,
            settings)) {
        return 0;
    }

    for (size_t i = 0;
        i < wall->opening_count;
        i++) {

        if (openings_conflict(
                &opening,
                &wall->openings[i],
                settings)) {

            return 0;
        }
    }

    if (wall->opening_count ==
        wall->opening_capacity) {

        size_t new_capacity =
            wall->opening_capacity == 0
                ? 1
                : wall->opening_capacity * 2;

        Opening *new_openings = realloc(
            wall->openings,
            new_capacity * sizeof *new_openings
        );

        if (new_openings == NULL) {
            return 0;
        }

        wall->openings = new_openings;
        wall->opening_capacity =
            new_capacity;
    }

    wall->openings[
        wall->opening_count
    ] = opening;

    wall->opening_count++;

    return 1;
}

int wall_opening_fits(
    const Wall *wall,
    const Opening *opening,
    const BuildSettings *settings
)
{
    if (wall == NULL ||
        opening == NULL ||
        settings == NULL) {
        return 0;
    }

    int frame_width =
        opening_frame_width(
            opening,
            settings
        );

    int frame_height =
        opening_frame_height(
            opening,
            settings
        );

    if (frame_width <= 0 ||
        frame_height <= 0) {
        return 0;
    }

    /*
     * Clear framed opening must fit
     * vertically.
     */
    if (opening->frame_bottom < 0 ||
        opening->frame_bottom +
        frame_height >
        settings->stud_height) {

        return 0;
    }

    /*
     * Entire opening assembly:
     *
     * KING | TRIMMER | OPENING |
     * TRIMMER | KING
     */
    int assembly_start =
        opening_assembly_start(
            opening,
            settings
        );

    int assembly_end =
        opening_assembly_end(
            opening,
            settings
        );

    /*
     * Preserve left wall-end stud.
     */
    if (assembly_start <
        settings->stud_width) {

        return 0;
    }

    /*
     * Preserve right wall-end stud.
     */
    int right_end_stud =
        wall->bottomplate.length -
        settings->stud_width;

    if (assembly_end >
        right_end_stud) {

        return 0;
    }

    return 1;
}

int wall_apply_openings(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    for (size_t opening_index = 0;
         opening_index < wall->opening_count;
         opening_index++) {

        Opening *opening =
            &wall->openings[opening_index];

        /*
         * Clear framed opening boundaries.
         */
        int opening_start =
            opening->frame_position;

        int opening_end =
            opening_start +
            opening_frame_width(
                opening,
                settings
            );

        /*
         * Trimmers sit immediately beside
         * the clear framed opening.
         */
        int left_trimmer_position =
            opening_start -
            settings->stud_width;

        int right_trimmer_position =
            opening_end;

        /*
         * Kings sit immediately outside
         * the trimmers.
         */
        int left_king_position =
            left_trimmer_position -
            settings->stud_width;

        int right_king_position =
            right_trimmer_position +
            settings->stud_width;

        /*
         * Entire opening assembly must fit
         * inside the wall.
         */
        if (left_king_position < 0) {
            return 0;
        }

        if (right_king_position +
            settings->stud_width >
            wall->bottomplate.length) {

            return 0;
        }

        /*
         * Remove generated common studs
         * that interfere with the opening.
         */
        size_t stud_index = 0;

        int assembly_start =
            left_king_position;

        int assembly_end =
            right_king_position +
            settings->stud_width;

        while (stud_index < wall->stud_count) {

            Timber *stud =
                &wall->studs[stud_index];

            if (stud->details.stud.type == STUD_COMMON &&
                stud_overlaps_range(
                    stud,
                    assembly_start,
                    assembly_end,
                    settings)) {

                wall_remove_stud(
                    wall,
                    stud_index
                );

            } else {
                stud_index++;
            }
        }

        /*
         * LEFT KING
         *
         * Reuse an existing stud if one
         * already happens to be exactly
         * where we need it.
         */
        if (!wall_add_stud(
                wall,
                settings,
                left_king_position,
                STUD_KING)) {
            return 0;
        }

        /*RIGHT KING*/

        if (!wall_add_stud(
                wall,
                settings,
                right_king_position,
                STUD_KING)) {
            return 0;
        }

        /*
         * Trimmer length.
         *
         * For now we're implementing the
         * straightforward door case.
         */
       switch (opening->type) {

            case OPENING_DOOR:

                if (!wall_frame_door(
                        wall,
                        settings,
                        opening,
                        left_trimmer_position,
                        right_trimmer_position)) {
                    return 0;
                }

                break;

            case OPENING_WINDOW:

                if (!wall_frame_window(
                        wall,
                        settings,
                        opening,
                        left_trimmer_position,
                        right_trimmer_position)) {
                    return 0;
                }

                break;

            default:
                return 0;
        }
    }
    

    /*
     * Adding opening members appends them
     * to the array, so restore physical
     * left-to-right ordering.
     */
    qsort(
        wall->studs,
        wall->stud_count,
        sizeof *wall->studs,
        wall_compare_stud_position
    );

    return 1;
}

static int stud_overlaps_range(
    const Timber *stud,
    int range_start,
    int range_end,
    const BuildSettings *settings
)
{
    if (stud == NULL || settings == NULL) {
        return 0;
    }

    int stud_start =
        stud->position.x;

    int stud_end =
        stud_start +
        settings->stud_width;

    return
        stud_start < range_end &&
        stud_end > range_start;
}












static int wall_frame_door(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening,
    int left_trimmer_position,
    int right_trimmer_position
)
{
    if (wall == NULL ||
        settings == NULL ||
        opening == NULL) {
        return 0;
    }

    int trimmer_length =
        opening_frame_height(
            opening,
            settings
        );

    if (trimmer_length <= 0) {
        return 0;
    }

    if (!wall_add_custom_stud(
            wall,
            settings,
            left_trimmer_position,
            0,
            trimmer_length,
            STUD_TRIMMER)) {
        return 0;
    }

    if (!wall_add_custom_stud(
            wall,
            settings,
            right_trimmer_position,
            0,
            trimmer_length,
            STUD_TRIMMER)) {
        return 0;
    }

    return 1;
}

static int wall_frame_window(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening,
    int left_trimmer_position,
    int right_trimmer_position
)
{
    if (wall == NULL ||
        settings == NULL ||
        opening == NULL) {
        return 0;
    }

    if (!wall_add_header(
            wall,
            settings,
            opening)) {
        return 0;
    }

    if (!wall_add_sill(
            wall,
            settings,
            opening)) {
        return 0;
    }

    /*
     * Window trimmers run from the bottom
     * framing reference up to the underside
     * of the header.
     */
    int trimmer_length =
        opening->frame_bottom +
        opening_frame_height(
            opening,
            settings
        );

    if (trimmer_length <= 0) {
        return 0;
    }

    if (!wall_add_custom_stud(
            wall,
            settings,
            left_trimmer_position,
            0,
            trimmer_length,
            STUD_TRIMMER)) {
        return 0;
    }

    if (!wall_add_custom_stud(
            wall,
            settings,
            right_trimmer_position,
            0,
            trimmer_length,
            STUD_TRIMMER)) {
        return 0;
    }

    if (!wall_generate_lower_cripples(
            wall,
            settings,
            opening)) {
        return 0;
    }

    if (!wall_generate_upper_cripples(
        wall, 
        settings,
        opening)) {
            return 0;
        }

    return 1;
}

static int wall_generate_lower_cripples(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
)
{
    if (wall == NULL ||
        settings == NULL ||
        opening == NULL) {
        return 0;
    }

    int length =
        opening->frame_bottom;

    if (length <= 0) {
        return 0;
    }

    int frame_width =
        opening_frame_width(
            opening,
            settings
        );

    if (frame_width <= 0) {
        return 0;
    }

    int start =
        opening->frame_position;

    int end =
        start +
        frame_width -
        settings->stud_width;

    if (end < start) {
        return 0;
    }

    StudGenerationContext context = {
        .wall = wall,
        .settings = settings,
        .length = length,
        .type = STUD_CRIPPLE
    };

    return wall_generate_positions(
        start,
        end,
        settings,
        wall_add_stud_at_position,
        &context
    );
}

static int wall_generate_upper_cripples(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
)
{
    if (wall == NULL ||
        settings == NULL ||
        opening == NULL) {
        return 0;
    }

    if (opening->type != OPENING_WINDOW) {
        return 0;
    }

    int frame_width =
        opening_frame_width(
            opening,
            settings
        );

    int frame_height =
        opening_frame_height(
            opening,
            settings
        );

    if (frame_width <= 0 ||
        frame_height <= 0) {
        return 0;
    }

    /*
     * Header position.y is its bottom face.
     */
    int header_y =
        opening->frame_bottom +
        frame_height;

    /*
     * Upper cripples begin on top
     * of the header.
     */
    int cripple_y =
        header_y +
        settings->stud_width;

    /*
     * They continue to the upper
     * framing limit.
     */
    int cripple_length =
        settings->stud_height -
        cripple_y;

    if (cripple_length <= 0) {
        return 0;
    }

    /*
     * Same horizontal span as the
     * lower window cripples.
     */
    int start =
        opening->frame_position;

    int end =
        start +
        frame_width -
        settings->stud_width;

    if (end < start) {
        return 0;
    }

    StudGenerationContext context = {
        .wall = wall,
        .settings = settings,

        .y = cripple_y,
        .length = cripple_length,

        .type = STUD_CRIPPLE
    };

    return wall_generate_positions(
        start,
        end,
        settings,
        wall_add_stud_at_position,
        &context
    );
}

static int opening_assembly_start(
    const Opening *opening,
    const BuildSettings *settings
)
{
    return
        opening->frame_position -
        (2 * settings->stud_width);
}

static int opening_assembly_end(
    const Opening *opening,
    const BuildSettings *settings
)
{
    return
        opening->frame_position +
        opening_frame_width(
            opening,
            settings
        ) +
        (2 * settings->stud_width);
}

static int openings_conflict(
    const Opening *a,
    const Opening *b,
    const BuildSettings *settings
)
{
    if (a == NULL ||
        b == NULL ||
        settings == NULL) {
        return 0;
    }

    int a_start =
        opening_assembly_start(
            a,
            settings
        );

    int a_end =
        opening_assembly_end(
            a,
            settings
        );

    int b_start =
        opening_assembly_start(
            b,
            settings
        );

    int b_end =
        opening_assembly_end(
            b,
            settings
        );

    return
        a_start < b_end &&
        a_end > b_start;
}

int wall_repair_stud_spacing(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL ||
        settings == NULL) {
        return 0;
    }

    qsort(
        wall->studs,
        wall->stud_count,
        sizeof *wall->studs,
        wall_compare_stud_position
    );

    size_t i = 1;

    while (i < wall->stud_count) {

        Timber *left =
            &wall->studs[i - 1];

        Timber *right =
            &wall->studs[i];

        int spacing =
            right->position.x -
            left->position.x;

        /*
         * Same X or touching/closely packed
         * members require no repair.
         */
        if (spacing <=
            settings->stud_spacing) {

            i++;
            continue;
        }

        /*
         * A door/window opening is an
         * intentional large span.
         */
        if (wall_span_is_opening(
                wall,
                settings,
                left,
                right)) {

            i++;
            continue;
        }

        /*
         * Genuine oversized framing bay.
         *
         * Work out how many legal gaps we
         * need, then divide this span evenly.
         */
        int gaps =
            (spacing +
             settings->stud_spacing - 1)
            /
            settings->stud_spacing;

        /*
         * Add interior studs only.
         */
        for (int gap = 1;
             gap < gaps;
             gap++) {

            int position =
                left->position.x +
                (spacing * gap) / gaps;

            if (!wall_add_stud(
                    wall,
                    settings,
                    position,
                    STUD_COMMON)) {

                return 0;
            }
        }

        /*
         * Adding studs can realloc the array,
         * so our old left/right pointers may
         * now be invalid.
         *
         * Re-sort and restart the scan.
         */
        qsort(
            wall->studs,
            wall->stud_count,
            sizeof *wall->studs,
            wall_compare_stud_position
        );

        i = 1;
    }

    return 1;
}

static int wall_span_is_opening(
    const Wall *wall,
    const BuildSettings *settings,
    const Timber *left,
    const Timber *right
)
{
    if (wall == NULL ||
        settings == NULL ||
        left == NULL ||
        right == NULL) {
        return 0;
    }

    /*
     * Clear bay between the two vertical
     * members.
     */
    int span_left =
        left->position.x +
        settings->stud_width;

    int span_right =
        right->position.x;

    if (span_right <= span_left) {
        return 0;
    }

    for (size_t i = 0;
         i < wall->opening_count;
         i++) {

        const Opening *opening =
            &wall->openings[i];

        int opening_left =
            opening->frame_position;

        int opening_right =
            opening_left +
            opening_frame_width(
                opening,
                settings
            );

        /*
         * This span is intentional only if
         * the clear faces exactly match the
         * framed opening.
         */
        if (span_left == opening_left &&
            span_right == opening_right) {

            return 1;
        }
    }

    return 0;
}

int wall_add_stud_at_position(
    int position,
    void *context
)
{
    StudGenerationContext *generation =
        context;

    return wall_add_custom_stud(
        generation->wall,
        generation->settings,
        position,
        generation->y,
        generation->length,
        generation->type
    );
}