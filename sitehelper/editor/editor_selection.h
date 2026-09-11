#ifndef EDITOR_SELECTION_H
#define EDITOR_SELECTION_H

#include <stdbool.h>

#include "domain_id.h"
#include "wall_selection.h"

typedef enum
{
    EDITOR_SELECTION_NONE = 0,
    EDITOR_SELECTION_WALL_MEMBER,
    EDITOR_SELECTION_WALL,
    EDITOR_SELECTION_OPENING
} EditorSelectionKind;

typedef struct
{
    EditorSelectionKind kind;

    DomainId wall_id;
    DomainId opening_id; /* Only for OPENING; wall_id is its explicit owner. */

    WallSelection wall_member;
} EditorSelection;

/* Value-only selection; no project collection or generated allocation pointers. */
void editor_selection_set_wall(EditorSelection *selection, DomainId wall_id);
void editor_selection_set_opening(EditorSelection *selection,
    DomainId wall_id, DomainId opening_id);

void editor_selection_init(
    EditorSelection *selection
);

void editor_selection_clear(
    EditorSelection *selection
);

void editor_selection_set_wall_member(
    EditorSelection *selection,
    DomainId wall_id,
    WallMemberKind member_kind,
    const Timber *timber
);

bool editor_selection_is_empty(
    const EditorSelection *selection
);

const WallSelection *
editor_selection_get_wall_member(
    const EditorSelection *selection,
    DomainId wall_id
);

#endif
