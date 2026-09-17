# Priority 26E3 — Multi-level / Abutment / Intersection Prototype

Priority 26E3 completes the compound-geometry discovery fixtures with F0 and F1.
It is still a prototype boundary, not the persisted roof domain.

The question tested here is deliberately narrow:

> If two roof portions occupy related or overlapping plan space at different
> elevations, can source composition intent distinguish a step/abutment from a
> true 3D surface intersection without guessing from plan overlap?

The answer from F0/F1 is yes, with two refinements to the earlier prototype.

## Source composition remains explicit

`RoofPrototypeCompositionKind` now distinguishes:

```text
INTERSECTS
ABUTS
```

`INTERSECTS` means visible source surfaces are resolved by their actual 3D
plane intersection and clipped accordingly. `ABUTS` means the portions meet by
an authored interface and remain distinct surfaces; coincident plan position
must not force equal elevation or a plane-plane solve.

No composition is inferred from plan overlap or plan contact. `OVERLAYS` is not
implemented because none of the frozen 26B fixtures require it. A future source
case should earn that semantic rather than adding it speculatively.

## F0 — vertical step / abutment

F0 combines the A0 gable with a lower skillion. The skillion's high edge lies on
`Y=0`, directly below part of the main south eave in plan:

```text
main interface edge:     X=[2000,10000], Y=0, Z=0
skillion interface edge: X=[2000,10000], Y=0, Z=-300
```

The prototype therefore cannot model the join as one topology edge. It adds a
transient `RoofPrototypeInterface` with
`ROOF_PROTOTYPE_INTERFACE_STEP_ABUTMENT`, containing both physical 3D edges.
The vertical closure/flashing/upstand between them remains an envelope detail,
not a sloping roof plane.

F0 derives three visible sloping planes total, retains the main ridge, and adds
no hip, valley or plane-intersection seam.

## F1 — true 3D intersection at unequal elevation and pitch

F1 combines the A0 gable with a 10 degree skillion whose high reference edge is
`Y=7000, Z=250`. The source relation is `INTERSECTS`.

Using the 26C canonical slopes:

```text
main       414214 / 1000000
overlap    176327 / 1000000
```

the derived seam is exactly:

```text
Y = 1829423000 / 237887 mm
Z = 15258194011 / 118943500 mm
```

The seam is emitted as `ROOF_PROTOTYPE_INTERIOR_PLANE_SEAM`, not as a ridge,
hip or valley. The main north surface and skillion are clipped to that exact
rational line. The seam Z is evaluated identically from both source plane
equations.

F1 therefore produces:

```text
visible sloping planes: 3
main ridge:              1
plane-plane seams:       1
hips:                    0
valleys:                 0
```

## Vertical datum refinement discovered by E3

The 26D single-slope prototype treated `reference_z_mm` as the low-edge datum.
That is insufficient for the frozen F fixtures, both of which author the high
edge explicitly.

The prototype now adds:

```text
ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_LOW_EDGE
ROOF_PROTOTYPE_SINGLE_SLOPE_REFERENCE_HIGH_EDGE
```

This is not a final persisted enum. It establishes the requirement that a
single-slope source must identify **which parallel boundary owns the authored
vertical datum**. Direction plus one unlabeled Z value is not enough.

## Boundary representation issue exposed by F1

The 26D `RoofPrototypeBoundary` still stores integer plan endpoints. F1's
clipped skillion side verges begin at the rational seam coordinate, so that
legacy boundary array cannot represent those endpoints exactly.

The prototype keeps the exact visible boundary in the plane polygons and only
emits wholly integer exterior segments in the legacy boundary array. This is an
intentional discovery result, not a production design recommendation.

Before the real roof domain is frozen, derived boundary/topology edges must be
able to reference the same exact rational points as derived planes and
intersections. They must not round a rational seam back to integer millimetres.

## Architectural conclusions

26E3 freezes the following rules:

1. Plan overlap/contact does not determine composition semantics.
2. `ABUTS` and `INTERSECTS` are materially different source relationships.
3. A step/abutment may contain two physical 3D edges with one plan projection.
4. A plane intersection seam is a generic derived edge and need not be called a
   ridge, hip or valley.
5. Single-slope vertical authority must identify its datum-owning boundary.
6. Derived roof boundary topology must support exact rational coordinates.
7. Vertical closure faces at steps/abutments are not sloping roof planes.
8. Different vertical references do not require Storey-derived roof datums.

The prototype remains deliberately fixture-shaped: axis-aligned rectangular
sources, one composition relation, and only the F0/F1 arrangements. It is not a
general polygon boolean engine.

## Verification target

`roof/tests/test_roof_multilevel_composition_prototype.c` proves:

- F0 retains two distinct 3D interface edges at the same plan position;
- source contact without an explicit relationship is rejected;
- choosing `INTERSECTS` for the F0 arrangement does not silently become `ABUTS`;
- F1 clips both surfaces at the exact 26C rational seam;
- F1's seam is classified as a generic plane seam, not a valley;
- changing the authored high-edge elevation regenerates the seam; and
- failed E3 generation preserves the previous output transactionally.

With F0/F1 proven, all six Priority 26 geometry fixture families have now been
exercised by the prototype sequence A/B/E (26D), C (26E1), D (26E2), and F
(26E3). The next discovery task is Priority 26F: define the structural-layout
boundary shared by conventional-rafter and truss strategies without
implementing engineering design yet.
