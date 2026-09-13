# Slab domain and internal penetrations

`Slab` is authoritative physical construction owned exclusively by a Storey's
`SlabCollection`, alongside its `BuildStructure`. It has a stable `DomainId` in
the Project's global namespace. It is independent of Walls, Rooms and topology.
The library depends only on `sitehelper_model` and the C runtime/math library;
`sitehelper_project` depends on `sitehelper_slab`, never the reverse.

`SlabDefinition` stores an ordered, implicitly closed polygon of integer-mm
`PlanPosition` vertices, positive `thickness_mm`, and signed
`top_level_offset_mm` relative to the owning Storey's reference elevation. Do not
repeat the first vertex at the end. `slab_absolute_top_elevation_mm()` widens the
Storey elevation plus this offset to `int64_t`; absolute elevation is not stored.

`SlabDefinition.penetrations` owns an ordered `SlabPenetrationCollection`. Each
penetration owns its own `SlabOutline` with the same integer-mm plan coordinates
and implicit closure as the exterior. Penetrations have **no DomainIds**; they are
subordinate geometry, not Storey/project entities. Destruction recursively frees
all outlines. No wall, room, topology, editor or renderer dependency is involved.

An internal penetration must be strictly inside the outer polygon. Every outer /
penetration edge pair is checked with the same exact inclusive intersection
predicate used for polygon simplicity. A half-open ray parity test then
classifies a point as inside, outside or boundary, using checked cross-product
signs without division or floating point. Checking every edge pair prevents an
edge crossing outside through a concave indentation even if all its vertices
are inside. Both outlines may be concave and either winding is accepted.

Penetration boundaries cannot touch, cross or share edges with the exterior or
with each other. Pairwise containment checks in both directions also reject
nested/enclosing holes. Duplicate, overlapping and point-touching holes fail.
There are no islands, edge notches, clipping, or polygon Boolean operations.

`slab_validate()` / `slab_definition_validate()` accept convex and concave simple
polygons in either winding. They reject fewer than three vertices, all repeated
vertices, zero area, adjacent backtracking, and nonadjacent edge intersections or
touches. Collinear forward boundary vertices are valid. Validation checks
collection metadata before traversal. As with other C owned collections, a
non-null pointer must designate the allocation described by its metadata.

Predicates and signed area accumulation use checked `int64_t` arithmetic, with
translation to the first vertex to avoid unnecessary absolute-origin products.
Unrepresentable intermediates return `SLAB_NUMERIC_OVERFLOW`, including for an
otherwise simple polygon; geometry is never approximated to decide validity.
Validation takes O(V²) time for V total outer/penetration vertices, O(1)
workspace and allocates no memory. Zero-area outlines are rejected before
intersection classification, so an exterior symmetric bow-tie returns
`SLAB_INVALID_OUTLINE` rather than `SLAB_SELF_INTERSECTION`. Invalid penetration
polygons use `SLAB_INVALID_PENETRATION_OUTLINE` for both cases.

`slab_measure()` retains its **gross-only** contract and derives `SlabQuantities`:
exact unsigned `area2_mm2` and
`volume2_mm3`, plus floating-point `perimeter_mm`. Actual area and volume are
these doubled values divided by two, preserving half-square/cubic-millimetre
cases. Volume is the base polygon area times thickness. Its multiplication is
checked separately: valid geometry may have an unrepresentable doubled volume.
`slab_edge_length_mm()` returns a selected edge's Euclidean length, including the
implicit closing edge. `perimeter_mm` remains the outer perimeter only; internal
edges are not added to it. All quantity outputs remain unchanged on failure.

`slab_measure_material()` returns `SlabMaterialQuantities` with explicit
`gross_area2_mm2`, `void_area2_mm2`, `net_area2_mm2`, `gross_volume2_mm3`,
`void_volume2_mm3` and `net_volume2_mm3`. Gross is exterior area, void is the sum
of disjoint penetration areas, and net is gross minus void. Each volume is its
area times the uniform thickness. All six fields are `uint64_t` doubled exact
units; additions, subtraction and products are checked. These values validate
the complete definition and are always recomputed, never authoritative state.

`slab_build()` validates and deep-copies a caller's vertices into owned storage.
Its output must be zero-initialized or an existing valid owned Slab. Success
replaces it; failure leaves every old value and allocation unchanged. The source
vertices may belong to that old output. Collection append moves an independently
owned Slab and zeros it only on success; removal destroys its outline and
preserves remaining order. Destroy functions accept NULL/zero/repeated calls;
otherwise their input must have valid owned storage and coherent metadata.
Never shallow-copy an owned Slab into a second owner.

`slab_build()` constructs a replacement with zero penetrations. `slab_clone()`
transactionally deep-copies the entire Slab, including all penetrations, and
supports self-copy. `slab_add_penetration()` validates the existing slab, candidate
polygon and all relationships before allocating independent vertices and growing
the collection. Any failure preserves all old pointers and values. Removal by
index validates first, frees the selected outline and preserves remaining order.
`slab_penetration_at()` gives read-only borrowed access; count is available through
`definition.penetrations.count`. Indices are transient positions, not identities.
`slab_penetration_validate()` checks a standalone outline; full slab validation
additionally checks its relationships. None of these operations consumes IDs.

Slab errors distinguish invalid penetration collection/outline metadata, invalid
standalone outline, outside-or-touching exterior, overlapping-or-touching/nested
penetrations, allocation failure and numeric overflow. Project validation reports
the owning slab ID and Storey ID, preserving old diagnostic ordering when there
are no penetrations.

Project add APIs require a Storey ID, stage identity allocation, and delegate
geometry and ownership to this library. Find/remove APIs use the same global
namespace as other entities. `sitehelper_project_insert_slab()` deep-copies an
existing identity for restoration; its caller establishes the final allocator
watermark. The project validator checks all slab collection metadata before
identity traversal and delegates polygon validity to this subsystem.

Persistence **v12** extends v11, which added a `slabs COUNT` section after each
Storey's room separators and before `end_storey`:

```text
slabs 1
slab 42 top_level_offset -50 thickness 100 outline 3
vertex 0 0
vertex 3 0
vertex 0 1
penetrations 0
end_slab
```

A nonempty section uses `penetrations COUNT`, followed by `penetration outline N`,
N ordered `vertex X Y` records and `end_penetration` for each polygon. It occurs
after the exterior vertices and before `end_slab`. No penetration IDs are stored.

Only identity and definition are serialized. Versions 1–10 load with no slabs;
v11 slabs load with zero penetrations. All existing migration rules remain in
place. Loading validates an independent
candidate, including global identity uniqueness and allocator watermark, before
replacing the destination. Derived area, edges, perimeter and volume are never
persisted.

Priority 25C+, steps, set-downs, rebates, edge notches, footings,
reinforcement, wall/room relationships or generation, rendering, selection,
editing tools, commands/history and take-off integration remain deferred.
