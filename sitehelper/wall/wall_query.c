#include "wall_query.h"


static bool timber_has_valid_dimensions(
    const Timber *timber
)
{
    return
        timber != NULL &&
        timber->length > 0 &&
        timber->width > 0;
}


static bool vertical_timber_contains_position(
    const Timber *timber,
    Position position
)
{
    if (!timber_has_valid_dimensions(timber)) {
        return false;
    }

    int left =
        timber->position.x;

    int right =
        timber->position.x +
        timber->width;

    int bottom =
        timber->position.y;

    int top =
        timber->position.y +
        timber->length;

    return
        position.x >= left &&
        position.x <= right &&
        position.y >= bottom &&
        position.y <= top;
}


static bool horizontal_timber_contains_position(
    const Timber *timber,
    Position position
)
{
    if (!timber_has_valid_dimensions(timber)) {
        return false;
    }

    int left =
        timber->position.x;

    int right =
        timber->position.x +
        timber->length;

    int bottom =
        timber->position.y;

    int top =
        timber->position.y +
        timber->width;

    return
        position.x >= left &&
        position.x <= right &&
        position.y >= bottom &&
        position.y <= top;
}


static const Timber *find_vertical_timber_in_array(
    const Timber *timbers,
    size_t count,
    Position position
)
{
    if (timbers == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (vertical_timber_contains_position(
                &timbers[i],
                position)) {

            return &timbers[i];
        }
    }

    return NULL;
}


static const Timber *find_horizontal_timber_in_array(
    const Timber *timbers,
    size_t count,
    Position position
)
{
    if (timbers == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        if (horizontal_timber_contains_position(
                &timbers[i],
                position)) {

            return &timbers[i];
        }
    }

    return NULL;
}


const Timber *wall_find_timber_at_position(
    const Wall *wall,
    Position position
)
{
    if (wall == NULL) {
        return NULL;
    }

    /*
     * Plates are checked first. Therefore, a position shared by a
     * stud and plate will currently select the plate.
     */

    if (horizontal_timber_contains_position(
            &wall->framing.bottomplate,
            position)) {

        return &wall->framing.bottomplate;
    }

    if (horizontal_timber_contains_position(
            &wall->framing.topplate,
            position)) {

        return &wall->framing.topplate;
    }

    const Timber *selected =
        find_vertical_timber_in_array(
            wall->framing.studs,
            wall->framing.stud_count,
            position
        );

    if (selected != NULL) {
        return selected;
    }

    selected =
        find_horizontal_timber_in_array(
            wall->framing.nogs,
            wall->framing.nog_count,
            position
        );

    if (selected != NULL) {
        return selected;
    }

    selected =
        find_horizontal_timber_in_array(
            wall->framing.members,
            wall->framing.member_count,
            position
        );

    return selected;
}