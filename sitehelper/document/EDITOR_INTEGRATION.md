# Plan note editor integration — Priority 28B

Priority 28B connects the Priority 28A plan-note authority to the existing Plan
editor without turning annotations into physical model objects.

## Selection and hit testing

`EDITOR_SELECTION_DOCUMENT` stores a `DocumentObjectRef` whose kind is
`DOCUMENT_OBJECT_NOTE` and uses `EDITOR_SELECTION_SCOPE_PLAN`. The value contains
only the stable note ID plus its document-family discriminator. The editor resolves that ID through
`SiteHelperProject` during reconciliation and clears the selection if the note is
missing, belongs to another Storey, or the selection scope is stale.

`document_plan_find_note_at_position()` is the document subsystem's narrow Plan
query. Selection tests note anchors before Walls and Slabs because notes are a
visual document overlay. The nearest anchor within the editor object-snap
tolerance wins; later-authored notes win exact ties. Selecting a note clears Wall
navigation rather than pretending that the note has a physical owner.

## Commands and history

Plan-note mutations use three explicit command types:

- `CREATE_PLAN_NOTE`
- `EDIT_PLAN_NOTE`
- `DELETE_PLAN_NOTE`

Create/edit commands own deep-copied text. History snapshots for edit/delete own
a complete annotation value. Undo/redo preserves the annotation ID, Storey
anchor, target ID and text exactly. A weak target that became unresolved remains
valid through text-only edits and history restoration.

The Project exposes transactional live note editing and a separate history
replacement primitive. Live edits require newly chosen physical targets to
resolve on the destination Storey; history replacement may restore an unresolved
weak target.

## Rendering

Plan notes render after physical Slabs/Walls so the document overlay is visible.
The current presentation is intentionally temporary: a fixed-pixel anchor marker
and the existing debug text backend, with multiline text split into screen-space
lines. Selection changes marker/text colour. No typography or paper-space model
is implied by this renderer.

## Application authoring boundary

`sitehelper_editor_create_plan_note_action()` and
`sitehelper_editor_create_edit_plan_note_action()` are the application-facing
boundaries for future text UI. Delete already flows through the normal selected-
object Delete key path.

Priority 28B deliberately does **not** add a Note toolbar tool, keyboard text
focus, properties panel, leader geometry, text styles, paper sheets, dimensions,
symbols or revision clouds. Those are separate UX/document semantics; the core
note lifecycle does not depend on choosing them now.

## Priority 28C — note authoring UI

The Plan workspace now exposes `EDITOR_TOOL_NOTE`. The tool is deliberately
Plan-only. A click is resolved by
`sitehelper_editor_prepare_plan_note_authoring()`:

- an existing note anchor within the editor object-snap tolerance is selected
  and its exact persisted anchor is returned for editing;
- otherwise the current Plan snap result is converted to authoritative integer
  millimetres and becomes the candidate anchor for a new note.

The editor still owns no text buffer. `AppInput` owns the transient keyboard
session and stores only stable IDs plus copied value state (`storey_id`, optional
`annotation_id`, weak `target_id`, and `PlanPosition`). No `DocumentAnnotation *`
is retained across events.

The Note tool uses the existing command boundary from Priority 28B. `Enter`
creates/edits through command history, `Shift+Enter` inserts a newline, and
`Esc` discards the transient edit. Re-saving an unchanged existing note closes
the session without creating a history entry. A failed command leaves the text
session intact so it can be corrected and retried.

Text input remains intentionally bounded and UI-local. The authoritative
`DocumentAnnotation` continues to own dynamically allocated persisted text; the
fixed-capacity `TextEdit` buffer is only the current desktop editing surface and
does not become a document-domain limit.

## Priority 28E1 — point-marker symbol integration

The first symbol primitive follows the same document-overlay identity boundary
without being forced into the note payload. `EDITOR_SELECTION_DOCUMENT` carries a `DOCUMENT_OBJECT_SYMBOL` reference and is
reconciled against the current Plan Storey through the common document identity query.

`EDITOR_TOOL_SYMBOL` is a one-click Plan tool. It consumes the existing
Project-aware snap result and creates a fixed integer-mm point marker through
`CREATE_PLAN_SYMBOL`; the snap source is not persisted as an association.
Create/edit/delete all flow through command history and selected symbols use the
normal Delete-selection path.

Plan SELECT checks note anchors first, then symbol anchors, then persistent
dimension geometry before physical Walls and Slabs. Point markers render as a
fixed-pixel target above physical geometry. This ordering is presentation/editor
policy only; it does not imply physical ownership between document objects.

Priority 28E3 adds `EDITOR_TOOL_VIEW_DIRECTION` without creating another
selection family. Its first click fixes a snapped anchor; its second snapped point
is reduced to a canonical primitive integer direction vector and emitted through
the same `CREATE_PLAN_SYMBOL` command path. Persistent selection/hit identity
remains the symbol anchor while fixed-pixel shaft/arrowhead geometry is derived.

The symbol editor path still does not introduce a generic annotation tool,
properties schema or symbol catalogue. Identifiers, paired section geometry and
sheet/detail destinations need their own concrete workflow before shared
abstraction is justified.


## Priority 28E2 — leader/callout integration

`EDITOR_SELECTION_DOCUMENT` carries a `DOCUMENT_OBJECT_CALLOUT` reference and is
reconciled against the current Plan Storey through the common document identity query. Callout leader segments participate in document
overlay selection after notes and before point symbols/dimensions.

`EDITOR_TOOL_CALLOUT` uses the existing Project-aware snap pipeline for a fixed
target click followed by a fixed label-anchor click. Once those two values exist,
`AppInput` becomes the sole owner of the transient text buffer. `Enter` emits a
`CREATE_PLAN_CALLOUT` or `EDIT_PLAN_CALLOUT` action, `Shift+Enter` inserts a line
break, and `Esc` discards the transient session. No document pointer is retained
across events.

An idle Callout-tool click on an existing leader selects that callout and begins
text editing. The geometry remains unchanged by that UI path in 28E2; geometry
editing is available at the command/value layer for later properties or drag
workflows. Persistent and preview rendering are described in `CALLOUTS.md`.
