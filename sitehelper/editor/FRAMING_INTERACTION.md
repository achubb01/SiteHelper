# Priority 34D — Framing Interaction & Edit Affordances

## Status

Implemented through Priority 34E on top of Priority 34A–34C.

34D established Wall-elevation hover and editable/generated affordances. 34E
adds a real selected-Opening direct-manipulation transaction without making
generated timber editable or introducing new Project authority.

## Interaction model

Wall-elevation SELECT now keeps a transient hover target beside, but separate
from, `EditorSelection`:

```text
pointer U/Z
    ↓
member hit first
    ↓ otherwise
opening clear-bounds hit
    ↓
WallElevationHover
```

Generated members use the existing by-value `WallSelection` representation.
No generated `Timber *` survives a pointer event. The application resolves the
value against the current `WallFraming` immediately before rendering. Openings
use their stable `DomainId` and explicit owning Wall ID.

Hover is cleared by pointer leave, tool/view/workspace transient invalidation,
project reconciliation and project replacement. Selection always has visual
precedence when the selected and hovered target are the same object.

## Editable versus generated

Generated framing remains inspect/select-only. A selected generated member is
identified as `Generated` in the transient detail label.

Openings are authoritative Wall definitions and already have undoable
`EditOpeningCommand` support through the property-edit architecture. A selected
opening is therefore identified as `Editable`.

34E now provides five operable grips for a selected Opening in the Framing
workspace: move, left, right, bottom and top. The grips are transient presentation
only and keep a fixed screen-pixel size as the camera zooms.

The gesture has explicit begin/update/commit/cancel semantics. Begin snapshots the
authoritative `Opening` by value. Pointer motion derives a new integer-mm candidate
from that original snapshot; it never incrementally mutates the candidate and never
mutates `Project`. Live replacement validation applies the same Wall opening rules
while excluding the opening being replaced from self-overlap checks. Release emits
exactly one existing `EDIT_OPENING` command when the candidate is valid and changed.
Escape, pointer leave and context invalidation cancel with zero model mutation.

Invalid candidates remain visible as a transient error-coloured rectangle but cannot
produce a command. Undo/redo therefore sees the entire drag as one edit rather than
a stream of geometry changes.

## Hover presentation

`AppInteractionStyle` adds a shared `hovered_colour`. Framing applies it as an
outline to a generated member, or to the authoritative opening outline and label.
The normal semantic member fill remains visible under hover, so hovering does not
erase the member-role language established in 34B.

## Local axes

`WallRenderStyle.show_local_axes` adds an optional debug-only local basis:

- `U` — positive wall-local horizontal direction.
- `Z` — positive elevation direction.

The basis is derived presentation only, uses no model state, and is disabled in
the normal SDL framing style. It exists for diagnosing wall-local transforms and
framing geometry rather than as permanent drafting chrome.

## Invariants

1. Hover is transient; selection is persistent editor interaction state.
2. Hover never stores generated allocation pointers.
3. Member hit precedence remains identical for hover and click selection.
4. Selection styling wins over hover styling.
5. Generated timber is inspectable but is not presented as directly editable.
6. Openings are identified as editable because an undoable edit command already
   exists.
7. Opening grips are shown only where a real command-backed drag transaction exists.
8. A drag previews by value; `Project` changes only once, on successful release.
9. Invalid direct-edit candidates cannot produce `EDIT_OPENING`.
10. Generated timber remains read-only even though authoritative Openings are draggable.
11. U/Z axes are opt-in diagnostics and never Project data.
