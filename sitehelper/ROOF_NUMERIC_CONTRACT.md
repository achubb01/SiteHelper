# Priority 26C — Roof Slope / Plane Numeric Contract

Status: **accepted prototype contract**. This document chooses the arithmetic
representation that later roof-domain work should build on. It does not create a
persisted `RoofDefinition`, public roof API, editor tool or persistence format.

The executable prototype is `roof/tests/test_roof_numeric_contract.c`. It is a
standalone test target deliberately kept out of production libraries so 26C can
prove the arithmetic decision without freezing the later domain object model.

## Decision

Use a **fixed-point tangent/gradient representation with a scale of 1,000,000**
for canonical roof slope geometry.

One slope unit means:

```text
1 unit = 0.000001 mm vertical rise per 1 mm horizontal run
       = 1 part per million of rise/run
```

For example:

```text
22.5 degrees -> 414214 / 1000000
30.0 degrees -> 577350 / 1000000
10.0 degrees -> 176327 / 1000000
 7.5 degrees -> 131652 / 1000000
45.0 degrees -> 1000000 / 1000000
```

The core geometry contract is therefore **not** a persisted `double` angle.
Degrees are a user/import representation that may be canonicalized to this
fixed-point slope. A future source model may still expose pitch as a construction
concept, but geometry must operate on the canonical fixed-point form rather than
repeatedly evaluating trigonometric functions.

For a derived plane, use fixed-point gradient components in plan X/Y. The
prototype expresses a plane as:

```text
S * Z = Gx * X + Gy * Y + K

where
S  = 1,000,000
Gx = vertical gradient along +X, scaled by S
Gy = vertical gradient along +Y, scaled by S
K  = integer scaled offset
X/Y/Z are millimetres
```

For an integer-mm reference point `(X0,Y0,Z0)`:

```text
K = S*Z0 - Gx*X0 - Gy*Y0
```

This is an affine plane with integer coefficients. Once a source slope/direction
has been canonicalized to `Gx/Gy`, evaluating the plane or intersecting two
planes no longer needs `tan()`, `atan()`, floating-point equality or an epsilon.

## Why fixed-point gradient wins

Three candidate families were considered.

### Variable rational rise/run

A normalized rational looks attractive because integer ratios such as `1:3` can
be represented exactly. It does not, however, make common degree pitches exact.
For example, `tan(22.5 degrees)` and `tan(30 degrees)` are irrational. A rational
representation therefore still needs an approximation/canonicalization policy
for the pitches SiteHelper is most likely to receive from plans.

Variable denominators also propagate through plane intersections and make
numeric-growth bounds harder to reason about. Exact rational coordinates remain
useful for **derived intersection results**, but a variable rational is not the
preferred source slope authority.

### Fixed-point angle

A fixed-point degree value would preserve entries such as `22.5 degrees`
exactly, but the roof geometry stage would then need trigonometric evaluation to
turn that angle into a plane gradient. If every rebuild evaluates `tan()` and
then uses floating point for intersections, topology decisions become dependent
on library/platform rounding and epsilon policy.

That is the wrong side of the boundary for nondeterminism. The model should
canonicalize once, then use linear exact arithmetic for generation.

### Fixed-point tangent/gradient

A fixed denominator gives SiteHelper one canonical slope value regardless of
whether the user entered degrees, a ratio or an imported pitch. It also turns a
roof plane into an integer-coefficient linear equation, which makes derived
intersections rational rather than floating-point approximations.

The small input quantization error is bounded, explicit and far below
construction-model millimetre resolution. That trade is preferable to carrying
transcendental floating-point values into every topology predicate.

## Precision bound

Rounding a mathematical slope to the nearest part per million introduces at
most:

```text
0.5 / 1,000,000 rise/run
```

of gradient error. Therefore the worst possible vertical discrepancy caused
solely by slope quantization is:

```text
16,000 mm horizontal run -> <= 0.008 mm
100,000 mm horizontal run -> <= 0.050 mm
```

Those are error **bounds**, not tolerances used by the topology algorithm.
Derived topology still uses the exact canonical fixed-point numbers; it does not
compare points using an 0.008 mm epsilon.

For common construction pitches exercised by the prototype, converting the
canonical slope back to degrees differs from the source angle by less than
`0.00005 degrees`.

## Fixture proof

The 26B fixtures intentionally include non-integer intersections. Using the
chosen canonical slopes gives exact rational results.

### A0 — simple gable

For `22.5 degrees`:

```text
s22.5 = 414214 / 1000000
```

The two opposing planes intersect exactly at:

```text
Y = 4000 mm
```

The canonical ridge elevation is:

```text
Z = 4000 * 414214 / 1000000
  = 207107 / 125 mm
  = 1656.856 mm
```

The ideal transcendental value using an unquantized 22.5 degree tangent differs
by less than `0.002 mm` at that ridge.

### C1 — unequal-pitch intersecting gables

Canonical slopes:

```text
main = 414214 / 1000000   (22.5 degrees)
wing = 577350 / 1000000   (30 degrees)
```

At the wing ridge `X=6000`, the derived valley junction is exactly:

```text
Y = 1079506000 / 207107 mm
  ~= 5212.310544790856 mm
```

The corresponding result from the ideal unquantized tangents is approximately
`5212.306299765296 mm`, so the canonicalization shifts this deliberately
non-integer intersection by about `0.00425 mm`.

No integer-mm rounding is performed. The exact rational result is retained as
derived geometry.

### F1 — unequal-pitch/elevation overlap

Canonical slopes:

```text
main    = 414214 / 1000000   (22.5 degrees)
overlap = 176327 / 1000000   (10 degrees)
```

The seam is exactly:

```text
Y = 1829423000 / 237887 mm
  ~= 7690.302538600260 mm
```

The ideal unquantized result is approximately `7690.301912886375 mm`, a
difference of about `0.00063 mm`.

The canonical Z elevation evaluated from either plane is the same exact
rational:

```text
Z = 15258194011 / 118943500 mm
  ~= 128.281024276232 mm
```

This proves that a derived seam can be shared by both surfaces without choosing
one plane as the floating-point oracle.

## Derived coordinate contract

Authoritative source coordinates remain integer millimetres, as required by
`model/MEASUREMENTS.md`.

Derived roof intersections are allowed to be fractional and should use exact
reduced rational coordinates or another mathematically equivalent checked exact
representation. In particular:

```text
authoritative source X/Y/Z        -> integer mm
canonical slope/plane gradients   -> fixed rise/run at 1e-6 scale
derived intersection X/Y/Z        -> exact rational mm
render/editor preview conversion  -> double only at the presentation boundary
```

A roof generator must not round C1/F1 intersection coordinates back to integer
millimetres merely because project authority uses integer-mm source geometry.

The existing plan-topology subsystem already demonstrates that SiteHelper can
carry exact rational derived coordinates with a portable wide-integer path. 26C
does **not** couple roof geometry to `PlanTopologyRational` or move that numeric
implementation into a generic module. If roof implementation later repeats the
same need, a generic exact-geometry utility may be earned then; this priority
only establishes the contract.

## Checked arithmetic and overflow

The production implementation must use checked arithmetic for:

- forming `K` from reference points and gradients;
- subtracting plane coefficients;
- solving plane/edge intersections;
- constructing/reducing rational coordinates; and
- evaluating Z at rational X/Y.

The 26C executable uses signed 64-bit arithmetic because its purpose is to prove
the frozen fixtures and candidate precision, whose intermediate values are well
inside those bounds. It is **not** permission for the eventual roof library to
assume all valid SiteHelper coordinates and gradients fit every 64-bit
intermediate.

The production implementation should use the project's portable checked
wide-integer strategy (native wide integer where available and a portable MSVC
path) or an equivalent checked exact implementation. Numeric overflow must be a
reported geometry failure, never wraparound or silent floating fallback.

## Degree and ratio conversion boundary

`22.5 degrees` is not itself the core stored arithmetic value selected by 26C.
A UI/import/parser may accept degrees and canonicalize them to the fixed slope.
Likewise, an exact-looking ratio such as `1:3` canonicalizes to the nearest
millionth of rise/run.

The important boundary rule is:

```text
text / CAD / degree / ratio input
             |
             v
checked canonical slope conversion
             |
             v
fixed-point slope/gradient authority
             |
             v
integer/rational roof geometry
```

Trigonometric conversion belongs on the **input/display side** of that boundary.
It must not be repeated inside plane intersection, clipping or topology code.

If exact preservation of the user's original textual spelling is ever desired,
that is presentation metadata and must not become a second geometric authority.

## Direction/orientation scope

26C chooses the plane arithmetic, not the final authoring representation for
roof direction.

The derived plane contract supports arbitrary signed `Gx/Gy` components. The
26B fixtures used here are axis-aligned, so their components are obtained
without any direction normalization. Priority 26D must decide how higher-level
source intent (ridge axis, fall direction, boundary roles, etc.) is converted to
canonical gradient components for arbitrary orientations.

That conversion must follow the same rule as pitch conversion: canonicalize
once to the fixed integer gradient, then keep trigonometric/normalization noise
out of topology operations. 26D must not silently fall back to a persistent
`double` plane because a roof is rotated in plan.

## Determinism rules for later roof geometry

The following are now part of the Priority 26 contract:

1. Plane equality/intersection decisions use canonical integer coefficients and
   exact derived rational arithmetic, not floating-point epsilons.
2. Swapping the order of two planes may negate an intersection-line equation but
   must not change the represented geometry.
3. Source insertion/array order must not influence intersection coordinates.
4. Exact rational derived points remain fractional until a consumer explicitly
   requests a display/render conversion.
5. There is no implicit snapping of derived roof topology to authoritative
   integer millimetres.
6. Geometry calculations after slope canonicalization do not call `tan()` or
   `atan()`.
7. Numeric overflow is an explicit failure result.

## Prototype test coverage

`roof/tests/test_roof_numeric_contract.c` checks:

- canonical part-per-million values for common degree pitches;
- degree round-trip and the 16 m quantization bound;
- A0's exact ridge position and canonical ridge height;
- C1's exact rational valley junction and equal Z on both source planes;
- F1's exact rational seam and equal Z on both source planes;
- non-integer intersections remain non-integer; and
- reversing plane order does not alter the geometric intersection.

The test contains its arithmetic helpers locally on purpose. 26D may replace
those helpers with the first real roof-geometry module after its source model is
chosen.

## 26C completion decision

Priority 26C is complete when this document and prototype test pass together.
The chosen contract is:

```text
source measurements: integer millimetres
canonical slope scale: 1,000,000 rise/run
canonical plane: integer affine coefficients
intersection coordinates: exact rationals
floating point: input/display/render boundary only
```

The next step is **Priority 26D — minimal roof-intent / geometry prototype** for
gable, hip and skillion. 26D should consume this numeric contract rather than
reopening pitch representation unless a fixture demonstrates a concrete failure.
