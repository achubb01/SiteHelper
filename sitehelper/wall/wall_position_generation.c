#include "wall_internal.h"

static int generate_positions_even(
    int start,
    int end,
    int max_spacing,
    PositionCallback callback,
    void *context
)
{
    if (callback == NULL) {
        return 0;
    }

    if (start > end ||
        max_spacing <= 0) {
        return 0;
    }

    int span =
        end - start;

    /*
     * No span means there is only
     * one required position.
     */
    if (span == 0) {
        return callback(
            start,
            context
        );
    }

    int gaps =
        (span + max_spacing - 1)
        / max_spacing;

    for (int i = 0;
         i <= gaps;
         i++) {

        int position =
            start +
            (span * i) / gaps;

        if (!callback(
                position,
                context)) {
            return 0;
        }
    }

    return 1;
}

static int generate_positions_maximise(
    int start,
    int end,
    int max_spacing,
    PositionCallback callback,
    void *context
)
{
    if (callback == NULL) {
        return 0;
    }

    if (start > end ||
        max_spacing <= 0) {
        return 0;
    }

    int span =
        end - start;

    if (span == 0) {
        return callback(
            start,
            context
        );
    }

    /*
     * Only one gap is needed.
     */
    if (span <= max_spacing) {

        if (!callback(
                start,
                context)) {
            return 0;
        }

        return callback(
            end,
            context
        );
    }

    int full_gaps =
        span / max_spacing;

    int remainder =
        span % max_spacing;

    /*
     * Start position.
     */
    if (!callback(
            start,
            context)) {
        return 0;
    }

    /*
     * Perfectly divisible.
     */
    if (remainder == 0) {

        for (int i = 1;
             i <= full_gaps;
             i++) {

            int position =
                start +
                i * max_spacing;

            if (!callback(
                    position,
                    context)) {
                return 0;
            }
        }

        return 1;
    }

    /*
     * Keep as many full-size gaps
     * as possible.
     */
    int position = start;

    for (int i = 0;
         i < full_gaps - 1;
         i++) {

        position += max_spacing;

        if (!callback(
                position,
                context)) {
            return 0;
        }
    }

    /*
     * Split the remaining distance
     * over the final two gaps.
     */
    int remaining_span =
        end - position;

    int first_special =
        remaining_span / 2;

    int second_special =
        remaining_span -
        first_special;

    position += first_special;

    if (!callback(
            position,
            context)) {
        return 0;
    }

    position += second_special;

    if (!callback(
            position,
            context)) {
        return 0;
    }

    return 1;
}

int wall_generate_positions(
    int start,
    int end,
    const BuildSettings *settings,
    PositionCallback callback,
    void *context
)
{
    if (settings == NULL) {
        return 0;
    }

    switch (settings->stud_spacing_mode) {

        case STUD_SPACING_EVEN:

            return generate_positions_even(
                start,
                end,
                settings->stud_spacing,
                callback,
                context
            );

        case STUD_SPACING_MAXIMISE:

            return generate_positions_maximise(
                start,
                end,
                settings->stud_spacing,
                callback,
                context
            );

        default:
            return 0;
    }
}