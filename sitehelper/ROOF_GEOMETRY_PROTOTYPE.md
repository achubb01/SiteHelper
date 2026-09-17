# Priority 26D — Minimal Roof Intent / Geometry Prototype

Status: **accepted isolated prototype**. This priority proves the smallest source
intent and owned derived-geometry snapshot needed for the simple gable, equal-
pitch hip and skillion fixtures. It does not introduce project/storey ownership,
DomainIds, commands, persistence, editor tooling, structural framing or standards
analysis.

The prototype implementation is:

```text
roof/roof_geometry_prototype.h
roof/roof_geometry_prototype.c
roof/tests/test_roof_geometry_prototype.c
```

It deliberately remains a separate CMake target named
`sitehelper_roof_geometry_prototype` until Priority 26E proves that the same
abstraction can survive compound/intersecting roofs.

## What 26D had to prove

The 26B fixtures require simple residential roofs to be describable without
persisting their consequences. For this phase the generator therefore had to
prove all of the following:

- A0: a rectangular two-sided gable generates two visible planes and one ridge;
- A1: a square gable is not resolved from footprint shape alone — an explicit
  source orientation chooses the ridge axis;
- B0: an equal-pitch rectangular hip generates four visible planes, four hips
  and its ridge without any ridge/hip source fields;
- E0/E1: the same skillion support rectangle and pitch produce distinct roofs
  when the downhill direction is reversed;
- derived ridge/apex coordinates may be fractional and are not rounded back to
  authoritative integer millimetres; and
- a failed rebuild leaves a previous successful derived snapshot untouched.

The implementation passes those checks with the 26C fixed-gradient contract.

## Prototype source authority

The prototype source is `RoofPrototypeIntent`:

```text
borrowed support rectangle
+ generation intent
+ canonical fixed slope
+ integer-mm vertical reference
+ explicit plan direction where required
```

Nothing else is authoritative.

In particular, the source does **not** contain:

```text
ridge endpoints
hip endpoints
roof-plane polygons
high-edge elevation
plane equations
boundary classifications
```

Those are generated every time.

### Support geometry

26D intentionally accepts only one axis-aligned rectangle expressed by its four
integer-mm corners. Input start vertex and winding are irrelevant; the generator
canonicalizes the rectangle to plan bounds before deriving geometry.

This is a prototype restriction, not a proposed final roof-domain limitation.
Compound source polygons and overlapping source portions belong to 26E.

### Generation intent

The prototype uses three generation operations:

```text
OPPOSING_SLOPES
ALL_BOUNDARY_SLOPES
SINGLE_SLOPE
```

For the current rectangle fixtures these correspond to the geometry needed for
simple gable, equal-pitch hip and skillion roofs respectively.

This enum is **not** accepted as a future persisted global `RoofType`. It is a
small generator instruction used to test whether higher-level source intent can
produce the required geometry. If 26E requires adding values such as `VALLEY`,
`DUTCH_GABLE` or one enum value per named compound roof style, the abstraction
has failed and must be revised before persistence.

### Direction intent

`RoofPrototypeDirection` is a plan-space unit direction.

For `OPPOSING_SLOPES` it identifies the ridge axis. Its sign is irrelevant to
the resulting simple gable, but supplying the axis is mandatory. This is what
makes A1 deterministic instead of guessing from a square footprint.

For `SINGLE_SLOPE` it identifies the downhill direction. The sign is significant
and distinguishes E0 from E1 while keeping the same support geometry and pitch.

`ALL_BOUNDARY_SLOPES` needs no direction for the current equal-pitch rectangle
case and requires the direction to be zero.

26D only accepts axis-aligned directions because every frozen simple fixture is
axis aligned. Arbitrary plan direction is still required before this becomes a
production authoring model.

### Vertical reference

The source carries one signed integer-mm `reference_z_mm`.

For the gable and hip prototypes it is the eave/support datum. For a skillion it
is the low-edge datum. Ridge height, hip height and the skillion high-edge height
are derived from this datum and the canonical slope.

The field is explicitly roof-local prototype input. It is not inferred from
`Storey.elevation`, `BuildSettings.stud_height`, a neighbouring storey or wall
framing.

## Derived geometry snapshot

`RoofPrototypeGeometry` owns three kinds of generated data:

```text
visible roof planes
outer support-boundary classifications
interior ridge/hip edges
```

The complete result is transient. It owns its allocations, receives no
`DomainId`, and becomes invalid after destruction/rebuild.

### Visible planes

Every generated plane contains:

- the 26C integer affine plane equation;
- an owned visible polygon; and
- exact rational X/Y/Z vertex coordinates.

The plane equation remains:

```text
1,000,000 * Z = Gx * X + Gy * Y + K
```

No trigonometry or epsilon comparison occurs in generation.

### Exact derived coordinates

Authority remains integer millimetres, but generated geometry may not be.
26D deliberately tests a gable with an odd support width so its ridge lies at a
half-millimetre plan coordinate. The result remains an exact rational instead of
being snapped or rounded.

The 26D prototype uses checked `int64_t` rational arithmetic because the simple
fixture bounds are small and the phase does not yet solve arbitrary plane-plane
intersections. This does **not** replace the 26C requirement for a portable
checked wide-integer exact path when 26E starts producing compound intersections.

### Outer boundaries versus interior edges

The result separates outer source-boundary semantics from interior generated
edges.

Outer boundaries are still straight plan support segments and are classified as
one of the prototype semantics needed by A/B/E:

```text
eave
gable/verge
low eave
high edge
side verge
```

Interior edges are currently:

```text
ridge
hip
```

This separation avoids pretending that a whole gable-end boundary is one
straight 3D edge: its plan support edge can cross the derived ridge while the
visible plane polygons retain the actual 3D segmentation.

## Fixture results

### A0 — simple gable

Input authority:

```text
support:     (0,0) (12000,0) (12000,8000) (0,8000)
generation:  OPPOSING_SLOPES
ridge axis:  +X
slope:       414214 / 1000000
reference Z: 0 mm
```

Derived:

```text
planes: 2
ridge:  (0,4000) -> (12000,4000)
ridge Z: 207107 / 125 mm
outer boundaries: 2 eave + 2 gable/verge
```

The source has no ridge coordinate or ridge height.

### A1 — square orientation ambiguity

For an 8000 x 8000 support rectangle, changing only source direction from `+X`
to `+Y` changes the generated ridge from:

```text
(0,4000) -> (8000,4000)
```

to:

```text
(4000,0) -> (4000,8000)
```

A zero direction is rejected. The prototype therefore cannot silently infer a
ridge from longest-axis logic or input vertex order.

### B0 — equal-pitch hip

Input authority contains the rectangle, equal slope and support datum only.
Derived geometry contains:

```text
4 visible planes
1 ridge
4 hips
4 eave boundaries
```

For the 12000 x 8000 fixture, the generated ridge is exactly:

```text
(4000,4000) -> (8000,4000)
```

A square support naturally collapses the ridge to one apex. The generator emits
four hips and no zero-length ridge; there is no separate persisted "pyramid"
case.

### E0/E1 — skillion direction

For E0 (`downhill = -Y`) the canonical plane gradient is:

```text
Gx = 0
Gy = +131652
```

The `Y=0` boundary is the low eave and `Y=6000` is the high edge.

For E1 (`downhill = +Y`) the same support/pitch instead generates:

```text
Gx = 0
Gy = -131652
```

with `Y=6000` as the low eave. No plan vertices are moved to encode the reversal.

## Ownership and transactional behaviour

`roof_prototype_build()` follows the same direction as established SiteHelper
generators:

```text
validate source
    -> build complete candidate snapshot
    -> on success destroy old output and replace it
    -> on failure destroy candidate and preserve old output
```

The support vertex pointer is borrowed for the duration of the build only. No
borrow survives success. All plane polygons and result arrays are owned by the
snapshot and released by `roof_prototype_geometry_destroy()`.

Repeated destroy is safe for a successful/zeroed snapshot.

## What the prototype deliberately does not solve

26D is not a production roof API. It deliberately omits:

- arbitrary/simple polygons beyond one axis-aligned rectangle;
- edge-specific eave/barge projection from A-P1;
- arbitrary ridge/fall vectors;
- multiple source portions;
- overlap/intersection/abutment/termination composition;
- valleys;
- Dutch-gable/gablet termination;
- multi-level roofs;
- support relationships to Walls/Beams/Posts;
- covering or structural-system specification;
- project/storey ownership and DomainIds;
- commands/history;
- persistence;
- rendering/editor integration; and
- framing or standards calculations.

These omissions are the gate for 26E rather than hidden assumptions.

## Architectural decision from 26D

26D supports the candidate architecture from discovery:

```text
small authoritative roof intent
          ↓
deterministic geometry generator
          ↓
owned transient roof geometry
```

It also demonstrates that a source need not store ridge/hip geometry to produce
useful exact plane topology for simple forms.

The source type itself is **not frozen for persistence**. The important result is
the boundary it proves:

- support extent, slope/orientation and vertical reference belong on the source
  side;
- planes, ridges, hips, boundary classifications and derived heights belong on
  the generated side; and
- generated geometry can carry exact fractional coordinates without weakening
  the integer-mm authority rule.

## 26E handoff

Priority 26E must now challenge this abstraction with C0/C1, D0 and F0/F1.
The first step should be **source portions plus composition semantics**, not an
`INTERSECTING_GABLE` generator enum.

26E should answer whether the simple `RoofPrototypeIntent` evolves naturally
into a collection of source portions or whether a different primitive is
required. It must also introduce the portable checked exact arithmetic needed
for arbitrary plane intersections.

Do not add persistence, Storey ownership or editor commands until those compound
fixtures pass without named-style special casing.
