#ifndef EDITOR_SELECTION_H
#define EDITOR_SELECTION_H

#include <stdbool.h>
#include <stddef.h>

#include "domain_id.h"
#include "document.h"
#include "wall_selection.h"

typedef enum
{
    EDITOR_SELECTION_NONE = 0,
    EDITOR_SELECTION_WALL_MEMBER,
    EDITOR_SELECTION_WALL,
    EDITOR_SELECTION_OPENING,
    EDITOR_SELECTION_SLAB,
    EDITOR_SELECTION_SLAB_PENETRATION,
    EDITOR_SELECTION_SLAB_REGION,
    EDITOR_SELECTION_SLAB_EDGE_REBATE,
    EDITOR_SELECTION_ROOF,
    EDITOR_SELECTION_ROOF_PORTION,
    EDITOR_SELECTION_DOCUMENT
} EditorSelectionKind;

/* Context of a transient selection, independent of its object kind and of
 * EditorView (the presentation/navigation enum). */
typedef enum
{
    EDITOR_SELECTION_SCOPE_NONE = 0,
    EDITOR_SELECTION_SCOPE_PLAN,
    EDITOR_SELECTION_SCOPE_WALL_ELEVATION
} EditorSelectionScope;

typedef struct
{
    EditorSelectionKind kind;
    EditorSelectionScope scope;

    DomainId wall_id;
    DomainId opening_id; /* Only for OPENING; wall_id is its explicit owner. */
    DomainId slab_id;
    DomainId roof_id;
    DomainId roof_portion_id; /* Only for ROOF_PORTION; roof_id is its owner. */
    /* Project-owned, Storey-scoped Plan document identity. Payload semantics
     * remain in the document subsystem and are discriminated by kind. */
    DocumentObjectRef document;
    /* Only for subordinate slab kinds. Ephemeral collection position, not ID. */
    size_t slab_feature_index;

    WallSelection wall_member;
} EditorSelection;

/* Value-only selection; no project collection or generated allocation pointers.
 * Setters require a valid non-NONE scope and valid IDs/payload; failure clears
 * the entire selection. Wall kinds retain their established scope flexibility;
 * slab kinds are plan-only. NONE always has NONE scope; initialization and
 * clearing also zero every irrelevant payload. */
void editor_selection_set_wall(EditorSelection *selection,
    EditorSelectionScope scope, DomainId wall_id);
void editor_selection_set_opening(EditorSelection *selection,
    EditorSelectionScope scope, DomainId wall_id, DomainId opening_id);
void editor_selection_set_slab(EditorSelection *selection,
    EditorSelectionScope scope, DomainId slab_id);
void editor_selection_set_slab_feature(EditorSelection *selection,
    EditorSelectionScope scope, DomainId slab_id, EditorSelectionKind kind,
    size_t feature_index);
void editor_selection_set_roof(EditorSelection *selection,
    EditorSelectionScope scope, DomainId roof_id);
void editor_selection_set_roof_portion(EditorSelection *selection,
    EditorSelectionScope scope, DomainId roof_id, DomainId portion_id);
void editor_selection_set_document(EditorSelection *selection,
    EditorSelectionScope scope, DocumentObjectRef document);
/* Family-specific wrappers preserve call-site readability while storing the same
 * narrow document identity shape. */
void editor_selection_set_annotation(EditorSelection *selection,
    EditorSelectionScope scope, DomainId annotation_id);
void editor_selection_set_dimension(EditorSelection *selection,
    EditorSelectionScope scope, DomainId dimension_id);
void editor_selection_set_symbol(EditorSelection *selection,
    EditorSelectionScope scope, DomainId symbol_id);
void editor_selection_set_callout(EditorSelection *selection,
    EditorSelectionScope scope, DomainId callout_id);
void editor_selection_set_revision_cloud(EditorSelection *selection,
    EditorSelectionScope scope, DomainId revision_cloud_id);
int editor_selection_is_document_kind(const EditorSelection *selection,
    DocumentObjectKind kind);
int editor_selection_matches_document(const EditorSelection *selection,
    DocumentObjectKind kind, DomainId id);

void editor_selection_init(
    EditorSelection *selection
);

void editor_selection_clear(
    EditorSelection *selection
);

void editor_selection_set_wall_member(
    EditorSelection *selection,
    EditorSelectionScope scope,
    DomainId wall_id,
    WallMemberKind member_kind,
    const Timber *timber
);

bool editor_selection_is_empty(
    const EditorSelection *selection
);

/* True only for a non-empty selection in the requested valid non-NONE scope. */
bool editor_selection_matches_scope(const EditorSelection *selection,
    EditorSelectionScope scope);

/* Both context and explicit owner must match. Never infer either from navigation. */
const WallSelection *
editor_selection_get_wall_member(
    const EditorSelection *selection,
    EditorSelectionScope scope,
    DomainId wall_id
);

#endif
