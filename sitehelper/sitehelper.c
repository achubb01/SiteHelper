#include <stdlib.h>

#include "./sitehelper.h"

static int wall_generate_studs(Wall *wall, const BuildSettings *settings);
static int wall_generate_noggins(Wall *wall, const BuildSettings *settings);
static void wall_clear_studs(Wall *wall);
static void wall_clear_noggins(Wall *wall);
static int wall_apply_openings(Wall *wall, const BuildSettings *settings);
static int stud_overlaps_range(const Timber *stud, int range_start, int range_end, const BuildSettings *settings);
static void wall_remove_stud(Wall *wall, size_t index);
static int compare_stud_position(const void *a, const void *b);
static int noggin_intersects_opening(const Wall *wall, size_t bay, int vertical_position, const BuildSettings *settings);
static int wall_add_custom_stud(Wall *wall, const BuildSettings *settings, int x, int y, int length, StudType type);
static int wall_add_header(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int wall_add_member(Wall *wall, Timber member);
static int wall_add_sill(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int wall_frame_door(Wall *wall, const BuildSettings *settings, const Opening *opening, int left_trimmer_position, int right_trimmer_position);
static int wall_frame_window(Wall *wall, const BuildSettings *settings, const Opening *opening, int left_trimmer_position, int right_trimmer_position);
static void wall_clear_members(Wall *wall);
static int wall_generate_lower_cripples(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int add_stud_at_position(int position, void *context);
static int generate_positions(int start, int end, const BuildSettings *settings, PositionCallback callback, void *context);
static int wall_generate_upper_cripples(Wall *wall, const BuildSettings *settings, const Opening *opening);
static int opening_assembly_start(const Opening *opening, const BuildSettings *settings);
static int opening_assembly_end(const Opening *opening, const BuildSettings *settings);
static int openings_conflict(const Opening *a, const Opening *b, const BuildSettings *settings);
static int wall_repair_stud_spacing(Wall *wall, const BuildSettings *settings);
static int wall_span_is_opening(const Wall *wall, const BuildSettings *settings, const Timber *left, const Timber *right);


int build_add_room(BuildStructure *structure)
{
    if (structure == NULL) {
        return 0;
    }

    if (structure->room_count ==
        structure->room_capacity) {

        size_t new_capacity =
            structure->room_capacity == 0
                ? 1
                : structure->room_capacity * 2;

        Room *new_rooms = realloc(
            structure->rooms,
            new_capacity * sizeof *new_rooms
        );

        if (new_rooms == NULL) {
            return 0;
        }

        structure->rooms = new_rooms;
        structure->room_capacity = new_capacity;
    }

    structure->rooms[structure->room_count] =
        (Room){0};

    structure->room_count++;

    return 1;
}

int room_add_wall(Room *room)
{
    if (room == NULL) {
        return 0;
    }

    if (room->wall_count == room->wall_capacity) {
        size_t new_capacity =
            room->wall_capacity == 0
                ? 1
                : room->wall_capacity * 2;

        Wall *new_walls = realloc(
            room->walls,
            new_capacity * sizeof *new_walls
        );

        if (new_walls == NULL) {
            return 0;
        }

        room->walls = new_walls;
        room->wall_capacity = new_capacity;
    }

    room->walls[room->wall_count] = (Wall){0};
    room->wall_count++;

    return 1;
}

int build_set_stud_spacing(
    BuildSettings *settings,
    int spacing
)
{
    if (settings == NULL) {
        return 0;
    }

    if (spacing <= 0) {
        return 0;
    }

    settings->stud_spacing = spacing;

    return 1;
}

int wall_set_length(Wall *wall, int length)
{
    if (wall == NULL) {
        return 0;
    }

    if (length <= 0) {
        return 0;
    }

    wall->bottomplate.length = length;

    return 1;
}

int wall_generate(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (wall->bottomplate.length <= 0) {
        return 0;
    }

    if (settings->stud_height <= 0 ||
        settings->stud_width <= 0 ||
        settings->stud_depth <= 0 ||
        settings->stud_spacing <= 0 ||
        settings->nog_spacing <= 0) {
        return 0;
    }

    wall_clear_studs(wall);
    wall_clear_noggins(wall);
    wall_clear_members(wall);

    if (!wall_generate_studs(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_apply_openings(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_repair_stud_spacing(
            wall,
            settings)) {
        return 0;
    }

    if (!wall_generate_noggins(
            wall,
            settings)) {
        return 0;
    }

    return 1;
}

static int wall_generate_studs(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL ||
        settings == NULL) {
        return 0;
    }

    int end =
        wall->bottomplate.length -
        settings->stud_width;

    if (end < 0) {
        return 0;
    }

    StudGenerationContext context = {
        .wall = wall,
        .settings = settings,
        .length = settings->stud_height,
        .type = STUD_COMMON
    };

    return generate_positions(
        0,
        end,
        settings,
        add_stud_at_position,
        &context
    );
}

int wall_add_stud(
    Wall *wall,
    const BuildSettings *settings,
    int position,
    StudType type
)
{
    return wall_add_custom_stud(
        wall,
        settings,
        position,
        0,
        settings->stud_height,
        type
    );
}

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

static int generate_positions(
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

static int add_stud_at_position(
    int position,
    void *context
)
{
    StudGenerationContext *generation =
        context;

    if (generation == NULL) {
        return 0;
    }

    return wall_add_custom_stud(
        generation->wall,
        generation->settings,
        position,
        generation->y,
        generation->length,
        generation->type
    );
}

static void wall_clear_studs(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->stud_count = 0;
}

static void wall_clear_noggins(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->nog_count = 0;
}

int wall_add_noggin(
    Wall *wall,
    const BuildSettings *settings,
    size_t bay,
    int vertical_position
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (bay + 1 >= wall->stud_count) {
        return 0;
    }

    Timber *left =
        &wall->studs[bay];

    Timber *right =
        &wall->studs[bay + 1];

    int length =
        right->position.x
        - left->position.x
        - settings->stud_width;

    if (length <= 0) {
        return 0;
    }

    if (wall->nog_count == wall->nog_capacity) {

        size_t new_capacity =
            wall->nog_capacity == 0
                ? 1
                : wall->nog_capacity * 2;

        Timber *new_nogs = realloc(
            wall->nogs,
            new_capacity * sizeof *new_nogs
        );

        if (new_nogs == NULL) {
            return 0;
        }

        wall->nogs = new_nogs;
        wall->nog_capacity = new_capacity;
    }

    Timber noggin = {
        .length = length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = left->position.x
                + settings->stud_width,
            .y = vertical_position
        },

        .type = TIMBER_NOGGIN,

        .details.noggin = {
            .bay = bay
        }
    };

    wall->nogs[wall->nog_count] = noggin;
    wall->nog_count++;

    return 1;
}

static int wall_generate_noggins(
    Wall *wall,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (settings->stud_height <= 0 ||
        settings->nog_spacing <= 0) {
        return 0;
    }

    if (wall->stud_count < 2) {
        return 0;
    }

    int gaps =
        (settings->stud_height +
         settings->nog_spacing - 1)
        /
        settings->nog_spacing;

    for (int row = 1;
         row < gaps;
         row++) {

        int vertical_position =
            (settings->stud_height * row)
            / gaps;

        for (size_t bay = 0;
            bay + 1 < wall->stud_count;
            bay++) {

            Timber *left =
                &wall->studs[bay];

            Timber *right =
                &wall->studs[bay + 1];

            int clear_width =
                right->position.x
                - left->position.x
                - settings->stud_width;

            if (clear_width <= 0) {
                continue;
            }

            /*
            * Both vertical members must physically
            * reach this noggin row.
            */
            if (left->length <= vertical_position ||
                right->length <= vertical_position) {
                continue;
            }

            if (noggin_intersects_opening(
                    wall,
                    bay,
                    vertical_position,
                    settings)) {
                continue;
            }

            if (!wall_add_noggin(
                    wall,
                    settings,
                    bay,
                    vertical_position)) {

                return 0;
            }
        }
    }

    return 1;
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

static int wall_apply_openings(
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
        compare_stud_position
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

static void wall_remove_stud(
    Wall *wall,
    size_t index
)
{
    if (wall == NULL ||
        index >= wall->stud_count) {
        return;
    }

    for (size_t i = index;
         i + 1 < wall->stud_count;
         i++) {

        wall->studs[i] =
            wall->studs[i + 1];
    }

    wall->stud_count--;
}

static int compare_stud_position(
    const void *a,
    const void *b
)
{
    const Timber *stud_a = a;
    const Timber *stud_b = b;

    return
        stud_a->position.x -
        stud_b->position.x;
}

static int noggin_intersects_opening(
    const Wall *wall,
    size_t bay,
    int vertical_position,
    const BuildSettings *settings
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (bay + 1 >= wall->stud_count) {
        return 0;
    }

    const Timber *left_stud =
        &wall->studs[bay];

    const Timber *right_stud =
        &wall->studs[bay + 1];

    int noggin_start =
        left_stud->position.x
        + settings->stud_width;

    int noggin_end =
        right_stud->position.x;

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

        int opening_bottom =
            opening->frame_bottom;

        int opening_top =
            opening_bottom +
            opening_frame_height(
                opening,
                settings
            );

        int horizontal_overlap =
            noggin_start < opening_right &&
            noggin_end > opening_left;

        int vertical_overlap =
            vertical_position > opening_bottom &&
            vertical_position < opening_top;

        if (horizontal_overlap &&
            vertical_overlap) {

            return 1;
        }
    }

    return 0;
}

static int wall_add_custom_stud(
    Wall *wall,
    const BuildSettings *settings,
    int x, 
    int y,
    int length,
    StudType type
)
{
    if (wall == NULL || settings == NULL) {
        return 0;
    }

    if (x < 0 || y < 0 || length <= 0) {
        return 0;
    }

    if (wall->stud_count ==
        wall->stud_capacity) {

        size_t new_capacity =
            wall->stud_capacity == 0
                ? 1
                : wall->stud_capacity * 2;

        Timber *new_studs = realloc(
            wall->studs,
            new_capacity * sizeof *new_studs
        );

        if (new_studs == NULL) {
            return 0;
        }

        wall->studs = new_studs;
        wall->stud_capacity =
            new_capacity;
    }

    Timber stud = {
        .length = length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = x,
            .y = y
        },

        .type = TIMBER_STUD,

        .details.stud = {
            .type = type
        }
    };

    wall->studs[
        wall->stud_count
    ] = stud;

    wall->stud_count++;

    return 1;
}

static int wall_add_header(
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

    int left_trimmer_position =
        opening->frame_position -
        settings->stud_width;

    int header_length =
        frame_width +
        (2 * settings->stud_width);

    int header_y =
        opening->frame_bottom +
        frame_height;

    if (left_trimmer_position < 0) {
        return 0;
    }

    if (header_y < 0 ||
        header_y > settings->stud_height) {
        return 0;
    }

    Timber header = {
        .length = header_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = left_trimmer_position,
            .y = header_y
        },

        .type = TIMBER_HEADER
    };

    return wall_add_member(
        wall,
        header
    );
}

static int wall_add_member(
    Wall *wall,
    Timber member
)
{
    if (wall == NULL) {
        return 0;
    }

    if (wall->member_count ==
        wall->member_capacity) {

        size_t new_capacity =
            wall->member_capacity == 0
                ? 1
                : wall->member_capacity * 2;

        Timber *new_members = realloc(
            wall->members,
            new_capacity *
            sizeof *new_members
        );

        if (new_members == NULL) {
            return 0;
        }

        wall->members = new_members;
        wall->member_capacity =
            new_capacity;
    }

    wall->members[
        wall->member_count
    ] = member;

    wall->member_count++;

    return 1;
}

static int wall_add_sill(
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

    int sill_length =
        opening_frame_width(
            opening,
            settings
        );

    if (sill_length <= 0 ||
        opening->frame_bottom <= 0) {
        return 0;
    }

    Timber sill = {
        .length = sill_length,
        .depth = settings->stud_depth,
        .width = settings->stud_width,

        .position = {
            .x = opening->frame_position,
            .y = opening->frame_bottom
        },

        .type = TIMBER_SILL
    };

    return wall_add_member(
        wall,
        sill
    );
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

static void wall_clear_members(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall->member_count = 0;
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

    return generate_positions(
        start,
        end,
        settings,
        add_stud_at_position,
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

    return generate_positions(
        start,
        end,
        settings,
        add_stud_at_position,
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

static int wall_repair_stud_spacing(
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
        compare_stud_position
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
            compare_stud_position
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