# Revision / review markup — Priority 28G1

Priority 28G1 adds the first production review-markup payload: a Plan revision
cloud. Revision markup is non-physical Project authority and remains separate
from construction model objects and from permanent note/dimension/symbol/callout
semantics.

## Authoritative state

`DocumentPlanRevisionCloud` owns:

- a globally stable `DomainId`;
- explicit Plan Storey scope;
- three or more authored integer-millimetre `PlanPosition` vertices.

The vertex sequence is an implicitly closed boundary. The first vertex is not
repeated at the end. Adjacent duplicate vertices and an explicit repeated first
vertex are invalid. Self-intersection and collinearity are intentionally not
rejected: a revision cloud is markup, not a physical polygon or topology source.

Snapping assists authoring, but the persisted vertices are fixed coordinates.
Revision clouds do not become associated constraints to Walls, Slabs, junctions
or other document objects.

## Derived presentation and picking

The familiar scalloped cloud edge is presentation only. Rendering derives
fixed-screen-size lobes from each authoritative boundary segment so zoom does
not change the persisted geometry or introduce paper-space style data before a
sheet/print model exists.

Plan picking uses distance to the authoritative closed polyline, not to the
screen-derived scallops. That keeps document queries independent from camera
scale. Later-authored clouds win exact hit-distance ties.

Revision clouds are the top document overlay in current Plan selection policy:

```
revision clouds
notes
callouts
symbols
dimensions
walls
slabs
```

## Authoring

The Plan `Rev` tool is a polygon-style transient workflow:

1. snapped clicks append fixed vertices;
2. moving the pointer previews the next edge and implicit closure;
3. Enter commits once at least three vertices exist;
4. Esc cancels the in-progress boundary without history mutation.

Clicking the first vertex again after a valid three-point sketch does not append
a duplicate closure vertex; Enter remains the explicit commit.

Create/edit/delete use typed revision-cloud commands and exact deep-owned history
snapshots. Stable identity survives undo/redo.

## Persistence

Persistence format v21 introduced the `revision_clouds` collection. Priority
28G2 advances the format to v22: project-level revision records are serialized
before clouds and each cloud carries an optional weak revision ID. v21 clouds
load as unassigned. See `REVISION_LIFECYCLE.md`.

The scallop presentation is never serialized.

## Lifecycle boundary

Priority 28G2 adds reusable revision identifiers/descriptions and cloud grouping,
but deliberately defers author/date, issue status, approvals and sheet issue
semantics until those domains exist.
