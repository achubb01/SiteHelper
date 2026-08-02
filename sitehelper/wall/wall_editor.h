#ifndef WALL_EDITOR_H
#define WALL_EDITOR_H

#include "wall.h"
#include "wall_selection.h"

typedef struct
{
    WallSelection selection;
} WallEditor;

void wall_editor_init(
    WallEditor *editor
);

void wall_editor_select_at_position(
    WallEditor *editor,
    const Wall *wall,
    Position position
);

void wall_editor_clear_selection(
    WallEditor *editor
);

const WallSelection *wall_editor_get_selection(
    const WallEditor *editor
);

#endif