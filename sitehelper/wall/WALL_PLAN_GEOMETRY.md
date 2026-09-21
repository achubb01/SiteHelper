# Wall Plan Geometry

Priority 34 keeps `WallDefinition.segment` as the authoritative placement datum.
A wall's physical Plan representation is instead derived from:

```
WallDefinition.segment
+ WallDefinition.plan_specification
    -> WallPlanGeometry
```

`WallPlanSpecification` currently stores nominal thickness in integer millimetres
and an explicit datum alignment (`CENTER`, `LEFT_FACE`, or `RIGHT_FACE`). New and
legacy walls default to 90 mm centre alignment. Changing thickness/alignment does
not move the segment.

`WallPlanGeometry` is transient. It is never persisted and has no identity. Its
face/corner coordinates are `PlanPoint` values because a perpendicular offset
from an arbitrary integer-coordinate diagonal segment is generally irrational.
The authoritative datum and specification remain integer construction inputs.

## Consumers

Use the **datum** for source editing, endpoint snapping, topology/junction
identity, room-boundary source relationships, wall-local transforms and any
operation whose semantic target is the wall path.

Use the **physical geometry** for normal Plan presentation and visible-body hit
testing. Future room finished dimensions, slab synthesis, roof support synthesis
and dimensions may choose datum or physical faces explicitly according to their
semantic meaning; existence of the physical geometry is not permission to
change those consumers implicitly.

Generated framing remains wall-local U/Z geometry. It is presented within the
physical-wall context but does not define Plan thickness.

## Junctions

The topology/junction layer remains datum-based source authority. Priority 34
does not introduce persistent corner objects or trimmed wall-face identities.
Each wall derives a closed physical body; connected bodies intentionally overlap
at L, T, cross and collinear joins. With ordinary opaque body rendering this is a
set-union presentation, so connected walls have neither gaps nor renderer-only
endpoint extensions. Selection in an overlap resolves deterministically to the
later stored wall, matching other Plan overlap policies.

Exact face trimming, finish-layer mitres and construction-specific corner build-
ups are deliberately deferred. If those become necessary, they should be a
second derived junction-resolution stage consuming current wall bodies plus the
existing datum topology; they must not replace or mutate the authoritative
segments.

## Persistence

Format v23 persists only the new authoritative `plan_spec` (`thickness_mm` and
alignment). v1-v22 walls load with the explicit current default of 90 mm centre
alignment. Derived corners/faces are reconstructed and never serialized.
