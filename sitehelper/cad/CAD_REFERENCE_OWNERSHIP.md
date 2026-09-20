# Priority 30E — Transactional CAD plan-reference ownership

Priority 30E gives the mapped Plan reference from 30D a live-project lifecycle
without turning imported CAD geometry into construction authority.

The ownership chain is now:

```text
ASCII DXF
    -> 30C CadIrDocument
    -> 30D CadPlanReference proposal
    -> 30E explicit adopt/replace
    -> SiteHelperProject ancillary reference collection
          keyed by Storey DomainId
```

## 1. Project-owned adjunct, not a Storey domain child

A reference belongs to a project and is scoped to one existing Storey, because
Plan display/tracing needs a definite level. The first workflow allows **at most
one CAD plan reference per Storey**.

The collection lives on `SiteHelperProject` rather than inside `Storey`.
This is deliberate:

- `Storey` remains a construction-model type and does not acquire a CAD-library
  or CAD-mapping dependency;
- the `sitehelper_model` -> CAD -> Project dependency cycle is avoided;
- reference geometry is visibly ancillary rather than another physical Storey
  child beside Walls/Slabs/Roofs; and
- future non-DXF adapters can still produce the same `CadPlanReference` proposal.

The attachment's `storey_id` is an existing SiteHelper identity used only as
scope. The reference itself and every reference path have **no `DomainId`**.
DXF handles remain opaque provenance strings.

## 2. Explicit ownership transfer

`sitehelper_project_adopt_cad_plan_reference()` accepts only a
`CAD_PLAN_REFERENCE_READY` proposal for a live Storey.

On success:

- the Project takes all owned strings, paths, provenance and diagnostics;
- the caller's `CadPlanReference` is reset to the empty initialized state;
- an existing reference for that Storey is destroyed only after the replacement
  has been installed; and
- the Project DomainId allocator is unchanged.

On failure the caller retains the complete proposal and the live Project retains
its previous reference unchanged.

Adding the first attachment may allocate collection storage. Allocation failure
is therefore a normal explicit apply result and does not consume the proposal.
Replacing an existing Storey reference requires no collection growth.

`sitehelper_project_clear_cad_plan_reference()` is allocation-free and destroys
the owned reference immediately.

Project destruction destroys every attached reference.

## 3. Import is atomic with respect to the live project

30E deliberately does not add a DXF-specific `import_into_project()` shortcut.
The concrete transaction is already naturally staged:

```text
CadIrDocument decoded = temporary
CadPlanReference mapped = temporary

if decode failed:
    live Project unchanged
if mapping blocked/failed:
    live Project unchanged
if adopt failed:
    live Project unchanged; mapped proposal still owned by caller
if adopt succeeded:
    exactly one Storey reference is created/replaced
```

This keeps the Project API independent of DXF. A future DWG/IFC/other adapter can
reuse the same final application boundary without Project knowing which format
produced the proposal.

Semantic CAD-to-Wall/Room/Slab conversion remains a separate future workflow and
will need its own candidate-domain transaction rather than reusing reference
attachment as a back door.

## 4. Validation boundary

`sitehelper_project_validate()` continues to validate authoritative project
state. It does not use imported reference paths to validate Walls, Rooms, Slabs,
Roofs, topology, dimensions or generated framing.

30E lifecycle APIs maintain the ancillary collection invariant themselves:

- count does not exceed capacity;
- nonzero capacity has storage;
- each attachment is created only for an existing Storey; and
- one attachment exists per Storey when using the public API.

A CAD reference therefore cannot make valid construction geometry valid or
invalid merely by existing.

## 5. Rendering and selection boundary

30E intentionally adds no renderer or editor selection behavior.

A future Plan renderer may query
`sitehelper_project_find_cad_plan_reference(project, storey_id)` and draw its
paths as a background/reference layer. That renderer must not turn individual
paths into construction selections.

The ordinary `EditorSelection` remains unchanged. There is no CAD-reference
selection kind and no path identity to reconcile. If later UX needs selecting a
reference resource (for hide/show/reposition/reload), that should select the
**reference attachment/resource**, not pretend each DXF line is a SiteHelper
domain object.

Reference paths also do not participate in snapping in 30E. Opting into CAD
reference snapping is a later explicit editor policy because it changes drawing
behavior even though it does not change authority.

## 6. Persistence decision

30E **does not persist mapped CAD references yet** and does not bump the current
SiteHelper project format.

That omission is intentional rather than accidental. The project does not yet
have a reference-resource contract covering source file identity/path,
embedded-vs-linked data, reload behavior, missing-file handling, provenance
fingerprints or whether import diagnostics are durable project data. Freezing a
large embedded path dump into SiteHelper persistence before answering those
questions would make later external-reference behavior harder to design.

For 30E, attached references are therefore an in-memory project/session adjunct.
Saving construction authority remains unchanged; reopening a project requires
re-importing the reference.

When persistence is introduced, it must be an explicit later format version and
must state whether the reference is embedded, linked, or both. External CAD
files must never become an alternative SiteHelper project-save format.

## 7. Command history decision

Reference attach/replace/clear is not added to construction command history in
30E. Import is an external-resource operation and there is not yet a persistent
reference resource identity to make history restoration robust across reloads.

If resource editing later becomes persistent UI state (visibility, transform,
reload, multiple references), it can receive a dedicated history policy then.
This does not affect undo/redo for authoritative construction commands.

## 8. 30E acceptance cases

The dedicated tests prove:

- adopt transfers ownership and empties the caller proposal;
- one reference is scoped to the requested Storey;
- replacing a Storey reference destroys the old payload and keeps one entry;
- clearing destroys/removes the attachment;
- reference lifecycle never advances the global DomainId allocator;
- blocked-unit proposals are rejected without replacing a live reference;
- missing-Storey apply is rejected without consuming the proposal;
- decode and mapping failure leave the live reference unchanged;
- first-attachment allocation failure leaves both Project and proposal intact;
- project construction validation remains valid with/without a reference; and
- project destruction owns cleanup.

## 9. Next boundary

With ownership fixed, Priority 30F can address **export policy/representation**
without treating DXF as persistence. A separate follow-up can add Plan rendering
or a durable reference-resource contract if that provides more immediate user
value than export.
