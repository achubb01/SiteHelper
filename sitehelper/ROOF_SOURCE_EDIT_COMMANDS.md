# Priority 26G2B — Roof Source-Edit Commands and History

Priority 26G2B places authoritative Roof source editing behind the same
transactional command/history boundary established by 26G2A. It edits only
persistable source authority. Generated planes, ridges, hips, valleys, seams,
interfaces and structural-layout snapshots remain derived and are never stored
in command history.

## One command family, explicit edit kinds

`RoofSourceEditCommand` is one owned command family with explicit operations:

```text
ADD_PORTION_COMPOSED
REMOVE_PORTION
SET_PORTION
SET_COMPOSITION
REMOVE_COMPOSITION
SET_TERMINATION
REMOVE_TERMINATION
```

ADD and SET portion commands deep-own their support polygons. Wrapping them in a
`SiteHelperCommand` clones that ownership again for history, so caller lifetime
is independent from command-history lifetime.

The command result always carries the Roof ID. Operations that target or create
a portion also return that portion ID. Relationship-only operations may return
an invalid portion ID because compositions and terminations are value
relationships rather than globally identifiable entities.

## Transactional project mutations

Every production mutation follows the same boundary:

```text
current Roof authority
        ↓ deep clone
candidate authority
        ↓ mutate source only
roof_validate()
        ↓
roof_build_derived_geometry()
        ↓ success only
commit candidate
```

Failure destroys only the candidate. The live Roof, global DomainId watermark
and history cursor remain unchanged.

`REMOVE_PORTION` also removes compositions and terminations that refer to the
removed portion before validating the candidate. Removing the final portion is
invalid because a Roof cannot exist without source geometry.

A composition pair is unique regardless of endpoint order. `SET_COMPOSITION`
updates the existing pair if present; it does not create a second relationship
for `A-B` when `B-A` already exists. A termination is unique by `(portion ID,
RoofEnd)`. `SET_TERMINATION` updates that key when present.

## Exact identity and history snapshots

History captures one deep pre-edit `Roof` snapshot for every 26G2B command.
Undo does not blindly overwrite the live Roof. It first reconstructs the exact
expected post-command authority from:

```text
pre-edit Roof snapshot
+ owned command payload
+ result portion ID, when ADD allocated one
```

Undo proceeds only if the current live Roof exactly equals that expected
post-state. External or out-of-history mutation therefore causes undo to fail
closed without moving the history cursor.

After verification, undo uses `sitehelper_project_replace_roof()` to restore the
pre-edit authority. That replacement preserves the Roof ID and all historical
portion IDs. Any portion identity that needs to re-enter the Roof must be below
the current allocator watermark and globally free outside the Roof.

Redo first requires the current live Roof to exactly equal the captured pre-edit
snapshot. Non-allocating edits simply execute again. `ADD_PORTION_COMPOSED`
uses `sitehelper_project_restore_roof_portion_composed()` so the original newly
created portion DomainId is reused exactly; redo never allocates a substitute
identity and never rewinds or advances the watermark.

## Source operations and invariants

`SET_PORTION` can replace support geometry, generation intent, fixed-point slope,
vertical reference, direction and single-slope datum ownership while preserving
the portion DomainId.

`SET_COMPOSITION` / `REMOVE_COMPOSITION` edit explicit relationship authority.
A removal that would leave the current compound geometry unresolved is rejected
by regeneration rather than committing disconnected source state.

`SET_TERMINATION` / `REMOVE_TERMINATION` edit primitive termination intent.
Derived hip/ridge/cut geometry is regenerated and is not copied into history.

These operations deliberately do not edit structural-layout intent. Priority
26F proved that structural strategy is a separate layer; its eventual production
commands should not be mixed into envelope source editing.

## Persistence boundary

Priority 26G3 advances project persistence to v15 and serializes the authoritative
Roof/portion/composition/termination state manipulated by these commands.
Generated geometry remains excluded and is regenerated after load. See
`ROOF_PERSISTENCE.md`.

## Completion boundary

With 26G2A and 26G2B complete, production Roof lifecycle and source authority are
fully command/history aware. Priority 26G3 now persists that command-stabilized
authority in format v15 rather than prototype or derived structures.
