# Derived plan topology, Room resolution and wall junctions (Priorities 7D–8)

`sitehelper_topology` is an on-demand, read-only query over ordered physical
Wall and virtual RoomSeparator segments. It produces an owned temporary result;
it is not project state, persistence, history, an editor cache, or Room topology.
The Storey adapter reads only source collections, IDs and segments. It ignores
Rooms, Room placement, openings, framing, settings and the allocator watermark.

## API and ownership

`plan_topology_build(sources, count, output)` builds from `PlanTopologySource[]`.
`plan_topology_build_from_storey(storey, output)` copies authoritative source
geometry before invoking the same builder. `output` must be zero-initialized or
an existing successful result. Success destroys/replaces that result. Failure
preserves all its arrays and values. Neither entry point mutates input.
`plan_topology_destroy` frees every result allocation and zeros the structure;
NULL and repeated destruction of zero state are safe. No result pointer borrows
project or temporary workspace storage. All partial allocations use one cleanup
path. Results must not be shallow-copied and independently destroyed.

`PlanTopologyResult` returns the first deterministic `PlanTopologyCode` and,
where applicable, source/related DomainIds. Codes distinguish success, invalid
arguments/sources, allocation failure, numeric overflow, unsupported overlap,
unsupported nesting and unsupported non-simple bounded faces. These errors do
not imply that authoritative project validation should reject the project.

## Exact arithmetic audit

Authoritative coordinates are signed integers no wider than 32 bits, enforced
by a compile-time assertion. Let B = 2^31 and M = 2^32 - 1. For every source:

| Quantity / operation | Absolute magnitude bound |
| --- | --- |
| Source coordinate | <= B |
| Difference between source coordinates; ray vector; query offset | <= M (fits int64_t) |
| Product in a 2D cross product | <= M^2 < 2^64 |
| Cross product (difference of two products) | <= 2 M^2 < 2^65 |
| Intersection denominator D, after sign normalization | 0 < D < 2^65 |
| Accepted source parameter numerator T or U | 0 <= T,U <= D |
| Coordinate term origin * D | < 2^96 |
| Coordinate term delta * T | < 2^97 |
| Sum of coordinate terms before reduction | < 2^98 |

`topology_cross` is only called with the bounded integer differences above,
including angular comparisons and component-containment supporting-line tests.
Its unchecked signed 128-bit product/subtraction is therefore safe. Negating a
cross product or ray vector is safe under the same bounds. Coordinates are
constructed only after both source parameters pass the [0,D] intersection test.
Checked 128-bit multiplication/addition still guards their construction, returning
`PLAN_TOPOLOGY_NUMERIC_OVERFLOW` if the established bounds ever change.
No new intersections are computed from already-derived rational coordinates.

`PlanTopologyRational` exposes a sign and two unsigned 128-bit magnitudes, each
encoded as two portable uint64_t limbs (`lo + hi * 2^64`). A Euclidean GCD reduces
numerator/denominator; zero is nonnegative 0/1. Unsigned magnitude conversion
also safely handles the minimum signed 128-bit integer. Denominators are always
positive. Reduction can only shrink the builder bounds above. In fact an accepted
intersection is within source coordinate bounds, so its unreduced numerator is
also bounded by B*D, although construction uses the looser intermediate bound.

Cross-multiplying rational coordinates for comparison could require more than
128 bits. `topology_rational_compare` instead compares integer quotients, then
reciprocates nonzero remainders, reversing comparison direction at each step.
Equal quotients reduce comparison to the fractional remainders; reciprocation
reverses their order. A zero remainder terminates before any zero denominator
can arise. All operations are unsigned division/remainder on the original
128-bit magnitudes or smaller values. Thus comparison and lexicographic vertex
ordering remain exact even for denominators wider than 64 bits, and indeed for
all canonical rationals representable by this API. No floating-point operation
participates in production construction, ordering, orientation or identity.

Array size products, growth, count addition and directed-edge doubling are
checked separately. Allocation exhaustion returns allocation failure; size
arithmetic overflow returns numeric overflow. Within signed 32-bit source
coordinates, geometry arithmetic itself fits the proven bounds. Tests exercise
full-range inputs, wide reduced denominators, near-limit comparisons, signed
minimum normalization, checked arithmetic overflow and impossible array sizes.

## Compiler portability

The topology API and algorithms use only the private numeric abstraction. GCC
and Clang retain their native 128-bit implementation; MSVC x64 uses a two-limb
128-bit implementation with `_umul128` for the one word multiply primitive.
Neither topology source nor its public API contains compiler-specific branches.
The exact topology target is enabled by default on every supported desktop
compiler. There is no floating-point fallback.

## Connectivity, faces and determinism

Sources are sorted by kind and stable ID. Pairwise exact intersections add cuts
parameterized on each original source. Endpoint joins, collinear endpoint-only
touches, T-junctions, crossings, concurrent intersections, and multiple cuts on
one source are supported. Equal rational points deduplicate exactly. Zero-length
sources and duplicate/zero IDs are rejected defensively.

Vertices sort lexicographically by exact (x,y). Edges sort by canonical vertex
indices. Each edge preserves source kind, source ID, a copy of its original ordered
integer source segment, and exact original-source parameters `source_t_start/end`.
Parameters can descend: canonicalizing an edge
does not canonicalize its source. Reversing the source maps t to 1-t. For a Wall,
U = t * its existing derived scalar length preserves the current transform
convention without rounding the stored span. There are no Room IDs or Room sides.

Private paired directions provide just enough connectivity for face walks, not
a mutable/general DCEL. Outgoing rays sort counter-clockwise by half-plane and
integer cross product. At a destination, the clockwise predecessor of the twin
keeps the face on the left. Each walk starts at its least directed edge index.
At the lexicographically least vertex of each component, all outgoing vectors
lie in (-pi/2, pi/2]; selecting the uppermost ray by cross product identifies
that component's exterior walk, whose left sector reaches x < minimum_x. This
works across the angular sort's 0/2pi cut and at leaves/high-degree junctions.
The invariant is documented next to the implementation and tested under all
eight square symmetries, source permutations and complete endpoint reversal.

`PlanTopology` owns vertices, provenance edges, directed boundary steps,
contiguous boundary ranges and face ranges. Face 0 is always the single
unbounded exterior, even for empty input. It can have one boundary walk for
each unnested connected component, including retraced bridge/open-segment walks.
Each bounded face has one simple counter-clockwise boundary (Cartesian X/Y,
interior on the left). Bounded faces and exterior boundary components follow
least-directed-edge walk order. Reflections still produce CCW bounded walks.
Changing source collection order preserves the full result; reversing endpoint
order preserves geometric connectivity and indices while complementing t.

## Explicit unsupported geometry

Positive-length collinear overlaps, including coincident duplicate segments,
return `UNSUPPORTED_OVERLAP`; endpoint-only touching remains supported. Nested
disconnected embeddings return `UNSUPPORTED_NESTING`, including an isolated
segment inside a bounded face. The exact containment test exists solely to
reject unsupported nesting; it is not a Room-placement query. Connected holes,
interior slits or other bounded walks with repeated vertices return
`UNSUPPORTED_NON_SIMPLE_FACE`. Exterior bridge retracing remains valid. There is
no claim to support arbitrary polygons with holes or coincident-edge resolution.
These are derived-query limitations only, independent of project validity.

## Complexity and deferred work

For n sources, K stored cuts (including duplicates), E subdivided edges, and W
walks, pairwise intersections and ID checks take O(n^2); K is O(n^2). Sorting and
lookup take O(K log K). Connectivity uses O(E) directions and sorted rays.
The simple current component/root and face/nesting scans have a conservative
O(E^2) bound (W and the vertex count are O(E)); total worst-case time is therefore
O(n^2 + K log K + E^2), conservatively O(n^4) in source count, not universally
O(n^2). Memory is O(n + K + E). This is a correctness-first on-demand builder;
no present workload evidence justifies an acceleration structure in 7D.

Deferred: RoomBoundary and adjacency,
Room-side assignments, persisted or incremental topology, area/perimeter APIs,
production rendering, Room annotations/automatic creation, hole/overlap solving,
and a general CAD geometry kernel. None is needed to own and discard this result.

## Room resolution (Priority 7E)

The builder still ignores Rooms. `plan_topology_find_face_at_plan_position`
queries a previously successful, unmodified topology using an integer
`PlanPosition`; it has no Room knowledge. It returns `PlanTopologyPointResult`
with a subsystem code, point state, and local face index:

- BOUNDED: strictly inside one bounded face, with that face index.
- UNBOUNDED: in the exterior, face index 0; empty/open arrangements are valid.
- ON_BOUNDARY: on any edge or vertex, including an isolated segment; no adjacent
  face is chosen (`face_index = SIZE_MAX`).
- Failure: non-success code and UNCLASSIFIED state, with no face. NULL or an
  unbuilt/destroyed topology is INVALID_ARGUMENT, not an empty geometry result.

The query assumes builder-owned topology, not arbitrary hand-edited arrays.
It is not a second topology validator. It does not allocate, mutate, regenerate,
or update any authoritative or derived state.

### Exact predicates

7D's temporary workspace retained integer source lines for exact containment
predicates, but discarded them on export. 7E retains each edge's original
ordered `source_segment` as owned derived provenance. No pointer borrows source
storage and no authoritative model changes. This small per-edge copy permits
exact point queries without computing wider rational determinants or creating
another numeric backend.

For point P and integer source endpoints A,B, cross(B-A,P-A) tests the exact
supporting-line side with magnitude < 2^65. A zero result plus inclusive exact
rational X/Y bounds on the subdivided edge establishes ON_BOUNDARY. The complete
edge scan runs before face containment, so vertices and internal divider edges
never arbitrarily select a face.

For each bounded face, use its existing boundary steps, with no re-sorting or
reconstruction. A half-open horizontal-ray test compares each rational endpoint
height to the integer query height. A straddling edge crosses to the right when
its directed supporting-line orientation has the appropriate sign. Original
source direction is adjusted by the source t ordering and the boundary step's
reversal flag. Shared vertices count once; horizontal edges do not cross.
All comparisons use the existing continued-fraction comparator. Coordinates
are never rounded and no floating-point epsilon or area calculation is used.
The query takes O(E + F) time and O(1) extra space for a supported built topology.

### Room adapter and failures

`room_region_resolve(project, room_id, topology)` finds the stable Room ID,
then queries its authoritative location. It returns a `RoomRegionResult`:
RoomRegionCode, Room ID, borrowed topology pointer, face index, and original
PlanTopologyResult failure details where applicable. Its states are UNPLACED,
BOUNDED, UNBOUNDED, ON_BOUNDARY, INVALID_ARGUMENT, ROOM_NOT_FOUND and
TOPOLOGY_FAILED. UNPLACED returns without inspecting topology or geometry;
NULL topology is allowed for that state. Room collection metadata is checked
before lookup. The adapter does not inspect walls or editor navigation.

`room_region_build_and_resolve(project, room_id, output)` is an explicitly named
convenience operation: output remains caller-owned, with the normal topology
initialization and replacement contract. For a placed Room it builds once, then
queries. Unsupported overlap/nesting/non-simple faces, numeric overflow or
allocation failure return TOPOLOGY_FAILED with the builder's original code and
source IDs. A failed build preserves old output but does not classify against
that stale output. Unplaced/missing Rooms do not trigger a build.
For many Rooms, build once and call `room_region_resolve` for each.

### Lifetime, boundaries and future sides

A RoomRegionResult owns nothing. For a bounded result, follow
`result.topology->faces[result.face_index]` through existing boundary and step
ranges. There is no duplicate RoomBoundary array. Boundary order and CCW winding
are exactly those supplied by 7D. Topology can outlive the source project.

A face index is local to the topology snapshot, not a DomainId. The borrowed
pointer and indices expire on topology destruction or replacement. Never store
these in Room or persistence. Caller-managed snapshots do not detect project
changes: after geometry edits, successfully rebuild before querying new geometry.
After placement changes, query again using the desired geometry snapshot. Failed
rebuilds leave an older snapshot alive, so always inspect the returned status.

Multiple Rooms may return the same face. No spatial uniqueness policy, enclosure
requirement or Room adjacency is introduced. Project validation, persistence,
commands/history, and production editor/rendering policy are unchanged.

Boundary-step direction together with original source t direction and source
kind/ID is sufficient for later recovery of the physical Wall span and which
side faces the bounded interior (face-on-left convention). This priority does
not assign Room sides, adjacency or wall membership.

## Wall junctions (Priority 8)

`wall_junctions_build(const PlanTopology *, WallJunctionSet *)` consumes an
already successfully built, unmodified topology snapshot. It does not rebuild
topology or validate the project. Build topology once and reuse it for Room
resolution and wall-junction queries:

```text
Storey (WallDefinition.segment and virtual RoomSeparators)
    -> PlanTopology (exact geometry and provenance)
        -> WallJunctionSet (distinct physical Walls meeting at a vertex)
```

`WallDefinition.segment` remains the sole authoritative wall geometry. Junctions
are derived relationships, with no DomainIds of their own, persistent records,
BuildStructure/Project fields, commands, or undo snapshots. Participant Wall IDs
refer to existing physical sources. Adding/deleting a wall, moving an endpoint,
undoing/redoing an edit, or loading a project changes the relationships obtained
by rebuilding. Intersections are valid project geometry; unsupported
positive-length collinear overlaps remain topology limitations, not project
validation failures. Persistence v8 stores Storey containment only; junctions remain derived.

### Owned snapshot and errors

`WallJunctionSet` owns two contiguous arrays: `junctions` and `participants`.
Each `WallJunction` holds its kind, an exact `PlanTopologyVertex` copy, and a
`first_participant`/`participant_count` range. Each participant contains
`wall_id`, `position`, and the exact original-source `source_t`. All arrays have
exact result counts; temporary incidence storage is discarded. Nothing borrows
project, Wall, topology, or builder workspace storage. A junction set can outlive
both its topology and project. Do not shallow-copy it into another owner.

Output must be zero-initialized or a previous valid result. Success replaces
the old arrays; failure preserves every old pointer, count and value.
`wall_junctions_destroy(WallJunctionSet *)` frees both arrays and zeros the
structure; NULL, zero state and repeated destruction are safe. Pointers into
the set and its participant ranges expire on its own destruction/replacement.

The operation returns a subsystem-local `WallJunctionCode`: `SUCCESS`,
`INVALID_ARGUMENT`, `ALLOCATION_FAILED`, or `NUMERIC_OVERFLOW` (array size
arithmetic), all prefixed `WALL_JUNCTION_`. No source-specific error payload is
needed because topology construction has already resolved the geometry.
NULL/unbuilt/destroyed topology is invalid; a successfully built empty topology
produces a successful empty set. Like the point query, this API assumes a valid
builder result and is not a validator for arbitrary hand-edited topology arrays.

A set describes exactly the topology snapshot from which it was built. It does
not detect staleness or refresh itself. After authoritative geometry changes,
successfully rebuild topology, then rebuild junctions to describe the new state.
Building junctions against old topology still describes old geometry. Check both
return statuses: either failed rebuild preserves its own previous result, and
the two snapshots may then describe different geometry. Old sets remain usable
as historical snapshots even after topology replacement or source deletion.

### Participants, exactness and determinism

Only `PLAN_TOPOLOGY_SOURCE_WALL` edges contribute. At least two distinct physical
Wall IDs must occur at a vertex. Wall/RoomSeparator and separator-only meetings
produce no wall junction. A separator sharing an existing wall-junction vertex
does not become a participant or change its classification.

Each wall-edge endpoint contributes a temporary incidence sorted by topology
vertex index, Wall ID, and edge index. Equal `(vertex, wall_id)` incidences are
compacted before counting participants. Thus an INTERIOR wall whose two
subdivided edges touch the same vertex appears exactly once. Junctions follow
topology's exact lexicographic `(x,y)` vertex order; participants follow ascending
stable Wall DomainId. Neither ordering depends on project storage order or
allocation addresses. For E topology edges, this layer takes O(E log E) time
and O(E) temporary memory, with O(J + P) owned output for J junctions and P
participants. It does not repeat pairwise intersection construction.

The participant parameter is copied from `source_t_start` when the incidence
touches `start_vertex`, and from `source_t_end` at `end_vertex`. Canonical edge
direction is independent of original Wall endpoint order; parameters may descend.
Exact t == 0 means `WALL_JUNCTION_PARTICIPANT_START`, t == 1 means
`WALL_JUNCTION_PARTICIPANT_END`, otherwise `WALL_JUNCTION_PARTICIPANT_INTERIOR`.
Reversing an original Wall exchanges its START/END roles and complements its t;
the junction position and kind remain unchanged.

Positions and parameters retain the full reduced rational limb representation,
including fractional intersections and denominators wider than 64 bits. No
rounding to PlanPosition, epsilon comparison, or floating-point test occurs.
The only additional geometric predicate distinguishes collinear endpoint
directions, using retained integer source segments and the existing private
`topology_cross` kernel with the same proven bounds. No new public numeric
primitive or second intersection implementation is introduced.

### Geometry classification and construction boundary

| Distinct physical Walls | Participant positions | Kind |
| --- | --- | --- |
| 2 | Both endpoints, non-collinear | `WALL_JUNCTION_CORNER` |
| 2 | Both endpoints, collinear | `WALL_JUNCTION_CONTINUOUS` |
| 2 | One endpoint, one interior | `WALL_JUNCTION_T` |
| 2 | Both interior | `WALL_JUNCTION_CROSS` |
| More than 2 | Any combination | `WALL_JUNCTION_MULTIWAY` |

CORNER includes arbitrary non-collinear angles, not just right angles.
CONTINUOUS requires only the already-supported endpoint-only collinear contact.
Three separately modelled walls that visually resemble a T remain MULTIWAY;
their actual identities and endpoint roles are retained.

For a T, the endpoint participant identifies the terminating/butting wall and
the INTERIOR participant identifies the wall continuing through. This already
preserves the information needed for later construction decisions; there is no
separate butt object or duplicated authoritative relationship.

Internal/external corners are deferred. Centreline geometry alone cannot
establish construction sides: those require later face, room/building side,
envelope, orientation, or other higher-level semantics.

Framing is intentionally unaffected. `wall_generate`, stud/plate/corner-stud
placement, blocking, openings and all other member generation remain independent
per-wall operations. Walls may still generate framing along their entire lengths
despite a derived junction. Later priorities will choose construction treatment.
No junction handles, trimming, snapping changes, rendering policy, incremental
cache, persistent connectivity, overlap/hole solving, or CAD kernel is added.

### Tests and dependency direction

`test_wall_junctions` covers empty/lone sources, separator exclusion, all pair
classifications under eight square symmetries, independent endpoint reversals
and source permutations, exact fractional and full-range/wide-limb copies,
deduplication, concurrent multiway meetings, multiple ordered junctions, and
all 24 permutations of a four-source fixture. Integration tests rebuild after
create/delete/move/history/load, verify project storage-order independence,
staleness and independent lifetimes, and preserve authoritative validity for
unsupported overlap. Linux GNU/Clang link-time allocation wrappers fail every
junction allocation with both zero and populated outputs, checking unchanged
input, transactional output and successful retries. No production allocator
hooks are required; the same tests can run with ASan/UBSan.

The implementation is part of the optional `sitehelper_topology` target and
depends on topology/model types and its existing private exact numeric helper.
Topology has no dependency on junction semantics, editor, commands, persistence
or rendering. Command/persistence dependencies belong only to the integration
test executable. The `SITEHELPER_BUILD_TOPOLOGY` compiler/backend policy is
unchanged.

## Storey scope (Priority 9A)

`plan_topology_build_from_storey(const Storey *, PlanTopology *)` replaces the
former project-wide adapter. It copies only the supplied Storey's Walls and
RoomSeparators. Other Storeys may contain identical or intersecting X/Y sources
without contributing overlaps, intersections, faces or junctions to this result.
Elevation does not enter the 2D numeric engine. A null Storey fails; an empty
Storey builds successfully. No Storey ID is added to derived geometry.

The generic builder remains independent of project ownership; its header uses
model geometry types and forward-declares Storey for the adapter. The exact
intersection engine and WallJunctionSet implementation are unchanged.

RoomRegion supplied-topology queries require a snapshot from the Room's owning
Storey. The caller must enforce that contract; no cache/provenance scheme is
introduced to prove it. `room_region_build_and_resolve` finds the Room by global
ID, resolves its owning Storey and builds only that Storey's topology. Build once
per Storey to resolve multiple Rooms or build junctions from the same snapshot.
Snapshots retain the existing transactional ownership and staleness contracts.

See [Storey ownership and settings direction](../project/README.md).
