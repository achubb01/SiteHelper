# Priority 26B — Roof Geometry Acceptance Fixtures

This document freezes the geometry acceptance cases for Priority 26 before a
persisted roof representation or generator API is designed.

The fixtures are deliberately **representation-neutral**. They specify source
intent, exact plan coordinates, vertical references, composition semantics and
expected derived geometry. They do not prescribe C structs, persistence fields,
array order, DomainIds, editor commands or the final numeric representation of
roof pitch.

A future roof model/generator proposal is acceptable only if it can express all
six fixture families without storing the expected ridge/hip/valley network as
co-equal authoritative input.

## Fixture notation and comparison rules

### Coordinate system and units

All authoritative plan coordinates and explicit vertical offsets in these
fixtures are integer millimetres, consistent with `model/MEASUREMENTS.md`.

For fixture descriptions only:

- `X` and `Y` are plan coordinates in millimetres;
- `Z` is vertical elevation in millimetres relative to a fixture-local datum;
- support polygons are listed counter-clockwise;
- a polygon list does not repeat its first vertex at the end;
- `P(a,b)` means plan point `(X=a, Y=b)`;
- `V(a,b,c)` means 3D point `(X=a, Y=b, Z=c)`;
- `t(p)` means `tan(p)` where `p` is a pitch in degrees.

The use of degrees and `tan()` in this document is descriptive, not a decision
that persisted roof pitch shall be stored as floating-point degrees. Priority
26C owns that decision.

### Source extent versus final roof surface

A source support polygon describes the plan extent used to author a roof
portion. It is not automatically the final visible roof polygon. Composition may
clip some source surfaces, and an overhang/projection policy may later extend a
surface beyond its support polygon.

Unless a fixture variant explicitly says otherwise, projections are zero. This
keeps the core topology fixtures focused on roof-plane generation and
composition. Projection is tested separately in Fixture A-P1.

### Vertical references

Each source portion states an explicit line or point whose elevation is known.
No fixture derives that datum from Storey elevation, wall stud height, the next
Storey, or another unrelated building property.

### Expected geometry is set-based

Acceptance must not depend on internal array ordering, source insertion order or
the eventual names of C enum values. A generator may choose a different stable
ordering so long as the geometric result is equivalent.

Tests should compare, as appropriate:

- visible roof-plane regions;
- 3D supporting planes;
- ridge/hip/valley/intersection/termination edges;
- edge endpoints and connectivity;
- boundary roles;
- visibility/clipping of source surfaces; and
- absence of edges that should not exist.

Coplanar regions may be represented as one polygon or multiple exactly-adjacent
patches if they are geometrically equivalent and no false seam is exposed.
Likewise, triangulation used internally for rendering is not part of these
acceptance fixtures.

### Derived edge terminology used by the fixtures

The labels below describe expected semantics; they do not freeze a future API
enum:

- **ridge** — high convex intersection of roof planes;
- **hip** — sloping convex intersection running from an outer corner toward a
  higher roof feature;
- **valley** — sloping concave intersection of roof planes;
- **eave** — draining outer roof boundary;
- **gable/verge** — outer boundary at a gable end;
- **high edge** — upper external boundary of a skillion;
- **termination/cut** — intentional end of a roof portion that is not itself a
  ridge, hip or valley;
- **step/abutment** — relation between roof portions that share plan position but
  not elevation, or intentionally meet without being solved as a plane-plane
  intersection;
- **pitch-break/intersection** — a plane-plane seam that does not naturally
  classify as ridge, hip or valley.

Vertical closure faces that may be required to complete a weather/building
envelope are called out where relevant, but they are **not counted as sloping
roof planes**.

## Fixture summary

| Fixture | Purpose | Source portions | Expected visible sloping planes |
| --- | --- | ---: | ---: |
| A | Simple gable + orientation ambiguity | 1 | 2 |
| B | Equal-pitch hip | 1 | 4 |
| C | Intersecting perpendicular gables / valleys | 2 | 4 |
| D | Dutch gable / gablet termination | 1 compound intent | 3 |
| E | Skillion / explicit fall direction | 1 | 1 |
| F | Multi-level composition and non-ridge intersections | 2 | 3 |

The plane counts above describe canonical visible surfaces for the stated
zero-projection base cases. They do not require a renderer to use exactly that
number of polygons internally.

# Fixture A — Simple gable

## A0 — canonical rectangular gable

### Authoritative source intent

Support polygon `A`:

```text
P(0,0)
P(12000,0)
P(12000,8000)
P(0,8000)
```

Intent:

```text
form intent:       two-sided gable
pitch:              22.5° on both sides
ridge orientation:  parallel to +X
south boundary:     eave
north boundary:     eave
west boundary:      gable/verge
east boundary:      gable/verge
vertical reference: Z = 0 along both eave support lines
projection:          zero
```

The fixture does not prescribe whether `two-sided gable` survives as a persisted
field. A future authoring preset may produce more primitive slope/boundary
intent. What matters is that the source intent determines the result without
persisting the ridge as independent truth.

### Expected derived geometry

The ridge plan segment is:

```text
P(0,4000) -> P(12000,4000)
```

and its elevation is:

```text
Zridge = 4000 * t(22.5°)
```

Expected visible roof planes:

1. south plane, supported in plan by
   `[(0,0), (12000,0), (12000,4000), (0,4000)]`;
2. north plane, supported in plan by
   `[(0,4000), (12000,4000), (12000,8000), (0,8000)]`.

Expected edge topology:

```text
ridges:   1
hips:     0
valleys:  0
outer eaves: 2
outer gable/verge sides: 2
```

The two planes meet only at the ridge. Ridge height is derived from run and
pitch; it is not an independently editable second height input.

## A1 — square-footprint orientation ambiguity

Use support polygon:

```text
P(0,0)
P(8000,0)
P(8000,8000)
P(0,8000)
```

with 22.5° equal pitch and zero projection.

Two geometrically valid gables exist:

```text
case X: ridge P(0,4000)    -> P(8000,4000)
case Y: ridge P(4000,0)    -> P(4000,8000)
```

Acceptance rule: footprint + pitch alone is **ambiguous**. A proposed source
model must require enough orientation/boundary intent to distinguish these
results. It must not silently choose one from coordinate ordering or whichever
axis happens to be longer, because the square has no longer axis.

## A-P1 — independent projection intent

Reuse A0 but add:

```text
south eave projection: 600 mm
north eave projection: 600 mm
west barge projection: 450 mm
east barge projection: 450 mm
```

For this simple non-compound case, the projected exterior is expected to be:

```text
south plane plan boundary:
  (-450,-600) (12450,-600) (12450,4000) (-450,4000)

north plane plan boundary:
  (-450,4000) (12450,4000) (12450,8600) (-450,8600)

projected ridge:
  P(-450,4000) -> P(12450,4000)

projected south/north eave elevation:
  Z = -600 * t(22.5°)
```

Acceptance rule: changing eave projection must not require changing the support
polygon, pitch or ridge orientation. Changing barge projection must not be
mistaken for changing wall/support extent. Compound-roof projection joins remain
a later policy, but this simple case freezes the basic distinction between
support geometry and final projected surface geometry.

# Fixture B — Equal-pitch hip

## B0 — canonical rectangular hip

### Authoritative source intent

Support polygon `B`:

```text
P(0,0)
P(12000,0)
P(12000,8000)
P(0,8000)
```

Intent:

```text
form intent:       equal-pitch hip
pitch:              22.5° on all four sides
all boundaries:     eaves
vertical reference: Z = 0 on the support perimeter
projection:          zero
```

### Expected derived geometry

Central ridge:

```text
P(4000,4000) -> P(8000,4000)
Zridge = 4000 * t(22.5°)
```

Hip edges:

```text
P(0,0)     -> P(4000,4000)
P(0,8000)  -> P(4000,4000)
P(12000,0) -> P(8000,4000)
P(12000,8000) -> P(8000,4000)
```

Canonical visible plane regions in plan:

```text
south:
  (0,0) (12000,0) (8000,4000) (4000,4000)

north:
  (0,8000) (4000,4000) (8000,4000) (12000,8000)

west hip-end:
  (0,0) (4000,4000) (0,8000)

east hip-end:
  (12000,0) (12000,8000) (8000,4000)
```

Expected edge topology:

```text
visible sloping planes: 4
ridges:                 1
hips:                   4
valleys:                0
outer eaves:            4
```

Acceptance rule: ridge endpoints and hip segments are consequences of equal
pitch plus support/boundary intent. A proposed model fails this fixture if those
same edges must also be entered as independent authoritative geometry.

# Fixture C — Intersecting perpendicular gables

This fixture family proves derived valleys, clipping, compound source portions
and non-integer intersections when pitches differ.

## C0 — equal pitch

### Authoritative source intent

Main gable source `C-main`:

```text
support polygon:
  P(0,0)
  P(12000,0)
  P(12000,8000)
  P(0,8000)

ridge orientation: parallel to +X
pitch:              22.5°
vertical reference: Z = 0 on Y=0 and Y=8000 eave lines
```

Perpendicular projecting gable source `C-wing`:

```text
support polygon:
  P(4000,4000)
  P(8000,4000)
  P(8000,11000)
  P(4000,11000)

ridge orientation: parallel to +Y at X=6000
pitch:              22.5°
vertical reference: Z = 0 on X=4000 and X=8000 eave lines
```

Composition intent:

```text
C-wing INTERSECTS C-main
visible result is clipped at plane intersections
```

This composition relationship is authoritative intent. Plan overlap alone must
not be treated as sufficient proof that two sources should intersect.

### Expected derived geometry

The main ridge remains:

```text
P(0,4000) -> P(12000,4000)
```

The wing ridge is visible from the valley junction outward:

```text
P(6000,6000) -> P(6000,11000)
```

The two valleys are:

```text
P(4000,8000) -> P(6000,6000)
P(8000,8000) -> P(6000,6000)
```

The valley junction elevation is:

```text
Z = 2000 * t(22.5°)
```

Canonical visible plan regions:

```text
main south plane:
  (0,0) (12000,0) (12000,4000) (0,4000)

main north plane:
  (0,4000)
  (12000,4000)
  (12000,8000)
  (8000,8000)
  (6000,6000)
  (4000,8000)
  (0,8000)

wing west plane:
  (4000,8000)
  (6000,6000)
  (6000,11000)
  (4000,11000)

wing east plane:
  (6000,6000)
  (8000,8000)
  (8000,11000)
  (6000,11000)
```

Expected topology:

```text
visible sloping planes: 4
main ridges:            1
wing ridges:            1
valleys:                2
valley/ridge junctions: 1 at P(6000,6000)
hips:                   0
```

The source portions overlap in plan over
`X=[4000,8000], Y=[4000,8000]`, but hidden source-surface areas are not part of
the exterior result.

## C1 — unequal pitch, same source geometry

Reuse C0 except:

```text
C-main pitch = 22.5°
C-wing pitch = 30°
```

The valley start points remain:

```text
P(4000,8000)
P(8000,8000)
```

The two valleys still converge on the wing ridge `X=6000`, but the junction
moves to:

```text
Yj = 8000 - 2000 * t(30°) / t(22.5°)
```

Numerically, this is approximately `5212.306 mm`; that decimal value is
**informative only** and must not be used to choose the 26C numeric contract.

Expected valley segments are therefore:

```text
P(4000,8000) -> P(6000,Yj)
P(8000,8000) -> P(6000,Yj)
```

and the visible wing ridge becomes:

```text
P(6000,Yj) -> P(6000,11000)
```

Acceptance rule: changing only one source pitch moves the derived valley network
without altering the source support polygons. No persisted valley coordinate is
edited to make that happen.

This subcase intentionally creates a non-integer intersection and is an input to
Priority 26C's numeric investigation.

# Fixture D — Dutch gable / gablet termination

This fixture is intentionally asymmetric: the west end is Dutch-gabled and the
east end is a normal gable. That isolates the termination behaviour without
adding an unrelated second hip end.

## D0 — west Dutch-gable termination

### Authoritative source intent

Support polygon `D`:

```text
P(0,0)
P(12000,0)
P(12000,8000)
P(0,8000)
```

Main intent:

```text
main pitch:          22.5°
main ridge:          parallel to +X
south/north lines:   eaves at Z=0
west lower end:      hipped
east end:            gable/verge
west hip termination station: X=2000
projection:           zero
```

The `X=2000` termination is intentional source information. It is not inferred
from the rectangle or pitch.

### Expected derived geometry

West hip edges:

```text
P(0,0)    -> P(2000,2000)
P(0,8000) -> P(2000,6000)
```

Both hip endpoints have elevation:

```text
Zcut = 2000 * t(22.5°)
```

The lower hip plane terminates on the cut segment:

```text
P(2000,2000) -> P(2000,6000)
```

at `Z=Zcut`.

The main ridge is:

```text
P(2000,4000) -> P(12000,4000)
Zridge = 4000 * t(22.5°)
```

Canonical visible sloping plan regions:

```text
south plane:
  (0,0)
  (12000,0)
  (12000,4000)
  (2000,4000)
  (2000,2000)

north plane:
  (0,8000)
  (2000,6000)
  (2000,4000)
  (12000,4000)
  (12000,8000)

west lower hip plane:
  (0,0)
  (2000,2000)
  (2000,6000)
  (0,8000)
```

Expected sloping-roof topology:

```text
visible sloping planes: 3
ridges:                 1
hips:                   2
valleys:                0
termination/cut edges:  1
west gablet closure:    required by the envelope, but not a sloping roof plane
```

The vertical gablet closure has the 3D triangular boundary:

```text
V(2000,2000,Zcut)
V(2000,6000,Zcut)
V(2000,4000,Zridge)
```

Acceptance rule: a roof-geometry result may expose enough information for a
building-envelope consumer to construct that vertical closure, but the closure
must not be misclassified as another sloping roof plane merely to make the model
work.

# Fixture E — Skillion

## E0 — explicit fall direction

### Authoritative source intent

Support polygon `E`:

```text
P(0,0)
P(8000,0)
P(8000,6000)
P(0,6000)
```

Intent:

```text
form intent:       single sloping plane
pitch:              7.5°
downhill direction: -Y
low edge:           Y=0
high edge:          Y=6000
vertical reference: Z=0 along Y=0
projection:          zero
```

### Expected derived geometry

The whole support polygon is one visible roof plane.

Boundary elevations:

```text
Y=0:    Z=0
Y=6000: Z=6000 * t(7.5°)
```

Expected topology:

```text
visible sloping planes: 1
ridges:                 0
hips:                   0
valleys:                0
low eave edges:         1
high edges:             1
side verge edges:       2
```

## E1 — fall-direction reversal

Keep the same support polygon and pitch but reverse the downhill direction to
`+Y`, using `Z=0` along `Y=6000` as the low-edge reference.

Acceptance rule: E0 and E1 have the same footprint and pitch magnitude but are
different roofs. A model that cannot distinguish them without moving plan
vertices fails the fixture.

# Fixture F — Multiple levels and explicit composition

Fixture F deliberately has two subcases because a single overlap rule cannot
represent both common situations. F0 tests a vertical step/abutment with no
plane-plane solve. F1 tests an actual plane intersection at different pitch and
reference elevation.

## F0 — lower attached skillion with a step/abutment

### Authoritative source intent

Main source `F-main` is the same gable as A0:

```text
support: (0,0) (12000,0) (12000,8000) (0,8000)
pitch: 22.5°
ridge: parallel +X
south/north eaves: Z=0
```

Lower skillion source `F-lean`:

```text
support polygon:
  P(2000,-3000)
  P(10000,-3000)
  P(10000,0)
  P(2000,0)

pitch:              5°
downhill direction: -Y
high edge:           Y=0
vertical reference: Z=-300 along Y=0
```

Composition intent:

```text
F-lean ABUTS F-main along plan segment
P(2000,0) -> P(10000,0)
with an intentional 300 mm vertical step.
```

### Expected derived geometry

The main gable remains geometrically unchanged. The lower skillion remains one
plane. Along the abutment there are **two distinct 3D edges with the same plan
projection**:

```text
main edge:    V(2000,0,0)    -> V(10000,0,0)
skillion edge:V(2000,0,-300) -> V(10000,0,-300)
```

Expected topology:

```text
visible sloping planes: 3 total
new ridges from lean-to:   0
new hips:                  0
new valleys:               0
step/abutment relations:   1
```

A vertical closure/flashing/upstand may ultimately be required between those
edges, but it is not a sloping roof plane.

Acceptance rule: coincident plan edges do not imply equal elevation and do not
collapse into one 2D topology edge.

## F1 — overlapping skillion with unequal pitch/elevation, explicit intersection

Main source `F-main` remains A0.

Secondary source `F-overlap`:

```text
support polygon:
  P(3000,7000)
  P(9000,7000)
  P(9000,12000)
  P(3000,12000)

pitch:              10°
downhill direction: +Y
high reference line:Y=7000
high-line elevation:Z=250
```

Composition intent:

```text
F-overlap INTERSECTS F-main
visible surfaces are clipped at their 3D intersection.
```

The sources overlap in plan over:

```text
X=[3000,9000]
Y=[7000,8000]
```

Within that overlap, the main north plane is:

```text
Zmain(Y) = (8000 - Y) * t(22.5°)
```

and the skillion is:

```text
Zlean(Y) = 250 - (Y - 7000) * t(10°)
```

They intersect at:

```text
Yi =
  (250 + 7000*t(10°) - 8000*t(22.5°))
  / (t(10°) - t(22.5°))
```

which is approximately `7690.302 mm` for human inspection only.

Expected visible intersection seam:

```text
P(3000,Yi) -> P(9000,Yi)
```

Canonical visible plan regions are:

```text
main south plane:
  (0,0) (12000,0) (12000,4000) (0,4000)

main north plane:
  (0,4000)
  (12000,4000)
  (12000,8000)
  (9000,8000)
  (9000,Yi)
  (3000,Yi)
  (3000,8000)
  (0,8000)

secondary skillion plane:
  (3000,Yi)
  (9000,Yi)
  (9000,12000)
  (3000,12000)
```

The portions of the skillion side boundaries `X=3000` and `X=9000` from
`Y=Yi` outward are termination/exterior boundaries. Within the main-roof overlap
they can imply vertical closure between surfaces, but those closure faces are not
sloping roof planes.

This seam is a **pitch-break/intersection**, not a ridge, hip or valley. Both
planes fall generally toward `+Y`; the outer envelope merely changes from the
steeper main plane to the shallower secondary plane.

Expected topology:

```text
visible sloping planes: 3 total
main ridge:               unchanged
new plane-plane seams:    1
new hips:                  0
new valleys:               0
```

The main north source surface is hidden inside `F-overlap` where the skillion is
above it. The skillion source surface is hidden in the part of the plan overlap
where the main roof is above it.

Acceptance rule: the clipping decision comes from explicit `INTERSECTS` intent.
The generator must not infer `INTERSECTS`, `ABUTS` or `OVERLAYS` solely from plan
polygon overlap.

# Cross-fixture acceptance requirements

A roof representation proposed after 26B must satisfy all of the following
before it is accepted for implementation:

1. **No authoritative ridge/hip/valley duplication.** Fixtures A, B and C must be
   generatable from source intent without entering their expected derived edge
   coordinates as second authoritative inputs.
2. **Orientation is explicit.** A1 and E1 cannot depend on rectangle aspect ratio,
   vertex order or global X/Y conventions to choose a roof direction.
3. **Vertical datum is explicit.** E0, F0 and F1 cannot derive placement from wall
   stud height or Storey spacing.
4. **Compound composition is explicit.** C and F must distinguish source plan
   overlap from intentional `INTERSECTS`/`ABUTS`/future `OVERLAYS` semantics.
5. **Derived clipping is deterministic.** Hidden source-surface areas in C and F1
   do not survive as visible exterior roof geometry.
6. **Vertical closures are not roof planes.** D0 and F0 may expose closure
   boundaries, but a vertical gable/step face is outside the sloping roof-plane
   count.
7. **Non-integer intersections are legitimate.** C1 and F1 are not rounded to
   integer plan coordinates merely because authoritative source coordinates are
   integer millimetres. Priority 26C must define a deterministic transient
   numeric representation.
8. **Result ordering is irrelevant.** Reordering source portions must not change
   geometric output.
9. **Source direction is irrelevant unless semantically declared.** Reversing
   polygon vertex order after normalization must not reverse fall/ridge intent;
   that intent is explicit.
10. **No standards limit is a geometry limit.** These fixtures are geometry
    acceptance cases only. A later standards module may report a method
    inapplicable without making otherwise representable roof geometry invalid.

# Negative / ambiguity checks

The following inputs must not silently produce an arbitrary roof:

- square gable footprint + pitch with no ridge/fall orientation;
- skillion footprint + pitch with no fall direction;
- compound plan overlap with no composition relationship;
- source portion with no usable vertical reference;
- contradictory boundary/slope intent that cannot define a coherent plane;
- a Dutch-gable request with no termination station/height/other equivalent
  termination constraint.

The eventual API may return different detailed error codes, but these cases are
part of 26B's acceptance contract.

# What 26B deliberately does not decide

The following remain open for Priority 26C or later:

- whether pitch authority is rise/run, fixed-point angle, another exact ratio or
  some checked canonical form;
- whether transient roof vertices use rational coordinates, a fixed-point 3D
  representation, checked doubles, or a mixed representation;
- the final `RoofDefinition` / source-portion structs;
- persistent identity and Storey ownership;
- exact overhang joining rules for compound roofs;
- plane/edge API naming and enum values;
- topology data-structure choice;
- rendering tessellation;
- conventional rafter or truss generation;
- RLW/ULW, wind zones, reactions, member sizing or standards compliance.

# 26B completion gate

Priority 26B is complete when these fixtures are accepted as the geometry contract
for the next prototype and no proposed source representation is allowed to bypass
them with roof-style-specific generated data.

The next task is **Priority 26C — choose and prove the roof slope/plane numeric
contract**. Its first prototype should use A0 plus C1 (and preferably F1) because
they cover both simple exact source geometry and non-integer derived
intersections without requiring a persisted roof model.
