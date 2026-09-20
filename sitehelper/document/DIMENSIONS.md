# Persistent plan dimensions — Priority 28D1–D6

A persistent dimension is document authority, not physical model authority and
not a cached measurement.

```text
DocumentPlanDimension
├── id                 global stable DomainId
├── storey_id           explicit Plan-view scope
├── first               typed geometry reference
├── second              typed geometry reference
└── offset_mm           signed drawing-placement offset
```

The displayed/measured value is derived every time from `first` and `second`.
No distance field is persisted. Physical geometry edits therefore update an
associative dimension automatically.

## References

`FIXED_POINT` contains an integer-mm `PlanPosition` and no target ID.
`WALL_START` / `WALL_END` contain the Wall's stable ID and resolve to the
semantically named endpoint of its authoritative `WallPlanSegment`.

The association is weak: missing physical target IDs are legal persisted state.
This avoids coupling Wall deletion to document deletion and permits undo to
restore the same identity. A live reference must resolve to a Wall on the
Dimension's Storey.

## Numeric semantics

Resolved distances use `wall_plan_segment_length_mm()`: nearest whole
millimetre using the same physical-plan convention as Walls. Coincident resolved
points are not a valid live dimension. `offset_mm` affects future drafting
placement only; it never affects measurement.

## Commands, history and selection

Priority 28D3 adds value-only `CREATE_PLAN_DIMENSION`, `EDIT_PLAN_DIMENSION` and
`DELETE_PLAN_DIMENSION` commands. Create redo restores the originally allocated
DomainId; edit/delete history snapshots copy the complete dimension authority so
undo can restore weak references even when their physical target is currently
missing. Live create/edit still require resolvable same-Storey references.

Editor selection uses the Plan-only `EDITOR_SELECTION_DOCUMENT` kind with a
`DOCUMENT_OBJECT_DIMENSION` reference storing only the stable dimension ID. Reconciliation resolves that ID through the
Project and clears it if the dimension is missing or belongs to another Storey.
Priority 28D4 derives one shared drafting geometry from resolved endpoints plus `offset_mm`: two extension lines, the offset dimension line and its midpoint. Rendering and hit testing both use this geometry so visible and selectable geometry cannot drift apart. Unresolved weak-reference dimensions remain valid document authority but are neither rendered nor pickable until their references resolve again.

Plan SELECT precedence is notes, then dimensions, then physical Walls/Slabs. Dimension picking measures world-mm distance to the displayed dimension line and extension lines using the editor object-snap tolerance; later-authored dimensions win exact ties. Selected dimensions use the existing document-selection highlight colour. Rendering adds fixed-pixel terminal ticks and a derived `<distance> mm` label; the label is presentation only and is never persisted.

## Deferred

Editable dimension text, chained dimensions, angular/radial measurements,
paper-space styles, and slab/roof feature references remain deferred.

Slab and roof source vertices are specifically deferred because an array index
is not yet a durable semantic feature identity. Adding such references requires
a feature-addressing contract that remains meaningful across geometry edits.

## 28D5 authoring workflow

The Plan workspace exposes `EDITOR_TOOL_DIMENSION`. Authoring is a three-click
interaction:

1. choose the first measured reference;
2. choose the second measured reference;
3. choose the signed perpendicular drafting offset.

The first two clicks use the ordinary Plan snap pipeline. A unique wall endpoint
snap is captured as `DOCUMENT_DIMENSION_WALL_START` or
`DOCUMENT_DIMENSION_WALL_END`, so later wall geometry edits move the measured
point. A snapped coordinate shared by multiple wall endpoints is intentionally
captured as `DOCUMENT_DIMENSION_FIXED_POINT`: the current snap result carries no
source identity, and silently choosing one wall at a junction would create an
unstable semantic association. A future disambiguation UI may make that choice
explicit.

The third click does not object/grid snap. It projects the raw Plan pointer onto
the signed perpendicular to the measured segment and rounds that drafting offset
to integer millimetres. The preview and final persistent geometry therefore use
the same `document_plan_dimension_geometry()` contract.

`Esc` cancels an in-progress dimension without Project mutation. Successful
creation executes `CREATE_PLAN_DIMENSION` through normal command history and
resets the tool to its first-reference stage, ready for another dimension.
## 28D6 hardening

Dimension authoring treats context changes as cancellation boundaries. Storey,
view and tool changes, Project replacement, and successful history reconciliation
clear any partially picked references while preserving the active Dimension tool
when that tool remains valid in Plan view. `Esc` remains the explicit local cancel.
This prevents a transient reference picked under one authoritative context from
being committed after that context changes.

Picking is deterministic under overlap: nearest visible drafting geometry wins;
later-authored dimensions win exact distance ties. An associative dimension whose
weak Wall target disappears remains valid document authority but is unresolved,
so it is hidden and unpickable. Restoring the same Wall ID makes it resolve, render
and pick again without modifying the dimension.

An object snap at a coordinate that uniquely names one Wall endpoint may preserve
that endpoint association for both `ENDPOINT` and `INTERSECTION` snap results.
The latter matters at T-junctions because intersection has higher snap precedence.
Shared corners with multiple endpoints and true crossings with no endpoint remain
fixed points. Grid/centreline snaps do not create endpoint associations.

The transient tool rejects inconsistent fixed-reference/preview pairs and measured
spans whose rounded millimetre distance is outside the persistent `int` distance
contract. Offset pointer updates are transactional: an unrepresentable extreme
offset cannot partially replace the last valid preview state.
