#include "wall_selection.h"


static bool positions_equal(
    Position a,
    Position b
)
{
    return
        a.x == b.x &&
        a.y == b.y;
}


static bool timber_matches(
    const Timber *candidate,
    const Timber *selected
)
{
    if (candidate == NULL ||
        selected == NULL) {

        return false;
    }

    if (candidate->type != selected->type) {
        return false;
    }

    if (candidate->length != selected->length ||
        candidate->depth != selected->depth ||
        candidate->width != selected->width) {

        return false;
    }

    if (!positions_equal(
            candidate->position,
            selected->position)) {

        return false;
    }

    switch (candidate->type) {

        case TIMBER_STUD:
            return
                candidate->details.stud.type ==
                selected->details.stud.type;

        case TIMBER_NOGGIN:
            /*
             * Bay is generation metadata rather than
             * persistent member identity.
             *
             * Position and dimensions are sufficient
             * for resolving the current generated noggin.
             */
            return true;

        case TIMBER_PLATE:
        case TIMBER_HEADER:
        case TIMBER_SILL:
            return true;
    }

    return false;
}


static const Timber *find_matching_timber(
    const Timber *timbers,
    size_t count,
    const Timber *selected
)
{
    if (timbers == NULL ||
        selected == NULL) {

        return NULL;
    }

    for (size_t i = 0;
         i < count;
         i++) {

        if (timber_matches(
                &timbers[i],
                selected)) {

            return &timbers[i];
        }
    }

    return NULL;
}


void wall_selection_init(
    WallSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    selection->kind =
        WALL_MEMBER_NONE;
}


void wall_selection_clear(
    WallSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    selection->kind =
        WALL_MEMBER_NONE;
}


void wall_selection_set(
    WallSelection *selection,
    WallMemberKind kind,
    const Timber *timber
)
{
    if (selection == NULL) {
        return;
    }

    if (kind == WALL_MEMBER_NONE ||
        timber == NULL) {

        wall_selection_clear(
            selection
        );

        return;
    }

    selection->kind =
        kind;

    selection->timber =
        *timber;
}


bool wall_selection_is_empty(
    const WallSelection *selection
)
{
    return
        selection == NULL ||
        selection->kind == WALL_MEMBER_NONE;
}


const Timber *wall_selection_resolve(
    const WallSelection *selection,
    const Wall *wall
)
{
    if (selection == NULL ||
        wall == NULL ||
        selection->kind == WALL_MEMBER_NONE) {

        return NULL;
    }

    switch (selection->kind) {

        case WALL_MEMBER_BOTTOM_PLATE:

            if (timber_matches(
                    &wall->framing.bottomplate,
                    &selection->timber)) {

                return &wall->framing.bottomplate;
            }

            return NULL;


        case WALL_MEMBER_TOP_PLATE:

            if (timber_matches(
                    &wall->framing.topplate,
                    &selection->timber)) {

                return &wall->framing.topplate;
            }

            return NULL;


        case WALL_MEMBER_STUD:

            return find_matching_timber(
                wall->framing.studs,
                wall->framing.stud_count,
                &selection->timber
            );


        case WALL_MEMBER_NOGGIN:

            return find_matching_timber(
                wall->framing.nogs,
                wall->framing.nog_count,
                &selection->timber
            );


        case WALL_MEMBER_GENERATED:

            return find_matching_timber(
                wall->framing.members,
                wall->framing.member_count,
                &selection->timber
            );


        case WALL_MEMBER_NONE:
            return NULL;
    }

    return NULL;
}


void wall_selection_reconcile(
    WallSelection *selection,
    const Wall *wall
)
{
    if (selection == NULL) {
        return;
    }

    if (wall_selection_is_empty(
            selection)) {

        return;
    }

    if (wall_selection_resolve(
            selection,
            wall) == NULL) {

        wall_selection_clear(
            selection
        );
    }
}