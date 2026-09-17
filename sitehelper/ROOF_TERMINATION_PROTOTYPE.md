# Priority 26E2 — Roof End Termination Prototype

## Purpose

Priority 26E1 proved that intersecting roof portions can remain independent
source authority joined by explicit composition intent. Priority 26E2 asks a
different question:

> Can fixture D0 be expressed as primitive end-termination intent, without a
> `DUTCH_GABLE` core roof type and without storing the generated hip/cut geometry?

The answer in this prototype is yes.

This remains an isolated geometry experiment. It does not add project ownership,
`DomainId`, commands, persistence, rendering, covering, structural layout or
engineering analysis.

## Source contract added by 26E2

A normal 26D opposing-slope portion remains authoritative. 26E2 adds one
primitive termination instruction:

```text
RoofPrototypeTermination
├── portion
├── end
└── termination_offset_mm
```

For D0:

```text
portion:               the 12000 x 8000 opposing-slope source
end:                   negative ridge-axis end (west)
termination offset:    2000 mm from that end
```

The instruction means that the selected end boundary contributes a lower
end-slope surface only to the explicit termination station. It does **not** name
the finished architectural style.

The following are therefore authoritative:

- the original support extent;
- the opposing-slope generation/slope/direction/reference intent;
- which end receives the end-slope treatment;
- the termination offset from that end.

The following remain derived:

- the west hip plane polygon;
- both hip edges;
- the shortened ridge start;
- the termination/cut segment;
- the elevations of those derived edges;
- the triangular vertical gablet closure implied by the cut base and ridge
  start.

There is no `DUTCH_GABLE` enum and no persisted hip or cut coordinates.

## D0 result

For the frozen 12000 x 8000 support, 22.5 degree canonical slope
(`414214 ppm`) and a 2000 mm west termination offset, the prototype derives:

```text
visible sloping planes: 3
ridges:                  1
hips:                    2
valleys:                 0
termination/cut edges:   1
```

The cut endpoints are:

```text
(2000, 2000, 207107/250)
(2000, 6000, 207107/250)
```

and the shortened main ridge begins at:

```text
(2000, 4000, 207107/125)
```

Those three points are sufficient for a later building-envelope consumer to
construct the vertical triangular gablet closure. That closure is deliberately
**not** represented as a fourth sloping roof plane.

## Why the termination offset is source authority

The D0 fixture deliberately requires the hip-end treatment to stop at
`X = 2000`. With the same rectangular support and pitch, that station cannot be
safely inferred from generic gable or hip geometry.

The prototype therefore treats the 2000 mm station as user/source intent. A
regression test changes the source offset to 1500 mm and requires the cut, hip
endpoints and ridge start to regenerate from it. No derived coordinate is
edited independently.

This is the same source-vs-derived rule already established for valleys in
26E1, applied to explicit termination semantics.

## Derived edge taxonomy

26E2 extends the transient edge classification with:

```text
ROOF_PROTOTYPE_INTERIOR_TERMINATION_CUT
```

This is not persistent domain identity. It labels a seam in one generated
snapshot so envelope/rendering consumers can distinguish an intentional cut
from a ridge, hip or valley.

## Scope deliberately kept narrow

The E2 implementation supports only the exact architectural question required
by D0:

- one rectangular opposing-slope source portion;
- ridge axis parallel to X;
- a termination at the negative ridge-axis end;
- positive integer termination offset;
- equal canonical pitch for the two main slopes and derived end slope.

Unsupported orientations or degenerate offsets return
`ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY`. They are not silently guessed.

This narrow scope is intentional. The purpose of E2 is to prove the semantic
primitive, not to turn the prototype into a generic end-treatment solver.

## Transactionality

As with 26D and 26E1, a successful rebuild replaces the owned transient output.
A failed termination build leaves the caller's existing geometry untouched.

## Architectural result

The successful model is:

```text
opposing-slope source portion
          +
explicit end termination intent
          |
          v
  derived end-slope geometry
          |
          +--> hips
          +--> shortened ridge
          +--> termination cut
          +--> closure boundary available to envelope consumer
```

The rejected model is:

```text
DUTCH_GABLE
├── hip_1
├── hip_2
├── cut_line
└── ridge_start
```

Priority 26E2 therefore confirms that "Dutch gable" can remain a user-facing
preset/classification assembled from lower-level source semantics rather than a
fundamental roof geometry type.

The next gate is Priority 26E3: multi-level composition. F0 must distinguish an
explicit step/abutment from geometric intersection, and F1 must prove a true
intersection between portions with different pitch and vertical reference.
