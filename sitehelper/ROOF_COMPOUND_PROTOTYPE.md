# Priority 26E1 — Compound Roof / Intersection Prototype

## Purpose

Priority 26D proved that simple gable, hip and skillion geometry can be generated
from a small source intent without storing ridges or hips. Priority 26E1 asks the
next architectural question:

> Can compound roof geometry be generated from independent source portions plus
> explicit composition intent, without introducing named compound roof types or
> authoritative valley coordinates?

The acceptance fixtures are C0 and C1 from `ROOF_GEOMETRY_FIXTURES.md`.

This is still a prototype boundary. It does not add project ownership,
`DomainId`, commands, persistence, UI tools, structural framing, covering or
engineering analysis.

## Source contract added by 26E1

The existing `RoofPrototypeIntent` remains the source definition for one roof
portion. 26E1 adds only a relationship layer:

```text
RoofPrototypeCompoundIntent
├── portions[]
└── compositions[]
       ├── first_portion
       ├── second_portion
       └── INTERSECTS
```

The key rule is:

```text
plan overlap != composition
```

Two portions may overlap in plan and remain unrelated. The generator only clips
and joins them when an explicit composition relationship says they intersect.

The fixture's apparent "main gable" and "wing gable" are therefore generator
roles inferred from geometry for this supported case. They are not persisted
style names and there is no `INTERSECTING_GABLE` enum value.

## What remains authoritative

For each source portion:

- support extent;
- generation intent;
- canonical fixed-point slope;
- explicit ridge/fall direction where required;
- integer-mm vertical reference.

For the compound roof:

- which source portions participate;
- their explicit composition relationship.

The following remain derived:

- clipped visible plane polygons;
- ridge visibility/extents;
- valleys;
- valley/ridge junction coordinates;
- junction elevation;
- outer visible boundaries.

Changing the C1 wing pitch therefore moves the valley junction without editing a
single source coordinate.

## Supported geometry in E1

The implementation is intentionally narrow. It accepts exactly two rectangular
`OPPOSING_SLOPES` portions whose ridge axes are perpendicular and whose eave
reference elevations are equal. One portion must pass through the other while
the projecting portion's two eave lines sit strictly inside the through
portion's ridge-axis extent.

That is enough to prove C0/C1 while avoiding an accidental generic roof solver.
Unsupported arrangements return `ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY` rather
than being guessed or silently reinterpreted.

The relation order is not semantic: swapping the two indices in the
`INTERSECTS` edge produces the same generated topology.

## Exact unequal-pitch intersection

Priority 26C froze roof planes as:

```text
S * Z = Gx * X + Gy * Y + K
S = 1,000,000
```

26E1 preserves exact rational millimetres for the C1 junction. With the frozen
canonical slopes:

```text
main: 22.5° -> 414214 ppm
wing: 30°   -> 577350 ppm
```

and a 4000 mm wing width, the junction on the wing ridge is exactly:

```text
X = 6000
Y = 1079506000 / 207107 mm
Z = 11547 / 10 mm
```

No decimal approximation is used to clip the planes or construct the valley
edges. The test also evaluates the junction through both participating affine
planes and requires exactly the same rational Z.

The prototype uses checked, portable `int64_t` arithmetic and reduced rational
values. Overflow is reported; it does not fall back to floating point. This is
sufficient for the frozen fixture scale but does not yet claim arbitrary-size
production geometry.

## C0 result

For equal 22.5° pitches the generator derives the frozen fixture result:

```text
visible planes: 4
ridges:         2
valleys:        2
hips:           0
junction:       (6000, 6000)
```

The main north plane is clipped into the seven-vertex polygon specified by C0,
and the hidden overlap beneath the projecting source is removed from the
visible result.

## C1 result

Changing only the projecting portion slope to 30° leaves both support polygons
unchanged but moves the valley/ridge junction to the exact rational Y above.
The valley starts remain `(4000,8000)` and `(8000,8000)`, proving that valley
geometry is regenerated rather than edited as source authority.

## Derived edge taxonomy

26E1 extends the transient interior edge enum with:

```text
ROOF_PROTOTYPE_INTERIOR_VALLEY
```

This does not give valleys domain identity. It merely classifies edges in one
owned generated snapshot, alongside the existing ridge and hip classifications.

## Transactionality and lifetime

The 26D ownership rule still applies:

- generated arrays are owned by `RoofPrototypeGeometry`;
- no generated element has a `DomainId`;
- all borrows/indices expire on destroy or successful rebuild;
- a failed compound build leaves the caller's existing output unchanged.

## What E1 deliberately does not solve

It does not yet solve:

- arbitrary polygon clipping/boolean operations;
- more than two source portions;
- diagonal/non-axis-aligned source portions;
- unequal source reference elevations;
- Dutch-gable/gablet termination;
- abutment, overlay or cut/termination relationships;
- F0/F1 multi-level composition;
- dormers, parapets or penetrations;
- project ownership, persistence or editing;
- framing/truss layout or structural analysis.

Those omissions are intentional. C0/C1 were chosen to answer the source
composition question before expanding the relation vocabulary.

## Architectural result

26E1 supports this model:

```text
source portion A ─┐
                  ├─ explicit INTERSECTS relation
source portion B ─┘
                  ↓
          compound geometry solve
                  ↓
      clipped visible plane network
                  ↓
       derived ridges + valleys
```

It rejects this model:

```text
INTERSECTING_GABLE
├── valley_start_1
├── valley_start_2
└── valley_junction
```

Priority 26E2 now extends this same compound source boundary with primitive
end-termination semantics for fixture D0. See `ROOF_TERMINATION_PROTOTYPE.md`.
The next unresolved compound gate is Priority 26E3: multi-level abutment versus
true 3D intersection.
