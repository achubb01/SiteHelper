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
        .roof_id = DOMAIN_ID_INVALID,
        .roof_portion_id = DOMAIN_ID_INVALID,
        .document = {DOCUMENT_OBJECT_NONE, DOMAIN_ID_INVALID},
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


void editor_selection_set_roof(EditorSelection *selection,
    EditorSelectionScope scope, DomainId roof_id)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (scope != EDITOR_SELECTION_SCOPE_PLAN || roof_id == DOMAIN_ID_INVALID) { return; }
    selection->kind = EDITOR_SELECTION_ROOF;
    selection->scope = scope;
    selection->roof_id = roof_id;
}

void editor_selection_set_roof_portion(EditorSelection *selection,
    EditorSelectionScope scope, DomainId roof_id, DomainId portion_id)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (scope != EDITOR_SELECTION_SCOPE_PLAN || roof_id == DOMAIN_ID_INVALID ||
        portion_id == DOMAIN_ID_INVALID) { return; }
    selection->kind = EDITOR_SELECTION_ROOF_PORTION;
    selection->scope = scope;
    selection->roof_id = roof_id;
    selection->roof_portion_id = portion_id;
}

void editor_selection_set_document(EditorSelection *selection,
    EditorSelectionScope scope, DocumentObjectRef document)
{
    if (selection == NULL) { return; }
    editor_selection_clear(selection);
    if (scope != EDITOR_SELECTION_SCOPE_PLAN || !document_object_ref_is_valid(document)) {
        return;
    }
    selection->kind = EDITOR_SELECTION_DOCUMENT;
    selection->scope = scope;
    selection->document = document;
}

void editor_selection_set_annotation(EditorSelection *selection,
    EditorSelectionScope scope, DomainId annotation_id)
{
    editor_selection_set_document(selection, scope,
        (DocumentObjectRef){DOCUMENT_OBJECT_NOTE, annotation_id});
}

void editor_selection_set_dimension(EditorSelection *selection,
    EditorSelectionScope scope, DomainId dimension_id)
{
    editor_selection_set_document(selection, scope,
        (DocumentObjectRef){DOCUMENT_OBJECT_DIMENSION, dimension_id});
}

void editor_selection_set_symbol(EditorSelection *selection,
    EditorSelectionScope scope, DomainId symbol_id)
{
    editor_selection_set_document(selection, scope,
        (DocumentObjectRef){DOCUMENT_OBJECT_SYMBOL, symbol_id});
}

void editor_selection_set_callout(EditorSelection *selection,
    EditorSelectionScope scope, DomainId callout_id)
{
    editor_selection_set_document(selection, scope,
        (DocumentObjectRef){DOCUMENT_OBJECT_CALLOUT, callout_id});
}

void editor_selection_set_revision_cloud(EditorSelection *selection,
    EditorSelectionScope scope, DomainId revision_cloud_id)
{
    editor_selection_set_document(selection, scope,
        (DocumentObjectRef){DOCUMENT_OBJECT_REVISION_CLOUD, revision_cloud_id});
}

int editor_selection_is_document_kind(const EditorSelection *selection,
    DocumentObjectKind kind)
{
    return selection != NULL && selection->kind == EDITOR_SELECTION_DOCUMENT &&
        selection->scope == EDITOR_SELECTION_SCOPE_PLAN &&
        selection->document.kind == kind && document_object_ref_is_valid(selection->document);
}

int editor_selection_matches_document(const EditorSelection *selection,
    DocumentObjectKind kind, DomainId id)
{
    return editor_selection_is_document_kind(selection,kind) &&
        id != DOMAIN_ID_INVALID && selection->document.id == id;
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
