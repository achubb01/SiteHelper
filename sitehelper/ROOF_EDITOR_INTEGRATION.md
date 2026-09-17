# Priority 26G4 — Roof Project / Editor Integration Hardening

## Status

Complete.

G4 connects the production roof source domain to read-only Plan queries and the
editor's value-only selection/reconciliation model without introducing a roof
renderer or silently changing the existing Plan SELECT precedence.

## Plan query boundary

`roof_plan_hit_test_storey()` operates on one resolved Storey and authoritative
`RoofPortionDefinition.support_vertices`. It returns stable Roof and portion IDs,
never collection pointers or generated-plane indices. Later appended Roofs and
portions win overlaps so the result is deterministic.

This is deliberately an **authoring/source** query. It does not pretend the
support polygon is the final visible eave envelope. Overhang/projection-aware
visual hit testing belongs with the future exact derived-topology/render layer.

## Editor selection

`EditorSelection` now supports `EDITOR_SELECTION_ROOF` and
`EDITOR_SELECTION_ROOF_PORTION`. Both are Plan-only and store IDs only. A portion
selection carries both its Roof owner ID and its own portion ID.

`sitehelper_editor_reconcile()` resolves those IDs fresh against the current
Storey. Deleting a Roof, deleting a selected portion, switching Storeys, changing
view, or replacing/loading the Project therefore cannot leave a borrowed Roof or
portion pointer in editor state.

`sitehelper_editor_select_roof_at_position()` is an explicit hook for a future
roof workspace/tool. Ordinary Plan SELECT still keeps its existing wall/slab
precedence. A roof commonly overlaps a slab over essentially the whole building;
choosing an implicit precedence here would make one domain unnecessarily hard to
select.

## Prototype bridge decision

G4 does **not** rename `RoofPrototypeGeometry` into a production topology API.
26E3 already proved `RoofPrototypeBoundary` cannot exactly represent rational
clipped boundary endpoints. Promoting that type now would freeze a known-bad
contract.

The production source domain and persistence are final enough to use. The
remaining `roof_build_derived_geometry()` bridge is intentionally retained and
isolated until a later exact-derived-topology priority promotes rational plane
polygons, boundaries and intersections together. Prototype executable tests stay
as executable evidence until that replacement has equivalent fixture coverage.

## Exit criteria

G4 is complete when:

1. Storey roof source polygons can be queried without generating/storing derived
   state;
2. roof and roof-portion editor selections contain stable IDs only;
3. reconciliation clears stale/missing Roof or portion identity;
4. Project replacement/view/Storey transitions preserve the existing transient
   selection rules;
5. ordinary Plan selection precedence is not changed accidentally by roof/slab
   overlap; and
6. the transitional derived-geometry bridge remains explicit rather than being
   promoted into a known-inexact production topology contract.

With G4 complete, Priority 26G and the roof-domain promotion phase are complete.
