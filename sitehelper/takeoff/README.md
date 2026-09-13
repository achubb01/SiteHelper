# Framing take-off

`sitehelper_takeoff` provides an owned, read-only derived report from committed
`WallFraming`. It is not authoritative project state, has no DomainId, and is
neither stored in the model nor serialized. Its dependency points toward
`sitehelper_project`/model; neither generation nor topology depends on it, and it
builds with `SITEHELPER_BUILD_TOPOLOGY=OFF`.

The public API is in `framing_takeoff.h`:

```c
FramingTakeoff report = {0};
FramingTakeoffResult status = framing_takeoff_build_project(project, &report);
if (status.code == FRAMING_TAKEOFF_SUCCESS) {
    /* Read report.items[0 .. report.item_count). */
}
framing_takeoff_destroy(&report);
```

`framing_takeoff_build_wall` and `framing_takeoff_build_storey` use the same
member traversal as the project query. Storeys contribute their physical Walls;
Rooms and virtual separators are irrelevant. Empty Storeys/Projects succeed.
The query never resolves settings, generates construction, reads openings, or
calls authoritative project validation.

Each Wall contributes its bottom plate, top plate, every `studs[]` entry, every
`nogs[]` entry, and every `members[]` entry exactly once. Headers and sills are
already physical entries in `members[]`; no opening rules are repeated here.
Both plates use `TIMBER_PLATE` and can aggregate.

Requirements aggregate by `(TimberType, StudType for studs, length_mm, depth_mm,
width_mm)`. Output follows that ascending lexicographic order, with enum values
ordering categories/subtypes. Non-stud `stud_type` is canonically `STUD_COMMON`,
meaning not applicable; non-stud union payloads are not inspected. Positions and
noggin bay numbers do not distinguish material requirements. Dimensions remain
integer millimetres, quantity uses `uint64_t`, and total linear length uses
`int64_t` millimetres. Allocation sizes, counts and accumulated lengths use
checked arithmetic. Metres belong at a later presentation boundary.

The implementation copies members to a temporary growable array, sorts it, and
coalesces equal keys in place. This uses O(N) storage and O(N log N) sorting for N
physical members. Only the first `item_count` entries belong to the public report;
spare allocation storage is private and must not be read.

Output must start zero-initialized or contain a previous successful result.
Success frees/replaces the previous storage. Failure frees only the candidate
and leaves the previous output, including its pointers and contents, unchanged.
Always check status: a retained older report still describes older construction.
The result owns its storage and can outlive the source project or regeneration.
Do not shallow-copy it into another owner. Destroy frees storage and zeros the
result; NULL, zero-state and repeated destruction are safe.

An ungenerated Wall fails the entire query with
`FRAMING_TAKEOFF_INVALID_FRAMING`, including its Wall ID. The narrow consumption
checks require two positive-dimension plates of plate type, at least two studs,
coherent count/capacity/pointer metadata for all three arrays, positive member
dimensions, known timber/stud types, and the expected types in studs/noggins
arrays. Zero noggins or additional members is valid. Invalid enclosing collection
metadata returns `FRAMING_TAKEOFF_INVALID_SOURCE`. Allocation and arithmetic
failures have separate codes. Diagnostics carry a Wall ID when known, otherwise
zero; Walls are visited in stored order. No Wall is silently skipped.

These checks are not a general framing validator: callers must provide valid,
disjoint owned allocations and prevent concurrent mutation. They cannot prove
pointer validity, reconstruct missing members, or detect stale but structurally
coherent framing after a low-level definition edit. Regeneration remains the
caller's responsibility; the report describes the committed construction.

Stock optimisation, stock lengths, packing, kerf, waste, offcuts, purchasing and
pricing are outside this subsystem. Material/product specifications (including
species, grade, treatment and SKUs) are deferred. `Timber` remains a generated
physical member; this query adds no material catalogue or authoritative fields.

One current generator limitation matters to interpretation: door framing adds
king/trimmer studs but does not currently generate a door header or upper
cripples. Window framing generates headers, sills and applicable cripples. A
future generator priority should address door completeness; the take-off must
not invent members absent from committed framing.

Focused tests cover empty scopes, exact generated-member accounting, openings,
aggregation and every key dimension, multiple Walls/Storeys, stable ordering,
read-only inputs, wide totals, malformed/missing framing, snapshot lifetime and
transactional replacement. On Linux with GNU/Clang they also sweep allocation
failures using linker wrapping, with no production allocation hooks.
