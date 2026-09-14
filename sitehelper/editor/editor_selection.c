#include "editor_selection.h"

static bool valid_scope(EditorSelectionScope scope)
{
    return scope == EDITOR_SELECTION_SCOPE_PLAN || scope == EDITOR_SELECTION_SCOPE_WALL_ELEVATION;
}

void editor_selection_init(
    EditorSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    *selection = (EditorSelection){
        .kind = EDITOR_SELECTION_NONE,
        .scope = EDITOR_SELECTION_SCOPE_NONE,
        .wall_id = DOMAIN_ID_INVALID,
        .opening_id = DOMAIN_ID_INVALID,
        .slab_id = DOMAIN_ID_INVALID,
        .slab_feature_index = SIZE_MAX
    };

    wall_selection_init(
        &selection->wall_member
    );
}

void editor_selection_set_slab(EditorSelection *selection,
    EditorSelectionScope scope, DomainId slab_id)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (scope != EDITOR_SELECTION_SCOPE_PLAN || slab_id == DOMAIN_ID_INVALID) { return; }
    selection->kind = EDITOR_SELECTION_SLAB;
    selection->scope = scope;
    selection->slab_id = slab_id;
}

void editor_selection_set_slab_feature(EditorSelection *selection,
    EditorSelectionScope scope, DomainId slab_id, EditorSelectionKind kind,
    size_t feature_index)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (scope != EDITOR_SELECTION_SCOPE_PLAN || slab_id == DOMAIN_ID_INVALID ||
        feature_index == SIZE_MAX ||
        (kind != EDITOR_SELECTION_SLAB_PENETRATION &&
         kind != EDITOR_SELECTION_SLAB_REGION &&
         kind != EDITOR_SELECTION_SLAB_EDGE_REBATE)) {
        return;
    }
    selection->kind = kind;
    selection->scope = scope;
    selection->slab_id = slab_id;
    selection->slab_feature_index = feature_index;
}

void editor_selection_clear(
    EditorSelection *selection
)
{
    if (selection == NULL) {
        return;
    }

    editor_selection_init(selection);
}

void editor_selection_set_wall(EditorSelection *selection,
    EditorSelectionScope scope, DomainId wall_id)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (!valid_scope(scope) || wall_id == DOMAIN_ID_INVALID) { return; }
    selection->kind = EDITOR_SELECTION_WALL;
    selection->scope = scope;
    selection->wall_id = wall_id;
}

void editor_selection_set_opening(EditorSelection *selection,
    EditorSelectionScope scope, DomainId wall_id, DomainId opening_id)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (!valid_scope(scope) || wall_id == DOMAIN_ID_INVALID || opening_id == DOMAIN_ID_INVALID) { return; }
    selection->kind = EDITOR_SELECTION_OPENING;
    selection->scope = scope;
    selection->wall_id = wall_id;
    selection->opening_id = opening_id;
}

void editor_selection_set_wall_member(
    EditorSelection *selection,
    EditorSelectionScope scope,
    DomainId wall_id,
    WallMemberKind member_kind,
    const Timber *timber
)
{
    if (selection == NULL) {
        return;
    }

    if (
        !valid_scope(scope)
        || wall_id == DOMAIN_ID_INVALID
        || member_kind <= WALL_MEMBER_NONE
        || member_kind > WALL_MEMBER_GENERATED
        || timber == NULL
    ) {
        editor_selection_clear(
            selection
        );

        return;
    }

    Timber value = *timber; /* Input may alias the previously selected value. */
    editor_selection_clear(selection);
    selection->kind =
        EDITOR_SELECTION_WALL_MEMBER;
    selection->scope = scope;
    selection->opening_id = DOMAIN_ID_INVALID;

    selection->wall_id =
        wall_id;

    wall_selection_set(
        &selection->wall_member,
        member_kind,
        &value
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
    EditorSelectionScope scope,
    DomainId wall_id
)
{
    if (
        !editor_selection_matches_scope(selection, scope)
        || selection->kind
            != EDITOR_SELECTION_WALL_MEMBER
        || selection->wall_id != wall_id
    ) {
        return NULL;
    }

    return &selection->wall_member;
}

bool editor_selection_matches_scope(const EditorSelection *selection,
    EditorSelectionScope scope)
{
    return !editor_selection_is_empty(selection) && valid_scope(scope) && selection->scope == scope;
}
