#include "wall_selection.h"


void wall_selection_init(
    WallSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    selection->selected = NULL;
}


void wall_selection_clear(
    WallSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    selection->selected = NULL;
}


void wall_selection_set(
    WallSelection *selection,
    const Timber *timber
)
{
    if (selection == NULL) {
        return;
    }

    selection->selected = timber;
}


const Timber *wall_selection_get(
    const WallSelection *selection
)
{
    if (selection == NULL) {
        return NULL;
    }

    return selection->selected;
}


bool wall_selection_contains(
    const WallSelection *selection,
    const Timber *timber
)
{
    if (selection == NULL ||
        timber == NULL) {

        return false;
    }

    return selection->selected == timber;
}