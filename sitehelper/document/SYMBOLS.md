# Plan symbols — Priority 28E1 / 28E3

Plan symbols are Project-owned, non-physical document authority. They share a
stable global `DomainId`, explicit Storey scope and integer-mm Plan anchor, but
remain concrete symbol kinds rather than a universal annotation payload.

## Kinds

### Point marker — 28E1

`DOCUMENT_PLAN_SYMBOL_POINT_MARKER` is a fixed Plan target/crosshair. Its
`direction` must be `{0,0}`. One snapped click authors the anchor. The snap source
is not persisted as an association.

### View-direction marker — 28E3

`DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION` adds construction-document orientation.
It represents a Plan anchor plus a viewing/reference direction suitable as the
primitive behind later elevation/section/detail-reference conventions.

Direction authority is an exact primitive integer vector:

```
(dx, dy), non-zero

gcd(abs(dx), abs(dy)) == 1
```

A two-click authoring gesture therefore persists direction rather than pointer
length. For example, `(600, 800)` and `(3, 4)` author the same canonical `(3,4)`
direction. No floating-point angle is persisted.

This kind deliberately does **not** yet contain a detail/sheet number, section
identifier, paired cut line, printable catalogue reference or model-object
association. Those are separate semantics and should be introduced only with a
real workflow that owns them.

## Identity and Storey scope

Symbols are document objects, not physical Storey children.
`sitehelper_project_find_owning_storey*()` therefore does not report physical
ownership for them. `storey_id` scopes presentation and editing only.

`EDITOR_SELECTION_DOCUMENT` stores a `DOCUMENT_OBJECT_SYMBOL` reference for both
concrete kinds; the selected identity remains one stable symbol ID.
Create/edit/delete use the existing `CREATE_PLAN_SYMBOL`, `EDIT_PLAN_SYMBOL` and
`DELETE_PLAN_SYMBOL` command family, so undo/redo preserves identity across
symbol kinds.

## Authoring and selection

`EDITOR_TOOL_SYMBOL` remains the one-click point-marker tool.

`EDITOR_TOOL_VIEW_DIRECTION` is a Plan-only two-click tool:

1. click the anchor using the ordinary Project-aware Plan snap pipeline;
2. click a second snapped Plan point to define direction;
3. reduce the integer delta to its canonical primitive vector;
4. emit `CREATE_PLAN_SYMBOL` with `DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION`.

`Esc`, Storey/view/tool changes and authoritative history changes cancel any
partial direction gesture through the normal editor transient-state boundary.

Both kinds participate in the existing symbol anchor query. The nearest anchor
within object-snap tolerance wins; later-authored symbols win exact distance
ties. Direction-marker presentation is fixed-pixel, so persistent hit identity
remains the stable anchor rather than making camera scale part of document
query authority.

## Rendering

Point markers render as the existing fixed-pixel target/crosshair.
View-direction markers render as a fixed-pixel diamond at the authoritative
anchor plus an oriented shaft and arrowhead derived from the canonical direction.
Only the anchor and direction are persisted; presentation length and arrowhead
size are not.

The transient second-point preview uses the same marker rendering path before a
command is created.

## Persistence

Persistence v18 introduced the `symbols` collection and point-marker records.
Persistence v20 extends that collection with `view_direction` records containing
`direction dx dy`. Versions 18–19 continue loading their point-marker records
unchanged; versions 1–17 continue loading with an empty symbol collection.

## Still deliberately deferred

28E3 does not turn `DocumentPlanSymbol` into a catalogue/database object and does
not introduce generic styles, arbitrary symbol geometry, sheet/detail targets,
section-line pairing, north/project orientation or a universal annotation base
class. Priority 28F should review the concrete repetition across notes,
dimensions, point/view symbols and callouts before extracting shared machinery.
