# Cut list

`sitehelper_cutlist` represents **required pieces**, derived from committed
generated construction. For each Wall it builds a wall-level `FramingTakeoff`
and copies each item into a `RequiredMember` with that Wall's `source_wall_id`.
It reuses Priority 21's member traversal, framing checks, aggregation and units;
it never reads openings or settings or recalculates construction.

`RequiredMember` contains the source Wall ID, timber type, stud subtype, integer
millimetre length/depth/width and `uint64_t` quantity. Non-stud subtype is
canonically `STUD_COMMON`, meaning not applicable. Identical requirements within
a Wall aggregate by Priority 21's key. Identical requirements from different
Walls remain separate. No generated member receives an identity. Source Wall
IDs must be nonzero; their global uniqueness remains a model contract.

The APIs in `cut_list.h` are `cut_list_build_wall`, `cut_list_build_storey`,
`cut_list_build_project`, and `cut_list_destroy`. Empty Storeys/Projects succeed.
An identified Wall without usable framing fails rather than disappearing.
Collection metadata, missing source identity, framing, allocation and arithmetic
errors have cut-list-specific status codes, with an offending Wall ID when known.
The framing checks and their limits are inherited from Priority 21: they do not
prove that structurally coherent framing is current relative to definitions.

Results follow stored Project Storey order, then stored Wall order, then
Priority 21's deterministic item order. Reordering sources reorders their blocks.
**This is not a cutting sequence.** Future optimisation will decide execution
order; this layer performs no sorting across Walls or cut planning.

Initialize `CutList list = {0}` and check the returned `CutListResult.code`.
Success replaces the previous snapshot; failure preserves all previous pointers
and contents. The list owns its `members[0 .. member_count)` values and survives
source regeneration/destruction. Do not shallow-copy it into another owner.
`cut_list_destroy` frees storage and zeros the list; NULL, zero and repeated calls
are safe. Allocation/count arithmetic is checked. Input must remain unmodified
during a query and its collections must own valid, disjoint allocations.

The dependency is `sitehelper_cutlist -> sitehelper_takeoff -> project/model/wall`.
No reverse dependency or topology dependency is introduced. Snapshots are derived
data, have no identity of their own, are not serialized, and do not participate
in commands/history or modify the project, framing or `Timber`.

A required piece is also not evidence of structural adequacy. The cut list must
not consume a calculator-proposed beam/lintel size until that choice has been
explicitly committed into project design authority. Calculation success itself
never implies engineering approval; see `../structural/README.md`.

Available stock is an intentionally separate future model. Stock lengths,
inventory, purchasing, packing/optimisation, kerf, waste, offcuts/remnants,
pricing, saw-bench instructions, labels/barcodes and GUI are outside this layer.
Material/product specifications (species, grade, treatment, SKU) remain deferred;
the available section dimensions and member purposes are sufficient here.
