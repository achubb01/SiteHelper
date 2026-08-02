#include "wall_editor.h"
#include "wall_query.h"
#include "wall_selection.h"

void wall_editor_init(
    WallEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    wall_selection_init(
        &editor->selection
    );
}

void wall_editor_select_at_position(
    WallEditor *editor,
    const Wall *wall,
    Position position
)
{
    if (editor == NULL ||
        wall == NULL) {

        return;
    }

    const Timber *timber =
        wall_find_timber_at_position(
            wall,
            position
        );

    wall_selection_set(
        &editor->selection,
        timber
    );
}

void wall_editor_clear_selection(
    WallEditor *editor
)
{
    if (editor == NULL) {
        return;
    }

    wall_selection_clear(
        &editor->selection
    );
}

const WallSelection *wall_editor_get_selection(
    const WallEditor *editor
)
{
    if (editor == NULL) {
        return NULL;
    }

    return &editor->selection;
}