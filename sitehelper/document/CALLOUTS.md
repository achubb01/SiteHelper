# Plan leaders / callouts — Priority 28E2

Priority 28E2 adds a concrete leader/callout document object so SiteHelper can
annotate a Plan feature with authored text without putting drafting data into a
Wall, Slab, Roof or other physical domain object.

## Authority

`DocumentPlanCallout` is Project-owned non-physical authority containing:

- a stable global `DomainId`;
- explicit Storey scope;
- a fixed integer-mm target point;
- a fixed integer-mm label anchor;
- owned non-empty text.

The target and label anchor must be distinct. The straight leader segment between
them is derived presentation geometry. The current arrowhead is also derived and
fixed-pixel; it is never persisted.

The target is intentionally **not associative** in this first slice. Plan snapping
chooses the coordinate authored by the user, but the callout does not retain the
identity of a Wall, junction or other snapped object. Associative callout targets
should only be introduced once there is a clear semantic requirement and a stable
feature-reference contract.

## Persistence

Persistence format v19 adds a `callouts` section after symbols. Versions 1–18
load with an empty callout collection. Text uses the same byte-counted hexadecimal
encoding as persistent notes so spaces and line breaks are preserved without
changing the token grammar.

## Commands and history

Callouts use explicit commands:

- `CREATE_PLAN_CALLOUT`
- `EDIT_PLAN_CALLOUT`
- `DELETE_PLAN_CALLOUT`

Create/edit command payloads and edit/delete history snapshots deep-copy text.
Create undo/redo preserves the allocated callout ID; edit/delete undo restores the
exact target, label anchor and text.

## Plan editor behavior

`EDITOR_TOOL_CALLOUT` is Plan-only. Creation is a two-point + text workflow:

1. click the target;
2. click the label/text anchor;
3. type text;
4. `Enter` commits through the normal command/history path;
5. `Shift+Enter` inserts a line break and `Esc` cancels.

The first two clicks consume the existing Project-aware Plan snap result. Snapping
changes the fixed coordinate only; it does not create an association.

When the Callout tool is idle, clicking an existing leader selects it and opens
its text in the same `AppInput` focus owner. Saving unchanged text creates no
history entry. The geometry is intentionally not drag-edited in this slice,
although `EDIT_PLAN_CALLOUT` already supports transactional geometry changes for
a future properties/geometry workflow.

`EDITOR_SELECTION_DOCUMENT` stores a `DOCUMENT_OBJECT_CALLOUT` reference containing
only the stable ID. Plan SELECT overlay
precedence is currently:

1. notes;
2. callouts;
3. symbols;
4. dimensions;
5. Walls;
6. Slabs.

Within callouts, the leader segment nearest the pointer inside object-snap
tolerance wins; later-authored callouts win exact ties.

## Rendering

A persistent callout renders as the target-to-label leader, a fixed-pixel
arrowhead at the target, and screen-space debug text beside the label anchor.
Selected callouts use the existing document-overlay highlight colour. A transient
leader/arrow preview is shown after the target click and remains visible while
text is being entered.

This remains drafting presentation, not a paper-space typography or sheet model.

## Deliberately deferred

Priority 28E2 does not add elbows/multi-segment leaders, associative targets,
leader styles, text boxes, rotation, automatic labels, section/detail references,
properties-panel geometry editing, or shared annotation styling. Those should be
added only when a concrete construction-document use case establishes the needed
semantics.
