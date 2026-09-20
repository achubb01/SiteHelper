SiteHelper measurement semantics
===============================

**Authoritative construction distances, dimensions and physical coordinates are
integer millimetres unless explicitly documented otherwise.** Units belong to the
type/API contract; never infer them from a field's name, magnitude or source.

This covers plan wall/separator endpoints, Room locations, Storey elevations,
wall-local U/Z positions, opening dimensions and allowances, timber dimensions
and positions, and construction settings including stud/noggin spacing and
Storey height overrides, slab exterior/penetration/region outlines, thicknesses and relative top levels. IDs, counts, indices, flags and modes are not distances.
Existing ordinary integers are intentional: the domain has no runtime unit tags,
unit-carrying wrappers, or per-value unit metadata.

Representation and coordinate space
-----------------------------------

| Boundary/value | Representation and unit |
| --- | --- |
| `PlanPosition`, segment endpoints, Room location | Authoritative integer plan X/Y millimetres. |
| `WallLocalPosition`, Opening, Timber | Integer wall-local U/Z and physical dimensions in millimetres; generated framing retains these units. |
| `Storey.elevation_mm` | Integer vertical offset from Project datum; Wall Z=0 is relative to that Storey plane. |
| Slab exterior/penetration/region outlines, thickness, top-level offset | Authoritative integer plan X/Y and physical millimetres; top offset is relative to Storey elevation. |
| Slab edge rebates | Authoritative outer-edge index plus integer-mm local U interval, inward width and local-top-relative depth. |
| Slab quantities | Derived exact doubled area (half mm²) and volume (half mm³). Legacy volumes use base thickness; construction volume applies replacement regions. Edge lengths/outer perimeter use floating-point millimetres. |
| Wall length | Integer millimetres derived by rounding endpoint Euclidean distance; not independently stored. |
| `PlanPoint`, editor pointer/previews, transforms, snap calculations | Calculated/transient millimetres, potentially fractional `double`; not authoritative persisted dimensions. |
| Topology/junction vertices | Derived exact rational X/Y millimetres; source parameters `t` are dimensionless (`U = t * wall_length_mm`). |
| `Vec2`, `Rect2`, generic grid/snap helpers | Caller-owned geometry units/spaces; types and algorithms do not assume millimetres. |
| SiteHelper editor snap settings/results | Physical millimetres, including grid spacing and object tolerance; not fixed pixel tolerances. |
| Viewport, GUI layout/HUD and renderer backend geometry | Screen pixels. Camera scale is pixels per world unit (pixels/mm in SiteHelper); zoom factors are dimensionless. |

A `double` does not imply metres, and an integer does not necessarily represent a
physical measurement. Camera unprojection converts screen positions to physical
view millimetres before editor calls. Grid rendering chooses world spacing to
meet a minimum **pixel** separation; it does not change editor snapping spacing.
Existing mouse quantization, wall-length derivation and exact numeric-length
resolution retain their own geometric contracts; they are not unit conversion.

Input and display
-----------------

GUI/application construction input uses the existing
[`length_parse_mm()`](../gui/length_parse.h): bare numbers and `mm` mean
millimetres, while `m` converts to millimetres. `4200`, `4200mm` and `4.2m` all
produce integer 4200. Conversion must consume the complete input, fit the integer
range and preserve whole millimetres exactly. Fractional millimetres, malformed
suffixes and overflow are rejected, never silently rounded or truncated. Signs
and zero are representable; the receiving domain/tool decides whether they are
valid for that operation. Non-measurement menu choices and IDs are not lengths.

Unit strings and any display formatting remain at GUI/application boundaries.
Editor intents, commands/history and Project/Wall/model APIs receive typed integer
millimetres. The existing parser also serves CLI construction inputs; there is no
second unit parser or speculative formatting framework. The current numeric HUD
continues to show the user's entry and unit/validation feedback.

Persistence and external import
-------------------------------

Persisted authoritative physical measurements are integer millimetres without
unit markers. Current format **16** and supported legacy formats **1–15** use the
same physical unit contract. Existing legacy wall/origin and opening-reference migrations reinterpret
geometry within millimetres, not through unit rescaling. No format/version change
is needed for this contract; existing files keep their meaning. Derived framing,
previews, camera state and topology are not authoritative persisted measurements.

A future importer must resolve the external file's units and normalize physical
values **before** calling the core APIs:

```text
external CAD coordinates/units → importer interpretation/conversion
                              → checked SiteHelper integer millimetres
                              → authoritative Project/model state
```

The importer must resolve unspecified units explicitly and report overflow or
non-whole-millimetre results instead of silently rounding. The core does not know
whether input began as metres, centimetres, inches, feet or drawing units. This
contract adds no CAD importer or imperial user-input syntax.
