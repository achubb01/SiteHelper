# Derived plan topology and Room resolution (Priorities 7D–7E)

`sitehelper_topology` is an on-demand, read-only query over ordered physical
Wall and virtual RoomSeparator segments. It produces an owned temporary result;
it is not project state, persistence, history, an editor cache, or Room topology.
The project adapter reads only source collections, IDs and segments. It ignores
Rooms, Room placement, openings, framing, settings and the allocator watermark.

## API and ownership

`plan_topology_build(sources, count, output)` builds from `PlanTopologySource[]`.
`plan_topology_build_from_project(project, output)` copies authoritative source
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

## Compiler constraint

Only the private numeric header/implementation use `__int128` and GCC-compatible
checked arithmetic builtins. CMake probes both addition and multiplication.
The default `SITEHELPER_BUILD_TOPOLOGY=ON` requires those capabilities; there is
no floating-point fallback. GCC/Clang support is target-dependent, so the probe
is authoritative for a particular compiler/target. Linux x86-64 GCC is verified
in this workspace. No Windows compiler/toolchain or declared Windows compiler
policy is present here. Existing MSVC-specific CMake branches indicate that
preventing all non-topology application builds would be an unnecessary restriction.

Native MSVC does not provide this numeric backend. Configure with
`-DSITEHELPER_BUILD_TOPOLOGY=OFF` to build the existing application and other tests
without this independent query target. A Windows GCC/Clang target must pass the
same capability check; Windows builds are not claimed as tested. A future MSVC
backend can remain private without changing the public limb representation.
This is a temporary, explicit subsystem toolchain constraint, not a weaker
numeric policy or an assertion that the application now requires GCC.

Compiler references: [GCC integer extension](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html),
[Clang checked arithmetic](https://clang.llvm.org/docs/LanguageExtensions.html#checked-arithmetic-builtins),
[Microsoft sized integer types](https://learn.microsoft.com/en-us/cpp/cpp/int8-int16-int32-int64).

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
requirement or Room adjacency is introduced. Project validation, persistence v7,
commands/history, and production editor/rendering policy are unchanged.

Boundary-step direction together with original source t direction and source
kind/ID is sufficient for later recovery of the physical Wall span and which
side faces the bounded interior (face-on-left convention). This priority does
not assign Room sides, adjacency or wall membership.
