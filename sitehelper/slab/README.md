# Slab domain, penetrations, replacement regions and edge rebates

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
Without regions, validation takes O(V²) time for V total vertices, O(1)
workspace and allocates no memory. With regions, exact boundary contact/interval
classification has O(V³) worst-case time and still O(1) workspace, with no
allocations. This deliberately favours simple exact predicates for small
residential polygons over clipping or a spatial index. Zero-area outlines are
rejected before intersection classification, so an exterior symmetric bow-tie returns
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
area times the **base thickness only**, even when regions exist. All six fields
are `uint64_t` doubled exact units; additions, subtraction and products are checked. These values validate
the complete definition and are always recomputed, never authoritative state.

`slab_build()` validates and deep-copies a caller's vertices into owned storage.
Its output must be zero-initialized or an existing valid owned Slab. Success
replaces it; failure leaves every old value and allocation unchanged. The source
vertices may belong to that old output. Collection append moves an independently
owned Slab and zeros it only on success; removal destroys its outline and
preserves remaining order. Destroy functions accept NULL/zero/repeated calls;
otherwise their input must have valid owned storage and coherent metadata.
Never shallow-copy an owned Slab into a second owner.

`slab_build()` constructs a replacement with zero penetrations and regions. `slab_clone()`
transactionally deep-copies the entire Slab, including all penetrations and regions, and
supports self-copy. `slab_add_penetration()` validates the existing slab, candidate
polygon and all relationships before allocating independent vertices and growing
the collection. Any failure preserves all old pointers and values. Removal by
index validates first, frees the selected outline and preserves remaining order.
`slab_penetration_at()` gives read-only borrowed access; count is available through
`definition.penetrations.count`. Indices are transient positions, not identities.
`slab_penetration_validate()` checks a standalone outline; full slab validation
additionally checks its relationships. None of these operations consumes IDs.
Validated indexed insertion is also available for history restoration. It owns a
fresh outline copy, preserves collection order and leaves the slab unchanged on
invalid geometry, an invalid index or allocation failure.

`SlabDefinition.regions` owns an ordered `SlabRegionCollection`. Each region owns
its outline and stores positive `thickness_mm` and a signed
`top_level_offset_mm` relative to the **Storey reference plane**, not the base top.
Regions have no global IDs. A slab region replaces the slab's base top level and
thickness over its material area. **Regions are not additional concrete layers.**
Outside all regions, base construction applies. Bottom offset is derived as top
minus thickness, widened to `int64_t`; adding Storey elevation gives absolute
level. For a Storey at 3000, base top 0/thickness 100 gives top 3000/bottom 2900;
region top -50/thickness 50 gives top 2950/bottom 2900. Region thickness 150 instead
gives bottom 2800. Changing the base offset does not move region levels.

Regions may share the exterior boundary, including corners or an entire side.
Every edge interval must remain inside or on the exterior: testing vertices alone
would miss edges crossing a concavity. Region interiors must be disjoint;
separation, shared vertices, full/partial edges and subdivided collinear edges are
valid. Nesting, duplicate polygons and any interior overlap are invalid. Private
exact predicates classify open boundary intervals at doubled-coordinate
midpoints and compare winding at coincident edges. No floating-point validity
or general polygon Boolean engine is used.

A slab region must describe at least some existing slab material. A region wholly
contained by, or coincident with, a penetration is invalid. A penetration may be
contained inside a region; the penetration remains void and is subtracted from
the region's material area. Separated regions and penetrations remain valid;
partial interior overlap and boundary crossing are rejected. Boundary contact
alone is allowed when interiors are disjoint or the region contains the
penetration and retains material. Invalid relationships use
`SLAB_REGION_PENETRATION_INTERSECTION`. These rules apply in either insertion
order and during independent validation/loading, including v13 files.

`slab_add_region()` copies validated geometry only after checking all slab,
penetration and region relationships; allocation or validation failures preserve
the entire slab. `slab_remove_region()` frees by index and preserves order.
`slab_region_at()` returns a borrowed const pointer, invalidatable by mutation;
count is `definition.regions.count`. `slab_region_validate()` checks the standalone
polygon and thickness, while full slab validation checks relationships too.
Cloning and destruction include every owned region outline.
Region and edge-rebate collections provide the same validated indexed insertion
contract. These operations exist to restore authoritative source order; their
indices remain transient collection positions rather than feature identities.

`slab_measure_construction()` explicitly applies replacement thickness:

```text
net_area2 = gross_area2 - void_area2                  (unchanged by regions)
region_material_area2 = region_polygon_area2 - void_area2_in_region
base_material_area2 = net_area2 - sum(region_material_area2)
total_volume2 = base_material_area2 * base_thickness
              + sum(region_material_area2 * region_thickness)
```

`SlabConstructionQuantities` exposes net, base and combined region material areas
and total construction volume. `slab_region_measure()` returns an individual
region's polygon, void and material areas, its thickness, and material volume.
Every accepted region has strictly positive material area.
All area/volume fields use exact `uint64_t` doubled units (`area2_mm2` /
`volume2_mm3`); divide by two only at the presentation boundary. Additions,
subtractions and products are checked. Old `slab_measure()` and
`slab_measure_material()` volumes continue to use base thickness only; use the
new construction query for replacement-region volume before rebates. Geometric validation remains
independent of volume representability, as in 25A/25B; quantity queries return
numeric overflow without changing their outputs.

`slab_properties_at_plan_position()` classifies integer plan points as outside,
outer boundary, penetration interior, penetration boundary, base interior,
region interior, or region boundary. Only base/region interiors return physical
properties: Storey-relative top offset, thickness and widened bottom offset.
Only region interior returns an index; every other classification has `SIZE_MAX`.
Exterior boundary takes precedence, then penetration interior/boundary, then
region boundary/interior, then base. A penetration boundary touching a region
boundary still reports penetration boundary. All boundary classifications carry zero properties, even if
neighbouring properties happen to agree; no arbitrary side of a step is chosen.
Classifications are successful queries; errors preserve the prior output.

Step edges are derived from neighbouring material regions with differing top
levels and are not persisted. Step-edge extraction remains a future query.

`SlabDefinition.edge_rebates` owns an ordered collection of perimeter-attached
subtractive profiles. A `SlabEdgeRebate` has no DomainId and refers only to one
outer-outline edge: edge `i` runs from vertex `i` to vertex `(i+1) mod N`.
Its integer-mm local coordinate has U=0 at the first vertex and increases toward
the second. The local edge length uses the same `round(hypot(dx,dy))` convention
as `WallPlanSegment`, so axis-aligned edges are exact and U=edge length denotes
the second endpoint even for a diagonal whose Euclidean length is fractional.

`start_offset_mm` and `end_offset_mm` define a nonempty interval within that
edge. `width_mm` is positive horizontal width inward from the exterior; inward
is derived from polygon interior and does not store a winding-dependent side.
`depth_mm` is positive depth downward from the locally applicable slab top. A
region reaching the edge therefore establishes its Storey-relative top and the
rebate cuts down from that top. The rebate never copies base or region level, and
changing a region does not mutate it. Derived rebate bottom is local top minus
depth and is not persisted.

Priority 25D does not split a rebate interval where arbitrary region boundaries
reach or cross its inward footprint, nor does it validate a wide footprint
against an interior penetration. Those operations need exact offset/Boolean
geometry. The authoritative local-top-relative meaning is retained for a future
resolver; penetrations remain independent interior voids and are never treated
as perimeter openings.

Same-edge interval interiors must not overlap, irrespective of width/depth;
separation and exact adjacency are valid. Stepped width or depth is represented
by adjacent rebate objects. A rebate turning a corner uses one object per edge;
objects on different edges may meet at a convex or re-entrant vertex. Exact 3D
corner unions and inward offset footprints at concave corners are future derived
geometry. Rebate validation checks collection metadata, edge index, rounded edge
length, interval, positive dimensions and same-edge overlap. It does not remap
stale references after raw outline changes.

The edge-rebate representation is informed by the drainage and water-ingress
role described for masonry cavity/veneer slab edges in AS 2870—2011 clause 5.3.4,
including stepped depths and the structural significance of deeper rebates.
SiteHelper 25D stores geometry only. It does not enforce code-specific minimum
depth, remaining concrete, edge-beam width, or deep-rebate engineering rules.
Values such as 15 mm remain model-valid for recording designed, drawn or measured
geometry. Compliance assessment needs construction and structural context and is
deferred. Panel/service recesses addressed separately by the Standard are not
edge rebates and are also deferred.

All existing slab quantity queries validate rebates but intentionally make no
rebate-volume deduction. `slab_measure_construction()` remains the exact
base/region/penetration volume before rebates. A naive length × width × depth
subtraction can double-count corners or ignore region, penetration and concave
footprint interactions. Exact rebate-aware whole-slab volume is deferred until
appropriate solid/Boolean semantics exist; no approximate deduction is reported
as actual concrete. `slab_edge_rebate_length_mm()` exposes only the unambiguous
longitudinal interval length.

Slab errors distinguish invalid penetration collection/outline metadata, invalid
standalone outline, outside-or-touching exterior, overlapping-or-touching/nested
penetrations, allocation failure and numeric overflow. Project validation reports
the owning slab ID and Storey ID, preserving old diagnostic ordering when there
are no penetrations or regions. Region errors additionally distinguish collection /
outline metadata, polygon, thickness, exterior containment, region overlap and
penetration intersection failures; project validation maps these without geometry
algorithms in the project layer.

Project add APIs require a Storey ID, stage identity allocation, and delegate
geometry and ownership to this library. Find/remove APIs use the same global
namespace as other entities. `sitehelper_project_insert_slab()` deep-copies an
existing identity for restoration; its caller establishes the final allocator
watermark. The project validator checks all slab collection metadata before
identity traversal and delegates polygon validity to this subsystem.

Persistence **v14** extends v13 (regions), v12 (penetrations) and v11, which added a
`slabs COUNT` section after each Storey's room separators and before `end_storey`:

```text
slabs 1
slab 42 top_level_offset -50 thickness 100 outline 3
vertex 0 0
vertex 3 0
vertex 0 1
penetrations 0
regions 0
edge_rebates 0
end_slab
```

A nonempty section uses `penetrations COUNT`, followed by `penetration outline N`,
N ordered `vertex X Y` records and `end_penetration` for each polygon. It occurs
after the exterior vertices. A `regions COUNT` section follows penetrations;
each region is written as:

```text
region top_level_offset -50 thickness 50 outline 3
vertex 0 0
vertex 1 0
vertex 0 1
end_region
```

The edge-rebate section follows the final region. No penetration, region or
edge-rebate IDs are stored.

In v14, `edge_rebates COUNT` follows regions. Each record contains only its
authoritative fields and terminator:

```text
edge_rebate edge 0 start_offset 0 end_offset 2500 width 110 depth 20
end_edge_rebate
```

No endpoint coordinates, local top/bottom, interval length, volume or compliance
result is serialized. Versions 1–13 load with zero edge rebates.

Only identity and definition are serialized. Versions 1–10 load with no slabs;
v11 slabs load with zero penetrations, regions and edge rebates; v12 slabs load
with zero regions and edge rebates; v13 slabs load with zero edge rebates. All
existing migration rules remain in place. Loading validates an independent
candidate, including global identity uniqueness and allocator watermark, before
replacing the destination. Derived area, edges, perimeter and volume are never
persisted.

Plan rendering and selection are downstream consumers in `sitehelper/slab_plan`;
they add no dependencies or state to this domain library. Priority 25E2+,
explicit step objects/extraction, panel/service recesses, footings, reinforcement,
wall/room relationships or generation, editing tools, commands/history and
take-off integration remain deferred.
