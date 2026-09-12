# Plan snapping

`editor_snap()` remains the generic, caller-space resolver. In Plan view the
editor supplies transient millimetre candidates from the active Storey's visible
physical Walls through `plan_collect_snap_candidates`:

- endpoints come from the authoritative ordered `WallDefinition.segment`;
- centreline points are pointer projections through the existing wall-plan U
  transforms, clamped to the finite segment, never samples along an infinite line;
- intersections come only from `plan_topology_build` and `wall_junctions_build`,
  supplied with physical Wall sources. All their junction kinds participate.
  Exact rational positions are converted to `double` only here, at the transient
  editor boundary; fractional junctions are not rounded to `PlanPosition`.

The producer scans every relevant Wall/junction and emits at most **three**
candidates: the nearest enabled point per type. Candidates have no source
identity or type-internal priority, so no other point of that type can affect the
resolver. Equal squared distances within a type choose the lexicographically
smallest `(x,y)`, independent of Wall storage order. There is no 256-candidate
Plan truncation. Cross-type distance ties prefer intersection, endpoint, then
wall centreline. Grid keeps its existing resolver-owned fallback semantics:
an enabled object within tolerance wins even if a grid point would be closer.
Centreline snapping defaults on and has a purple snap cursor.

`sitehelper_editor_update_snap_in_project` resolves the current active Storey.
Project-aware pointer and primary-action entry points resolve once at the actual
event position, then internal tool consumers use that result. Wall and Measure
share it for starts, previews and clicks. Neither tool gathers project geometry
or recomputes grid-only snapping. Low-level calls without Project context remain
grid-only in Plan; elevation still uses `wall_collect_snap_candidates` for
generated framing in Wall-local U/Z millimetres, with its existing semantics.

The topology dependency is private and conditional on `SITEHELPER_BUILD_TOPOLOGY`.
When disabled, endpoint/centreline/grid snapping works unchanged. When enabled,
topology and junctions are rebuilt from current geometry for each update; there
is no stale cache or persisted snap state. Allocation failures and unsupported
topology (including collinear overlaps, nesting and non-simple faces) omit only
intersection candidates for that update. Large projects can still incur the
existing topology builder's computation/allocation cost; candidate reduction
removes output truncation, not that cost.

Measurement retains fractional Plan points. Walls retain their existing integer
millimetre representation and tool conversion (fractional coordinates truncate
toward zero at command creation); integral snap targets are representable exactly.
An arbitrary fractional centreline/junction cannot become an exactly coincident
authoritative Wall endpoint under this contract. This priority does not change
that representation or conversion policy. See [measurement semantics](../model/MEASUREMENTS.md).

Snapping is a read-only editor query: no Project mutation, DomainId allocation,
command/history entry or persistence change. Normal Wall commits still create
ordinary commands. Tool/view/Storey changes and transient invalidation retain
their established lifecycle.

RoomSeparators are invisible and excluded even from topology input. Room location
points are not corners; guides have no model; openings have no visible Plan
geometry. Their snap semantics, persistent constraints and associative dimensions
are intentionally deferred. Additional visible geometry can supply candidates
without introducing another resolver.
