# Slab plan adapter

This derived adapter makes authoritative slabs visible and selectable in the plan workspace. It is downstream of `sitehelper_slab`; the slab/model libraries contain no renderer, editor, SDL, camera, colour, or screen-coordinate state.

Rendering uses closed outlines for the slab exterior, replacement regions, and penetrations. Penetrations use a distinct colour and boundary markers. Edge rebates render as their authoritative host-edge interval with endpoint markers. The adapter does not invent polygon fills, persisted steps, or an exact inward rebate footprint. Application composition draws slabs after the grid and before walls and tool overlays, and only visits the active Storey.

Hit testing uses plan-world millimetres and the editor's established object tolerance. Precedence is edge rebate, penetration, region, then whole slab. At equal precedence, the later item in Storey/render order wins, giving deterministic selection when slabs overlap. Invalid slab inputs are skipped rather than repaired.

The adapter also supplies the Rebate tool's nearest-exterior-edge query. It
returns the slab, outer-edge index, projected plan point, distance, and rounded
integer-mm edge-local U. This is interactive query geometry rather than global
snapping or authoritative slab state. Once a tool locks a host, a specific-edge
query prevents cursor motion from jumping around a corner.

The application keeps its established foreground-object policy: a Wall within
the same object tolerance is selected before consulting slab geometry. This
matches the render stack, where walls and editor overlays are drawn over slabs.

Whole slabs are selected by stable `DomainId`. Penetrations, regions, and rebates retain their deliberate subordinate identity policy: editor state stores the parent slab ID, feature kind, and slab-local collection index. These indices are ephemeral, are never persisted, and are cleared when reconciliation finds a missing parent, a changed Storey, or an out-of-range index.
