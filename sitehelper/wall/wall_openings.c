#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "wall.h"
#include "wall_internal.h"

static int stud_overlaps_range(const Timber *stud, int range_start, int range_end, const BuildSettings *settings);
static int wall_frame_door(Wall *wall, const BuildSettings *settings, const Opening *opening, int left_trimmer_position, int right_trimmer_position);
static int wall_frame_window(Wall *wall, const BuildSettings *settings, const Opening *opening, int left_trimmer_position, int right_trimmer_position);
static int wall_generate_lower_cripples(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int wall_generate_upper_cripples(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int64_t opening_assembly_start(const Opening *opening, const BuildSettings *settings);
static int64_t opening_assembly_end(const Opening *opening, const BuildSettings *settings);
static int openings_conflict(const Opening *a, const Opening *b, const BuildSettings *settings);
static int wall_span_is_opening(const Wall *wall, const BuildSettings *settings, const Timber *left, const Timber *right);

static WallOpeningValidation wall_opening_validation(
    WallOpeningValidationCode code,
    DomainId conflicting_opening_id
)
{
    return (WallOpeningValidation){
        .code = code,
        .conflicting_opening_id = conflicting_opening_id
    };
}

WallOpeningValidation wall_validate_opening(
    const Wall *wall,
    const BuildSettings *settings,
    const WallOpeningProposal *proposal
)
{
    if (wall == NULL ||
        settings == NULL ||
        proposal == NULL || wall_length_mm(wall) == 0) {

        return wall_opening_validation(
            WALL_OPENING_INVALID_ARGUMENT,
            DOMAIN_ID_INVALID
        );
    }

    if (proposal->type != OPENING_DOOR &&
        proposal->type != OPENING_WINDOW) {

        return wall_opening_validation(
            WALL_OPENING_INVALID_TYPE,
            DOMAIN_ID_INVALID
        );
    }

    if (proposal->frame_position < 0 ||
        proposal->frame_bottom < 0 ||
        proposal->width <= 0 ||
        proposal->height <= 0) {

        return wall_opening_validation(
            WALL_OPENING_INVALID_DIMENSIONS,
            DOMAIN_ID_INVALID
        );
    }

    Opening opening = {
        .type = proposal->type,
        .frame_position = proposal->frame_position,
        .frame_bottom = proposal->frame_bottom,
        .width = proposal->width,
        .height = proposal->height,
        .width_allowance = proposal->width_allowance,
        .height_allowance = proposal->height_allowance,
        .custom_allowance = proposal->custom_allowance
    };

    int frame_width = opening_frame_width(&opening, settings);
    int frame_height = opening_frame_height(&opening, settings);

    if (frame_width <= 0 ||
        frame_height <= 0) {

        return wall_opening_validation(
            WALL_OPENING_INVALID_DIMENSIONS,
            DOMAIN_ID_INVALID
        );
    }

    if ((int64_t)opening.frame_bottom + frame_height > settings->stud_height) {
        return wall_opening_validation(
            WALL_OPENING_INVALID_HEIGHT,
            DOMAIN_ID_INVALID
        );
    }

    int64_t assembly_start = opening_assembly_start(&opening, settings);
    int64_t assembly_end = opening_assembly_end(&opening, settings);

    if (assembly_start < settings->stud_width) {
        return wall_opening_validation(
            WALL_OPENING_TOO_CLOSE_TO_LEFT_END,
            DOMAIN_ID_INVALID
        );
    }

    if (assembly_end > wall_length_mm(wall) - settings->stud_width) {
        return wall_opening_validation(
            WALL_OPENING_TOO_CLOSE_TO_RIGHT_END,
            DOMAIN_ID_INVALID
        );
    }

    for (size_t i = 0; i < wall->definition.opening_count; i++) {
        const Opening *existing = &wall->definition.openings[i];

        if (openings_conflict(&opening, existing, settings)) {
            return wall_opening_validation(
                WALL_OPENING_OVERLAPS_OPENING,
                existing->id
            );
        }
    }

    return wall_opening_validation(
        WALL_OPENING_VALID,
        DOMAIN_ID_INVALID
    );
}

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

    int64_t width = (int64_t)opening->width + allowance;
    return width > 0 && width <= INT_MAX ? (int)width : 0;
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

    int64_t height = (int64_t)opening->height + allowance;
    return height > 0 && height <= INT_MAX ? (int)height : 0;
}

int wall_add_opening_definition(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
)
{
    if (wall == NULL || settings == NULL || opening == NULL) {
        return 0;
    }

    if (opening->id == DOMAIN_ID_INVALID) {
        return 0;
    }

    if (wall_find_opening_by_id(wall, opening->id) != NULL) {
        return 0;
    }

    WallOpeningProposal proposal = {
        .type = opening->type,
        .frame_position = opening->frame_position,
        .frame_bottom = opening->frame_bottom,
        .width = opening->width,
        .height = opening->height,
        .width_allowance = opening->width_allowance,
        .height_allowance = opening->height_allowance,
        .custom_allowance = opening->custom_allowance
    };

    if (wall_validate_opening(
            wall,
            settings,
            &proposal).code != WALL_OPENING_VALID) {
        return 0;
    }

    if (wall->definition.opening_count ==
        wall->definition.opening_capacity) {

        size_t new_capacity =
            wall->definition.opening_capacity == 0
                ? 1
                : wall->definition.opening_capacity * 2;

        Opening *new_openings = realloc(
            wall->definition.openings,
            new_capacity * sizeof *new_openings
        );

        if (new_openings == NULL) {
            return 0;
        }

        wall->definition.openings =
            new_openings;

        wall->definition.opening_capacity =
            new_capacity;
    }

    wall->definition.openings[
        wall->definition.opening_count
    ] = *opening;

    wall->definition.opening_count++;

    return 1;
}

int wall_apply_opening_definition(Wall *wall, const BuildSettings *settings,
    DomainId opening_id, const Opening *opening)
{
    const Opening *existing = wall_find_opening_by_id_const(wall, opening_id);
    if (existing == NULL || settings == NULL || opening == NULL ||
        opening->id != opening_id ||
        wall->definition.opening_count > SIZE_MAX / sizeof(Opening)) {
        return 0;
    }
    size_t index = (size_t)(existing - wall->definition.openings);
    Wall candidate = {.id = wall->id, .definition = wall->definition};
    candidate.definition.openings = malloc(wall->definition.opening_count * sizeof(Opening));
    if (candidate.definition.openings == NULL) { return 0; }
    candidate.definition.opening_capacity = candidate.definition.opening_count;
    memcpy(candidate.definition.openings, wall->definition.openings,
        wall->definition.opening_count * sizeof(Opening));
    candidate.definition.openings[index] = *opening;

    /* The existing segment transaction validates every pair once, excluding
     * self by construction, and generates independently owned framing. */
    if (!wall_apply_plan_segment(&candidate, settings, candidate.definition.segment)) {
        wall_destroy(&candidate);
        return 0;
    }
    Wall previous = *wall;
    *wall = candidate;
    wall_destroy(&previous);
    return 1;
}

int wall_add_opening(
    Wall *wall,
    const BuildSettings *settings,
    DomainId opening_id,
    OpeningType type,
    int frame_position,
    int frame_bottom,
    int width,
    int height
)
{
    Opening opening = {
        .id = opening_id,
        .type = type,
        .frame_position = frame_position,
        .frame_bottom = frame_bottom,
        .width = width,
        .height = height,
        .width_allowance = 0,
        .height_allowance = 0,
        .custom_allowance = false
    };

    return wall_add_opening_definition(
        wall,
        settings,
        &opening
    );
}

Opening *wall_find_opening_by_id(
    Wall *wall,
    DomainId opening_id
)
{
    if (wall == NULL ||
        opening_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < wall->definition.opening_count;
         i++) {

        Opening *opening =
            &wall->definition.openings[i];

        if (opening->id == opening_id) {
            return opening;
        }
    }

    return NULL;
}

const Opening *wall_find_opening_by_id_const(
    const Wall *wall,
    DomainId opening_id
)
{
    if (wall == NULL ||
        opening_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < wall->definition.opening_count;
         i++) {

        const Opening *opening =
            &wall->definition.openings[i];

        if (opening->id == opening_id) {
            return opening;
        }
    }

    return NULL;
}

int wall_apply_openings(
    Wall *wall,
    const BuildSettings *settings,
    int wall_length
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    for (size_t opening_index = 0;
         opening_index < wall->definition.opening_count;
         opening_index++) {

        Opening *opening =
            &wall->definition.openings[opening_index];

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
            wall_length) {

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

        while (stud_index < wall->framing.stud_count) {

            Timber *stud =
                &wall->framing.studs[stud_index];

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
        wall->framing.studs,
        wall->framing.stud_count,
        sizeof *wall->framing.studs,
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
        stud->position.u;

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
     * Header position.z is its bottom face.
     */
    int header_z =
        opening->frame_bottom +
        frame_height;

    /*
     * Upper cripples begin on top
     * of the header.
     */
    int cripple_z =
        header_z +
        settings->stud_width;

    /*
     * They continue to the upper
     * framing limit.
     */
    int cripple_length =
        settings->stud_height -
        cripple_z;

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

        .z = cripple_z,
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

static int64_t opening_assembly_start(
    const Opening *opening,
    const BuildSettings *settings
)
{
    return
        (int64_t)opening->frame_position -
        (2 * (int64_t)settings->stud_width);
}

static int64_t opening_assembly_end(
    const Opening *opening,
    const BuildSettings *settings
)
{
    return
        (int64_t)opening->frame_position +
        opening_frame_width(
            opening,
            settings
        ) +
        (2 * (int64_t)settings->stud_width);
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

    int64_t a_start =
        opening_assembly_start(
            a,
            settings
        );

    int64_t a_end =
        opening_assembly_end(
            a,
            settings
        );

    int64_t b_start =
        opening_assembly_start(
            b,
            settings
        );

    int64_t b_end =
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
        wall->framing.studs,
        wall->framing.stud_count,
        sizeof *wall->framing.studs,
        wall_compare_stud_position
    );

    size_t i = 1;

    while (i < wall->framing.stud_count) {

        Timber *left =
            &wall->framing.studs[i - 1];

        Timber *right =
            &wall->framing.studs[i];

        int spacing =
            right->position.u -
            left->position.u;

        /*
         * Same U or touching/closely packed
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
                left->position.u +
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
            wall->framing.studs,
            wall->framing.stud_count,
            sizeof *wall->framing.studs,
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
        left->position.u +
        settings->stud_width;

    int span_right =
        right->position.u;

    if (span_right <= span_left) {
        return 0;
    }

    for (size_t i = 0;
         i < wall->definition.opening_count;
         i++) {

        const Opening *opening =
            &wall->definition.openings[i];

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

int wall_remove_opening_by_id(
    Wall *wall,
    DomainId opening_id
)
{
    if (
        wall == NULL
        || opening_id == DOMAIN_ID_INVALID
    ) {
        return 0;
    }

    for (
        size_t i = 0;
        i < wall->definition.opening_count;
        i++
    ) {
        if (
            wall->definition.openings[i].id
            != opening_id
        ) {
            continue;
        }

        size_t remaining =
            wall->definition.opening_count
            - i
            - 1;

        if (remaining > 0) {
            memmove(
                &wall->definition.openings[i],
                &wall->definition.openings[i + 1],
                remaining
                    * sizeof *wall->definition.openings
            );
        }

        wall->definition.opening_count--;

        wall->definition.openings[
            wall->definition.opening_count
        ] = (Opening){0};

        return 1;
    }

    return 0;
}
