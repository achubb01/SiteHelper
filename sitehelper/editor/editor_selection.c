#include "editor_selection.h"

void editor_selection_init(
    EditorSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    *selection = (EditorSelection){
        .kind = EDITOR_SELECTION_NONE,
        .wall_id = DOMAIN_ID_INVALID
    };

    wall_selection_init(
        &selection->wall_member
    );
}

void editor_selection_clear(
    EditorSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    selection->kind =
        EDITOR_SELECTION_NONE;

    selection->wall_id =
        DOMAIN_ID_INVALID;

    wall_selection_clear(
        &selection->wall_member
    );
}

void editor_selection_set_wall_member(
    EditorSelection *selection,
    DomainId wall_id,
    WallMemberKind member_kind,
    const Timber *timber
)
{
    if (selection == NULL) {
        return;
    }

    if (
        wall_id == DOMAIN_ID_INVALID
        || member_kind == WALL_MEMBER_NONE
        || timber == NULL
    ) {
        editor_selection_clear(
            selection
        );

        return;
    }

    selection->kind =
        EDITOR_SELECTION_WALL_MEMBER;

    selection->wall_id =
        wall_id;

    wall_selection_set(
        &selection->wall_member,
        member_kind,
        timber
    );
}

bool editor_selection_is_empty(
    const EditorSelection *selection
)
{
    return
        selection == NULL
        || selection->kind
            == EDITOR_SELECTION_NONE;
}

const WallSelection *
editor_selection_get_wall_member(
    const EditorSelection *selection,
    DomainId wall_id
)
{
    if (
        selection == NULL
        || selection->kind
            != EDITOR_SELECTION_WALL_MEMBER
        || selection->wall_id != wall_id
    ) {
        return NULL;
    }

    return &selection->wall_member;
}