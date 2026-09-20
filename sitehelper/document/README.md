# Document / annotation layer — Priorities 28A–28G1

The document layer is authoritative **non-physical** project information. It is
owned by `SiteHelperProject`, alongside the Storey hierarchy rather than inside
Walls, Rooms, Slabs or Roofs. Structural objects therefore do not acquire note,
label, drafting or presentation fields.

Priority 28A intentionally introduces one small production payload: a plan note.
A note has a stable global `DomainId`, a Storey-plan anchor (`PlanPosition` in
integer millimetres), text, and an optional stable target ID. The target is a weak stable-ID association, not ownership. Live note creation
requires a physical model object on the same Storey. If that model object is
later deleted, the note remains valid with an unresolved target ID; undo/history
can restore the same model ID and the association becomes live again without a
cross-domain cascade. An association may never resolve to another annotation or
to a live object on a different Storey. A free-standing note uses
`DOMAIN_ID_INVALID`. No raw model pointer is retained.

The document collection itself is Project-owned. `sitehelper_project_contains_domain_id`
therefore includes annotation IDs, while `sitehelper_project_find_owning_storey`
continues to mean physical containment and deliberately returns no owner for a
project-owned annotation. Callers use the annotation's explicit Storey scope.
This preserves the Priority 27 distinction between identity/ownership queries and
feature-specific interpretation.

Plan notes persist as project authority. Their text uses a byte-counted save
record so spaces and line breaks are not forced into the persistence token
grammar. The core currently treats text as a non-empty C string; typography,
rich text, encoding policy and display formatting are presentation concerns.

## Priority 28B editor integration

Plan notes now have a narrow Plan-editor integration: anchor hit testing, stable-ID
selection/reconciliation, fixed-pixel overlay rendering, and explicit create/edit/
delete commands with undo/redo. Application-facing editor action helpers accept
text supplied by a future UI; Priority 28B does not introduce a general text-entry
mode or Note toolbar tool. See `EDITOR_INTEGRATION.md`.

## Deliberately deferred

The document layer still does **not** define a printable sheet/document hierarchy,
viewports, paper coordinates, production typography/text styles, semantic
section/elevation sheet references, richer associative constraints,
automatic labels, or annotation property panels. Those features need their own geometry and lifecycle
semantics. In particular, the existing Measure tool remains transient and is not
silently converted into an annotation.

Persistent dimensions use an explicit typed reference model; future reference kinds must preserve the same stable-semantic rule. Priority 28E1 adds a concrete point-marker symbol payload, while semantic construction symbols and revision graphics still need their own models.

## Priority 28D1–D2 persistent dimension foundation

Persistent plan dimensions are a second Project-owned document payload, separate
from notes. `DocumentPlanDimension` owns a global stable `DomainId`, explicit
Storey scope, two typed references and a signed drawing offset. It does **not**
store a measured number: `sitehelper_project_resolve_plan_dimension()` derives
the current nearest-whole-millimetre value from the referenced authoritative
geometry.

The initial persistent reference vocabulary is deliberately small:

- `DOCUMENT_DIMENSION_FIXED_POINT` stores an integer-mm `PlanPosition`.
- `DOCUMENT_DIMENSION_WALL_START` associates with the physical start endpoint of
  a Wall by stable ID.
- `DOCUMENT_DIMENSION_WALL_END` associates with the physical end endpoint of a
  Wall by stable ID.

Wall references are weak. Deleting a referenced Wall leaves the dimension valid
but unresolved, allowing undo/history to restore the same Wall ID and therefore
the association. Creation requires live same-Storey Walls. A live ID belonging
to another object type or another Storey is invalid.

Slab/roof vertices are intentionally not represented by collection indexes in
this contract. Their indexes are not yet stable feature identities and can change
meaning after source-geometry replacement. Associative dimensions should only be
extended to those domains after a feature-reference contract exists that cannot
silently retarget a dimension.

Persistence format v17 adds a `dimensions` section after notes. Versions 1–16
load with an empty dimension collection.


## Priority 28E1 / 28E3 — Plan symbols

`DocumentPlanSymbol` now has two concrete kinds while retaining one stable-ID
lifecycle. `DOCUMENT_PLAN_SYMBOL_POINT_MARKER` is the original fixed-anchor
target. `DOCUMENT_PLAN_SYMBOL_VIEW_DIRECTION` adds an exact canonical integer
direction vector for oriented construction/document references. It is authored
with two snapped Plan clicks and renders as a derived fixed-pixel directional
marker; pointer distance is not persisted as symbol authority.

Persistence v18 introduced the symbol collection and point markers. Persistence
v20 extends it with `view_direction` records while v18–v19 point-symbol files
remain compatible. Both kinds use the existing create/edit/delete command family,
stable-ID selection and anchor query. See `SYMBOLS.md`.

Identifiers, section cut lines, detail/sheet destinations and symbol catalogues
remain deliberately deferred rather than guessed into the shared payload.


## Priority 28E2 — Plan leaders / callouts

Priority 28E2 adds `DocumentPlanCallout` as a fourth Project-owned document
payload. It stores stable identity, Storey scope, a fixed target point, a fixed
label anchor and owned text. Persistence format v19 adds a `callouts` section;
versions 1–18 load with no callouts.

The target and label are fixed integer-mm Plan coordinates. Existing Plan snapping
helps author them but does not create an associative constraint. The leader line
and arrowhead are derived presentation geometry. Create/edit/delete commands,
undo/redo, stable-ID selection, leader hit testing, rendering and the two-click +
text authoring workflow are described in `CALLOUTS.md`.

Callouts deliberately remain separate from notes and symbols. Their two-point
geometry plus owned text gives Priority 28F a more useful set of real repetition
to review before any common annotation abstraction is extracted.


## Priority 28F — architecture review

Priority 28F reviewed the concrete repetition across notes, dimensions, symbols
and callouts. The only common production abstraction extracted is document
identity/context: `DocumentObjectRef` (`DocumentObjectKind` + stable `DomainId`)
and a narrow Storey-scope resolver. The editor now uses one Plan-only
`EDITOR_SELECTION_DOCUMENT` payload for all four families.

Authoritative payloads, typed commands/history, hit geometry and persistence
remain family-specific because their semantics are materially different. Generic
style/layer, sheet/paper-space and authoring abstractions remain deferred until a
real feature requires them. See `DOCUMENT_LAYER_REVIEW.md`.


## Priority 28G1 — revision cloud markup

Priority 28G1 adds a fifth concrete document family: `DocumentPlanRevisionCloud`.
It stores a stable ID, explicit Plan Storey scope and an implicitly closed array
of fixed integer-mm boundary vertices. Scalloped cloud edges are derived
presentation and are not persisted; hit testing uses the authoritative closed
polyline. The Plan `Rev` tool authors snapped vertices and Enter commits through
typed create/edit/delete history. Persistence v21 added the `revision_clouds`
section; versions 1–20 load with no revision clouds. See `REVISION_MARKUP.md`.


## Revision lifecycle grouping (Priority 28G2)

Project-level `DocumentRevision` records provide stable revision identity plus a
human identifier and optional description. Revision clouds may weakly reference
a revision ID, allowing multiple clouds (including across Storeys) to belong to
the same revision without making lifecycle metadata Plan geometry. Persistence
v22 stores revision records and cloud links. See `REVISION_LIFECYCLE.md`.
