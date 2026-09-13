# Wall surface

`WallSurface` is an owned, derived, non-authoritative snapshot of a rectangular
Wall surface and its opening apertures. It has no DomainId of its own and is not
persisted. Source Wall/Opening IDs are copied provenance values, never pointers.
No model fields, commands/history, or persistence format are added.

`wall_surface_build(wall, settings, surface_height_mm, output)` reads the Wall's
identity and `WallDefinition`/`Openings`, **never `WallFraming`**. Studs, noggins,
additional members and both plates are irrelevant, including when absent or
malformed. A valid definition can produce a surface before any framing exists.
The snapshot survives source mutation, regeneration and destruction.

Geometry is wall-local elevation geometry in integer millimetres:

```text
U = 0 .. width_mm       (along the ordered Wall segment)
Z = 0 .. height_mm      (above the Wall baseline)
```

Width comes from `wall_length_mm`, preserving the established checked, nearest
whole-millimetre Euclidean segment length. Plan orientation and translation do
not become surface coordinates. There are no screen or global-space transforms.

Height is the explicit positive `surface_height_mm` query argument. It is not
stored in `WallDefinition`. As documented in `project/SETTINGS.md`, `stud_height`
is not ceiling height, floor-to-floor height or overall assembled wall height.
The query neither reads nor validates it. A later authoritative height provider
can supply the same surface-query boundary without redefining this abstraction.

The surface is **side-neutral**: there is no inside/outside, left/right face or
exterior/interior classification. Both faces currently share the same U/Z
boundary and apertures; future attachment/construction work can define sides.

Each `WallSurfaceOpening` retains its source ID, type, left U, bottom Z, width
and height. Stored Opening order is retained, providing deterministic source
order. `wall_opening_frame_geometry` supplies the canonical effective clear
rectangle, including default/custom allowances and their precedence. Only the
opening allowance fields of resolved settings are consumed. No allowance
formula is duplicated; no studs, headers, sills, king/trimmer extents, or cladding
clearances are used to reconstruct or expand an aperture.

The query checks source identity/collection metadata, a positive representable
Wall length, positive explicit height, identified supported opening types,
canonical opening geometry and aperture containment. Wide right/top extents are
compared with surface bounds. Apertures may touch the boundary; those extending
outside it fail explicitly and are never clipped. This is a narrow aperture
query, not whole-project validation or another implementation of assembly
spacing/overlap rules. Identity uniqueness remains a model contract.

`WallSurfaceResult` distinguishes invalid arguments, invalid source Wall,
invalid surface extent, invalid opening geometry, out-of-bounds openings,
allocation failure and numeric overflow. Diagnostics include the offending Wall
and Opening IDs when available (zero otherwise). Existing geometry helpers
collapse nonpositive/unrepresentable geometry to failure: unrepresentable Wall
lengths return `WALL_SURFACE_INVALID_SOURCE_WALL`, and unrepresentable effective
opening dimensions return `WALL_SURFACE_INVALID_OPENING`. Array-size arithmetic
returns `WALL_SURFACE_NUMERIC_OVERFLOW`. No overflowing int extent is formed.

Initialize `WallSurface surface = {0}` and check the build status. Success frees
and replaces previous storage; any failure preserves all previous pointers and
contents. A retained result still describes its earlier inputs. The result owns
one flat opening array; do not shallow-copy it into another owner.
`wall_surface_destroy` frees that array and zeros the result, and supports NULL,
zero state and repeated calls. Inputs must remain valid/unmodified during the
query; metadata cannot prove arbitrary pointer validity. Time and space are O(N)
for N openings, with no allocation for a Wall without openings.

The library dependency is `sitehelper_surface -> sitehelper_wall/sitehelper_model`.
There are no reverse dependencies or project, topology, GUI/render, take-off,
cut-list or optimisation dependencies. The intended future flow is:

```text
Wall authoritative geometry
          ↓
      WallSurface
          ↓
future CladdingDefinition
          ↓
future CladdingLayout
          ↓
future cladding material/cut optimisation
```

Future cladding layout consumes this surface rather than generated framing.
Boards, staggering, cladding products/specifications, clearances, flashing,
corner details, installation order, take-off, stock optimisation, pricing and
GUI tools are deferred. No generic surface hierarchy or connection to the
linear optimiser is introduced here.
