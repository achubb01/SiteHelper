# Slab domain foundation

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
Validation takes O(N²) time, O(1) workspace and allocates no memory. Zero-area
outlines are rejected before intersection classification, so a symmetric bow-tie
returns `SLAB_INVALID_OUTLINE` rather than `SLAB_SELF_INTERSECTION`.

`slab_measure()` derives `SlabQuantities`: exact unsigned `area2_mm2` and
`volume2_mm3`, plus floating-point `perimeter_mm`. Actual area and volume are
these doubled values divided by two, preserving half-square/cubic-millimetre
cases. Volume is the base polygon area times thickness. Its multiplication is
checked separately: valid geometry may have an unrepresentable doubled volume.
`slab_edge_length_mm()` returns a selected edge's Euclidean length, including the
implicit closing edge. All quantity outputs remain unchanged on failure.

`slab_build()` validates and deep-copies a caller's vertices into owned storage.
Its output must be zero-initialized or an existing valid owned Slab. Success
replaces it; failure leaves every old value and allocation unchanged. The source
vertices may belong to that old output. Collection append moves an independently
owned Slab and zeros it only on success; removal destroys its outline and
preserves remaining order. Destroy functions accept NULL/zero/repeated calls;
otherwise their input must have valid owned storage and coherent metadata.
Never shallow-copy an owned Slab into a second owner.

Project add APIs require a Storey ID, stage identity allocation, and delegate
geometry and ownership to this library. Find/remove APIs use the same global
namespace as other entities. `sitehelper_project_insert_slab()` deep-copies an
existing identity for restoration; its caller establishes the final allocator
watermark. The project validator checks all slab collection metadata before
identity traversal and delegates polygon validity to this subsystem.

Persistence **v11** adds a `slabs COUNT` section after each Storey's room
separators and before `end_storey`:

```text
slabs 1
slab 42 top_level_offset -50 thickness 100 outline 3
vertex 0 0
vertex 3 0
vertex 0 1
end_slab
```

Only identity and definition are serialized. Versions 1–10 load with no slabs;
all existing migration rules remain in place. Loading validates an independent
candidate, including global identity uniqueness and allocator watermark, before
replacing the destination. Derived area, edges, perimeter and volume are never
persisted.

Penetrations/voids (Priority 25B), steps, set-downs, rebates, footings,
reinforcement, wall/room relationships or generation, rendering, selection,
editing tools, commands/history and take-off integration remain deferred.
