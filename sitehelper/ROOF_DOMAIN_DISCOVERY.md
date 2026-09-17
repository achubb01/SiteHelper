# Priority 26 — Roof Domain Discovery

Status: discovery/design plus isolated numeric and geometry prototypes.
Priorities 26A (standards/regulatory discovery), 26B (geometry acceptance
fixtures), 26C (slope/plane numeric contract) and 26D (minimal roof-intent /
geometry prototype) are complete. This document intentionally introduces no
persisted `Roof` C type, no persistence grammar, no editor tool, no standards
calculator and no framing generator.

The purpose of Priority 26 is to identify the information that must remain
**authoritative** before SiteHelper commits to a roof object model. The existing
wall architecture remains the guiding principle—authoritative intent is kept
separate from deterministic generated construction—but roofs need an additional
geometry boundary because roof shape and roof framing are not the same problem.

## Existing SiteHelper constraints to preserve

The current project establishes several rules that roof work should not undo:

- authoritative physical distances and coordinates are integer millimetres;
- coordinate spaces are explicit rather than hidden inside generic `Position`
  values;
- `SiteHelperProject` owns a global `DomainId` namespace and Storeys own their
  contained physical domains;
- stable IDs, not persistent pointers, cross editor/command/project boundaries;
- generated wall framing is regenerated rather than persisted;
- plan topology and wall junctions are derived snapshots, not duplicated
  authoritative relationships;
- slabs show that complex owned geometry can remain authoritative while
  quantities and other consequences are recomputed;
- commands should eventually stage a complete candidate, validate/generate it,
  and commit atomically.

A roof domain should fit these contracts instead of creating a second spatial
model beside them.

## Construction context that changes the architecture

Residential roof **geometry** and residential roof **framing system** must be
separate concepts. The same exterior roof form may be constructed using
prefabricated nail-plated trusses, conventional rafters and supporting members,
or a mixture around special conditions. Conversely, a truss layout does not by
itself define the architectural roof envelope.

This means the wall shorthand

```text
Wall definition -> deterministic generator -> derived framing
```

should be refined for roofs to the conceptual pipeline

```text
                    project / site context
                 orientation, wind, ground ...
                           |
                           v
Authoritative roof envelope intent
           |
           v
Derived roof geometry / plane network
           |
           +-------------------------+
           |                         |
           v                         v
Roof covering specification   Roof structural specification
                                     |
                                     v
                            Roof structural layout
                                     |
                                     v
                            Derived roof framing
                                     |
                                     v
                         Structural/load queries
```

Analysis layers such as wind, standards applicability, structural design and
load-path analysis consume this model; they do not become fields hidden inside
`RoofDefinition`.

The exact structs and module boundaries are deliberately deferred. The important
contract is that ridges, hips, valleys and plane intersections generated from
higher-level intent should not be copied back into a second competing source of
truth.

## Priority 26A — standards and regulatory discovery

The roof architecture was checked against the principal Australian residential
sources needed to expose domain concepts before struct design:

- NCC 2022 Volume Two / Housing Provisions structural pathways;
- AS 1684.2:2021 (including Amendment 1), non-cyclonic residential timber
  framing;
- AS 1684.3:2021 (including Amendment 1), cyclonic residential timber framing;
- AS 1720.3:2016, reconfirmed 2026, design criteria for timber-framed
  residential buildings;
- AS 1720.5:2015 (including Amendment 1), reconfirmed 2026, nailplated timber
  roof trusses;
- AS 4440:2004, installation of nailplated timber roof trusses;
- AS 4055:2021 (including Amendment 1), wind loads for housing; and
- AS/NZS 1170.2:2021 (including Amendments 1 and 2), wind actions.

This is an architectural review, not an implementation of the Standards. The
Standards remain external normative sources. SiteHelper must not copy their
protected tables or text into source code or this design note; future compliance
modules should encode only the rules/data that the project is licensed and
legally able to implement, with source/edition metadata and tests.

### Standards-derived architectural rules

#### Roof form is not structural-system identity

AS 1684 distinguishes raftered coupled and non-coupled systems from engineered
trussed roofs. The same exterior envelope can therefore have materially
different structural systems. A roof-style enum must not mix geometric forms
(`gable`, `hip`, `skillion`) with structural systems (`coupled`, `non-coupled`,
`trussed`).

The authoritative model must preserve two independent questions:

```text
what exterior roof shape is intended?

what structural system is intended to carry it?
```

#### Structural support/layout is a distinct layer

AS 1684 roof load width and supported-area rules vary with support arrangement,
including trussed, cathedral, skillion, coupled roofs, ridge support and
underpurlins. AS 1684 also defines rafter span from actual points of support along
the rafter rather than from horizontal plan projection.

Therefore plan geometry alone cannot determine all structural consequences. A
future `RoofStructuralLayout`-like layer must be able to represent structural
intent such as bearing/support lines, ridge/intermediate support, framing
orientation and relationships between primary and supported members.

Quantities such as rafter span, roof load width (RLW), uplift load width (ULW),
supported area and reactions are consequences of geometry + structural layout +
analysis rules. They should normally be derived rather than persisted as
editable source values.

#### Trussed roofs require a layout domain, not merely generated timber

AS 1720.5 requires the truss system to be documented as a roof framing plan with
truss types/locations and associated bracing, tie-down and restraint information.
AS 4440 uses system concepts such as standard, jack/creeper, hip, truncated,
girder, Dutch-hip and valley/saddle trusses, plus pitching points and stations.

A truss strategy therefore needs a **layout result** before individual chord/web
member design. It must not be implemented as "fill every roof plane with the
same truss".

A geometric valley is also not the same thing as a conventional valley rafter or
an AS 4440 valley/saddle truss system. These are different layers that happen to
use the same everyday word.

#### Roof covering is independent authoritative specification

Roof covering affects mass, batten/support requirements, pitch applicability and
other validation, but it is not roof geometry and not the framing system itself.
The domain direction is therefore:

```text
DerivedRoofGeometry
        |
        +---- RoofCoveringSpecification
        |
        +---- RoofStructuralSpecification
```

The two specifications may constrain and inform each other through validation;
neither should own the other.

#### Geometry validity is separate from standards applicability

The reviewed standards have different scope limits. For example, conventional
AS 1684 and simplified AS 4055 pathways have roof-pitch limits that are not the
same as the design scope of AS 1720.5, while AS/NZS 1170.2 includes wind methods
for roof slopes outside the simplified-housing range.

SiteHelper therefore must distinguish:

```text
GEOMETRY_VALID

STANDARD_METHOD_APPLICABLE
STANDARD_METHOD_OUTSIDE_SCOPE
STANDARD_METHOD_REQUIRES_ENGINEERED_DESIGN
```

A roof that falls outside a particular Deemed-to-Satisfy or simplified method
must remain representable in the building model. Regulatory scope must not be
encoded as CAD geometry validity.

#### Wind/site data belongs above the roof domain

AS 4055 determines housing wind classification from site conditions. AS/NZS
1170.2 further shows that wind actions are directional: site wind speeds are
considered by cardinal direction and transformed relative to building axes.
Consequently SiteHelper will eventually need a project/site relationship to true
north (or another explicit orientation datum).

Do not make plan +X or +Y silently mean north, and do not place site wind
classification, terrain, topography or shielding directly on `RoofDefinition`.
These belong to a project/site structural-design context consumed by roof, wall,
cladding, bracing and tie-down analyses.

#### Wind-facing roles are analysis-time classifications

A physical roof plane is not permanently "upwind", "downwind" or "crosswind".
Those roles change with the wind case. Likewise local pressure regions near roof
edges, corners, hips and ridges depend on the building/roof geometry and the
analysis direction.

The model should support:

```text
DerivedRoofGeometry + WindCase
                |
                v
Directional plane roles / pressure regions
```

rather than persisting aerodynamic classifications into roof authority.

#### Wind analysis consumes the building envelope, not only the roof

AS/NZS 1170.2 internal-pressure treatment depends on openings, leakage,
building-envelope surfaces and in some cases enclosed volume. A future wind
module therefore needs a building-envelope view spanning walls, roof surfaces
and openings. Roof engineering must not become a self-contained wind solver.

#### Reference height and ground are cross-domain inputs

Wind standards use roof height relative to ground and distinguish upper/lower
roof conditions. This reinforces that a roof's vertical placement cannot be
silently reconstructed from `Storey.elevation_mm`, the next Storey or current
wall `stud_height`.

Future site/ground modelling and roof spatial placement must meet through an
explicit query boundary rather than hidden arithmetic inside the roof generator.

#### Load path is a project-wide structural concern

The framing standards explicitly carry roof loads through supporting walls,
beams, posts and other members, and truss rules distinguish concentrated loads
from girder/support relationships. The eventual load graph must cross domain
boundaries:

```text
roof member / truss
        |
        v
wall / beam / post
        |
        v
lower framing / slab / footing
```

Priority 26 should expose the necessary support relationships but should not
invent a roof-specific load-path engine that later has to be replaced.

### Standards requirements matrix for domain ownership

| Concept | Likely owner / status |
| --- | --- |
| roof envelope source geometry | authoritative roof intent |
| pitch / fall direction / vertical reference | authoritative roof intent |
| eave/barge projection intent | authoritative roof intent |
| roof covering choice/specification | authoritative covering specification |
| structural-system choice | authoritative structural specification |
| support/bearing arrangement where not safely inferable | authoritative structural intent |
| true-north/building orientation | project/site context |
| wind region/terrain/topography/shielding | project/site structural context |
| generated planes/ridges/hips/valleys | derived roof geometry |
| upwind/downwind/crosswind plane role | derived per wind case |
| local roof pressure regions | derived wind analysis |
| rafter/truss span | derived from member/support geometry |
| RLW / ULW / supported/tributary area | derived structural query |
| truss type/location layout | derived or staged structural-layout result |
| individual rafters/trusses/chords/webs | derived framing/design result |
| reactions/load paths | derived structural analysis |
| standards-method applicability | derived validation result |

The single important qualification is support intent: two roofs with identical
exterior geometry can have different valid structural support schemes. Where
support cannot be inferred unambiguously, the user/design process must be able
to author it explicitly.

## Why a single `RoofType` is not authoritative

The discovery examples demonstrate that familiar roof names are useful UI
language but poor core identity:

- a **gable** has two roof planes and gable-end boundary conditions;
- a **hip** has sloping end planes as well as side planes;
- a **valley** is normally an intersection relationship between roof planes, not
  a whole-roof type;
- a **Dutch gable** combines hip geometry with a gable/gablet termination;
- a **skillion** is one sloping plane and needs a fall direction;
- a complex house can contain several of the above at once, potentially with
  different pitches and elevations.

Therefore `ROOF_GABLE`, `ROOF_HIP`, `ROOF_VALLEY`, ... must not become the sole
persisted representation. Such names may later be **authoring presets** that
produce authoritative roof intent.

## Discovery fixtures

These are not proposed persisted records. They are deliberately concrete roofs
against which any future representation must be tested before implementation.
All plan dimensions below are millimetres.

### Fixture A — simple gable

Plan support rectangle: `12000 x 8000`.

Intent:

- two equal-pitch roof planes;
- ridge runs parallel to the 12000 direction;
- long boundaries are eaves;
- short boundaries are gable ends;
- nominal pitch: 22.5 degrees;
- eave/barge projection must be expressible independently of the support
  boundary.

Expected derived facts:

- one ridge;
- no hips or valleys;
- two roof planes;
- ridge height is derived from pitch and horizontal run, not an unrelated second
  height input.

What it proves: footprint + pitch is insufficient unless ridge/slope orientation
or boundary roles are also known. A square footprint is the obvious ambiguous
case.

### Fixture B — equal-pitch hip

Plan support rectangle: `12000 x 8000`.

Intent:

- all four boundaries drain to eaves;
- equal pitch on all sides;
- nominal pitch: 22.5 degrees.

Expected derived facts for the rectangular/equal-pitch case:

- four hip edges;
- a central ridge parallel to the 12000 direction;
- four roof planes;
- ridge endpoints and hip lines are consequences of the plane geometry.

What it proves: hips and ridge endpoints should normally be derived. Persisting
both a footprint/pitch and independently editable hip/ridge lines would create
redundant geometry that can disagree.

### Fixture C — intersecting gables producing valleys

Use a main rectangular gable mass and a perpendicular projecting gable wing,
with overlapping plan regions and equal reference elevation. Begin with equal
pitch, then repeat the case with different pitches.

Intent:

- each source roof portion has its own support extent, slope/ridge intent and
  pitch;
- the portions are intended to intersect rather than merely overlap visually.

Expected derived facts:

- valley edges occur where downward roof planes intersect;
- the equal-pitch valley is not a separate authored object;
- changing one pitch moves the valley while leaving the source plan intent
  unchanged;
- hidden portions of source planes are clipped from the exterior roof result.

What it proves: valleys belong naturally to a derived plane/intersection network.
Complex roofs also require explicit **composition/join intent** between source
roof portions; simple polygon union is not enough for every future case.

### Fixture D — Dutch gable / gablet

Plan support rectangle: `12000 x 8000` with a nominal 22.5-degree main pitch.

Intent:

- the lower roof behaves as a hip form;
- the hip end terminates below the upper ridge;
- a gablet continues above that termination;
- the termination position/height is intentional and cannot be recovered from
  rectangle + pitch alone.

Expected derived facts:

- lower hip planes and hip edges;
- an upper ridge/gable roof portion;
- a transition/cut boundary where the hipped portion terminates;
- a vertical gable closure may be required by the building envelope, but that
  vertical face is not itself a sloping roof plane.

What it proves: compound forms need explicit form/termination intent. A single
whole-roof enum is too coarse, while making every resulting plane authoritative
would be too low-level for normal editing.

### Fixture E — skillion

Plan support rectangle: `8000 x 6000`.

Intent:

- one roof plane;
- explicit downhill direction across the 6000 dimension;
- nominal pitch: 7.5 degrees;
- one vertical reference is supplied at a known support/eave line or point.

Expected derived facts:

- no ridge, hip or valley;
- high and low roof edges occur at different elevations;
- changing fall direction changes the roof even though footprint and pitch are
  unchanged.

What it proves: every future representation needs an unambiguous slope direction
and a vertical datum/reference. Pitch by itself never establishes a plane.

### Fixture F — multiple intersecting planes and levels

Combine a main gable/hip roof with at least one lower attached skillion or
secondary roof portion. Then test a second case where two source portions have
unequal pitch and unequal reference elevations.

Intent:

- multiple roof portions exist within one building/storey context;
- each can carry independent slope and reference-elevation intent;
- the join is explicitly an intersection/abutment/overlay decision rather than
  inferred only from plan overlap.

Expected derived facts:

- the exterior roof can contain ridges, hips, valleys, eaves, gable/barge edges
  and step/abutment boundaries at the same time;
- not every geometrical intersection should necessarily become a structural
  join;
- some source surfaces may be partly or entirely hidden after composition.

What it proves: the roof domain needs more than a single footprint and more than
one global pitch. Composition semantics are an authoritative input.

## Authoritative information discovered

Any future roof representation must be able to express the following information
without storing mutually redundant geometry.

### 1. Scope and vertical reference

A roof needs a stable project/storey context and a documented vertical datum.
`Storey.elevation_mm` is a floor/reference plane; it must **not** silently mean
roof eave level or top-plate level. Roof input needs an explicit Storey-relative
(or otherwise explicitly scoped) vertical reference.

Do not derive roof level merely from the next Storey's elevation. Likewise, the
current wall `stud_height` is not a sufficient universal roof datum: future
sloping walls, parapets, beams and differently heightened supports would break
that assumption.

Whether Roof objects are ultimately Storey-owned remains a design choice, but a
Storey is the strongest current containment candidate. A lower roof and an upper
roof can coexist on different Storeys or as multiple roof portions; ownership
must not be inferred from elevation alone.

### 2. Plan source/support extent

The generator needs plan geometry describing where a roof portion is intended to
exist or be supported. This is not necessarily identical to the finished eave
outline.

A useful distinction is:

```text
support/source boundary + edge projection intent -> derived exterior roof edge
```

because eaves and gable/barge projections may differ. Directly treating wall
segments as the only roof geometry would create brittle cross-domain ownership.
A future convenience operation may seed roof intent from wall/topology geometry,
but the roof must not retain raw Wall pointers.

If long-lived support associations are eventually needed, use stable IDs with
explicit semantics rather than pointer identity or geometric coincidence.

### 3. Boundary behaviour

The future authoring model must distinguish at least the intent represented by:

- draining/eave boundaries;
- gable/barge boundaries;
- high/low edges for a mono-pitch roof;
- intentionally terminated/cut boundaries such as a gablet transition;
- abutment/step boundaries in compound roofs.

These may not become literal enum fields on every edge; the requirement is that
the information is representable and unambiguous.

### 4. Slope intent

A roof plane needs both **magnitude** and **direction/orientation** of slope.
Simple forms can derive some directions from a ridge axis or boundary roles;
skillion and compound roofs make the need explicit.

Do not persist a naked `double pitch_degrees` as the geometry authority merely
because UI plans use degrees. Priority 26C tested normalized rational rise/run,
fixed-point angle and fixed-point tangent/gradient approaches and selected a
**1,000,000-scale fixed rise/run gradient** as the canonical arithmetic contract.
The detailed decision and fixture evidence are in `ROOF_NUMERIC_CONTRACT.md`.

The derived plane form is an integer affine equation (`S*Z = Gx*X + Gy*Y + K`)
and plane intersections remain exact rational millimetres. Degrees/ratios are
input/display representations that canonicalize at the boundary; topology does
not repeatedly call trigonometric functions or use epsilon equality. Final
source structs and arbitrary-direction authoring conversion remain for 26D.

### 5. Roof-form / plane-generation intent

Some information must tell the geometry stage whether a source area should form,
for example, a two-sided gable, all-sided hip, single-plane skillion, or a
compound/truncated form. This does **not** imply a global `RoofType` enum.

The likely abstraction is smaller source portions/regions/masses plus generation
rules. The exact name and struct layout should wait until the fixtures are
implemented as tests/prototypes.

### 6. Composition/join intent

When roof source portions overlap or meet, SiteHelper must know what relationship
is intended. Future operations will at least need to distinguish concepts such
as:

- intersect and expose the upper envelope;
- abut against another roof/wall;
- overlay without geometrically merging;
- intentionally cut/terminate one form with another.

A deterministic plane clip/intersection engine can derive the resulting geometry
only after this intent is known. Plan overlap alone is not a complete semantic
rule.

### 7. Projection / overhang intent

Eave and barge overhangs affect roof envelope, fascia locations, roof area,
cladding quantities and framing tails. They should not be hidden inside wall
geometry. Uniform defaults may eventually exist, but edge-specific differences
must remain representable.

### 8. Framing-system intent — separate from roof geometry

The future framing stage must be able to distinguish at least:

- conventional/cut rafter framing, including coupled and non-coupled structural
  arrangements where applicable;
- prefabricated nailplated trussed framing;
- mixed/special conditions where one strategy does not describe the whole roof.

Member sizes, spacing, truss families, girder/truncated truss choices, strutting
and engineering are **not** roof-envelope geometry. They belong in a framing
specification/strategy layer consumed after roof geometry exists.

The initial roof geometry work must therefore avoid types such as `Rafter` or
`Truss` in `RoofDefinition`.

### 9. Structural support intent

The standards review shows that roof load distribution and member spans depend
on how the roof is supported, not merely on its exterior shape. A future roof
structural specification/layout must therefore be able to express support intent
when it cannot be derived safely from other authoritative construction.

Examples include:

- a ridge acting as a non-structural ridgeboard versus a supported ridge beam;
- intermediate rafter support;
- an underpurlin/strut support arrangement;
- truss bearing lines and changes in truss run;
- girder relationships where one framing family supports another.

Do not solve this with persistent raw pointers to Walls or generated members.
Where cross-domain authority is required, use stable DomainIds plus an explicit
relationship semantic. Derived reactions and load widths should reference the
resolved support graph, not duplicate it as scalar properties.

### 10. Roof covering specification

The roof covering is authoritative product/construction intent but is neither
roof geometry nor structural framing identity. At minimum the future architecture
must allow covering properties relevant to geometry/structure validation to be
queried without `RoofDefinition` becoming a product database.

Covering mass, support/batten requirements and permitted pitch may influence
validation and structural actions. The exact product/material model remains a
later domain decision.

### 11. Project/site structural context

Roof analysis will eventually consume project-level information that is not
owned by a roof: jurisdiction/standards pathway, building orientation, site/ground
reference, wind context and later other environmental/design actions.

Priority 26 does not define that context, but the roof API must be designed so
those inputs can be supplied explicitly. No roof field should silently assume
true north, site wind classification or ground elevation.

## Information that should be derived, not authoritative

For roofs generated from higher-level intent, these should normally be outputs:

- roof-plane polygons after clipping/composition;
- 3D roof-plane vertices;
- ridge lines;
- hip lines;
- valley lines;
- plane adjacency;
- edge classification where it can be determined from the plane network;
- roof surface area and projected area;
- ridge/hip/valley/eave lengths;
- conventional rafters/jacks/hips/valleys;
- truss layout and individual truss members;
- roofing/cladding layouts and quantities;
- structural support graph consequences (spans, tributary/supported areas,
  RLW/ULW and reactions);
- wind-facing plane roles for a particular wind direction;
- local wind pressure regions derived from roof/building geometry;
- standards-method applicability results.

As with `PlanTopology`, a derived roof-geometry snapshot should have an explicit
lifetime and become stale after authoritative roof edits. It should not allocate
DomainIds for incidental intersections or generated edges unless a future user
workflow demonstrates that stable identity is genuinely needed.

## Information explicitly deferred

Priority 26 does **not** decide or implement:

- final production/persisted C structs or enum names;
- roof persistence/version changes;
- roof editor tools or UI presets;
- a general 3D coordinate type;
- automatic roof generation from rooms/walls;
- roof support/load-path engineering;
- project/site wind and true-north modelling;
- building-envelope wind analysis;
- AS 1684 member sizing or compliance calculations;
- AS 4055 / AS/NZS 1170.2 wind calculations;
- nail-plated truss engineering/design;
- fascia, gutter, soffit or barge construction;
- roof cladding/material systems;
- penetrations, skylights, chimneys or roof windows;
- dormers, parapets, curved roofs, butterfly roofs or mansards;
- CAD/BIM roof import semantics.

Those omissions are intentional. Several depend on the geometry boundary being
correct first.

## Candidate architecture to test, not yet adopt

Of the representations considered during discovery:

### Explicit authoritative roof planes

Powerful enough for arbitrary geometry and imported CAD, but poor as the primary
residential authoring model. Shared ridges/valleys become duplicated constraints,
editing one plane can invalidate neighbours, and simple wall-plan edits require
rewriting low-level 3D geometry.

**Verdict:** keep as a possible future advanced/import representation, not the
first normal authoring model.

### One footprint + one roof-type enum + one pitch

Very convenient for a demo but already fails square-orientation ambiguity,
skillion direction, Dutch-gable termination, mixed pitches and intersecting
roofs.

**Verdict:** reject as the authoritative model. It can exist only as UI preset
input.

### Authoritative ridge/hip/valley graph

Can describe complex geometry but promotes consequences of plane intersections
to source data. It also requires users/tools to maintain a globally coherent 3D
network through edits.

**Verdict:** reject as the default authority; a generated edge graph is valuable
as a derived query/render/framing representation.

### Source roof portions/masses + generation/composition intent

A source portion can own plan extent, vertical reference, slope/form intent,
boundary/projection intent and composition semantics. A deterministic geometry
stage then emits clipped roof planes and classified intersections.

This describes all six discovery fixtures without making valleys authoritative
and still leaves room for future imported explicit-plane geometry.

**Verdict:** strongest candidate. Priority 26D now proves the simple A/B/E
fixtures with a minimal source-intent object and a transient exact geometry
snapshot, without source ridge/hip fields. The source struct is still not frozen:
26E must prove compound/intersection semantics before this representation can
become project authority.

## Coordinate-space direction

Priority 1 established that coordinate spaces should be explicit. Roof work must
continue that rule.

Likely conceptual spaces are:

- authoritative plan X/Y in integer millimetres;
- authoritative Storey-relative vertical offsets in integer millimetres where a
  vertical scalar is explicitly supplied;
- derived roof spatial X/Y/Z for plane/intersection/framing calculations;
- screen pixels only at rendering boundaries.

Do not overload `WallLocalPosition` for roof geometry and do not add another
ambiguous generic `Position`. If a shared building-spatial 3D type is eventually
needed, introduce it because roof/slab/wall consumers have demonstrated the same
semantic space, not merely because three numbers are convenient.

## Relationship to current topology

`PlanTopology` is valuable input for authoring helpers (for example, selecting a
closed exterior boundary) but is not sufficient to represent a roof:

- it is 2D and Storey-local;
- it knows wall/separator plan segments, not roof slopes or elevations;
- room faces do not encode eave/gable behaviour;
- wall junctions do not encode roof joins.

Therefore the roof domain should **consume** topology where useful but not extend
`PlanTopology` into a pseudo-3D roof engine. Roof-plane topology deserves its own
derived representation with a similarly strict authority/lifetime contract.

## Relationship to walls and supports

A future framing generator will need support information: walls, beams, posts,
ridge/intermediate beams, underpurlin/strut systems, truss bearing lines, girder
trusses and possibly explicit engineering decisions. The standards review shows
that this is not optional metadata: support arrangement changes member spans,
load distribution and downstream load paths.

The roof envelope generator should still first be able to answer "what is the
intended roof shape?" without requiring every support/member decision.
`DerivedRoofGeometry` therefore remains independent of structural layout.

After geometry exists, a structural-layout stage can resolve authored support
intent against the current project model using stable IDs and geometry queries.
That stage may derive support topology but must never assume every roof-plane
boundary is load-bearing or every internal wall supports the roof.

This separation is particularly important for prefabricated trusses, which may
span across non-load-bearing internal walls and introduce concentrated reactions
at girder/support locations.

## Validation direction

Future validation should be layered rather than one giant `roof_valid()` rule:

1. validate authoritative source geometry and numeric ranges;
2. validate generation intent (slope, orientation, boundary/form rules);
3. validate composition relationships between source portions;
4. derive a complete roof-plane network transactionally;
5. validate the derived network is finite, closed/coherent where required and
   free of unsupported degeneracies;
6. validate structural-system/support intent against the current project;
7. derive structural layout and structural query data;
8. evaluate the selected standards/design method for **applicability**;
9. only when applicable, run method-specific structural/compliance checks.

Geometry validity and standards applicability are deliberately different result
types. A valid 3D roof must not become an invalid project merely because a
particular simplified standard is out of scope.

Project validation should inspect authoritative state. It should not reject a
project merely because a derived snapshot has not been built, just as current
project validation does not persist or require `PlanTopology`/wall junctions.

## Persistence direction

Do not bump persistence during discovery. Format 14 remains correct while no new
authoritative roof data exists.

When roof authority is eventually introduced, persist only the source intent
needed to regenerate roof geometry and structural layout/framing. Never persist
both source intent and a generated plane/ridge/valley/member network as co-equal
truth. RLW/ULW, wind pressure regions, reactions and standards-applicability
results are analysis outputs, not roof authority.

If project/site context later becomes authoritative (for example true-north
orientation or a selected standards/design context), persist it at that owning
layer rather than duplicating it into each roof.

Legacy projects should load with zero roofs unless a later migration has a
well-defined non-heuristic source for creating one. Do not infer roofs from wall
loops during persistence migration.

## Recommended Priority 26 implementation sequence

Priorities 26A-26E3 are complete. The six geometry fixture families have now
challenged the source/geometry prototype across simple, intersecting, terminated
and multi-level composition. Code should now move to the structural-layout
boundary rather than jumping to persistence or standards calculators.

### 26A — standards and regulatory discovery — COMPLETE

Review the relevant residential framing, truss and wind standards only far
enough to identify domain ownership, terminology, support relationships,
analysis boundaries and method applicability. The results are captured above.
No engineering tables/formulas are implemented by this step.

### 26B — freeze roof geometry acceptance fixtures — COMPLETE

The six fixture families are frozen in `ROOF_GEOMETRY_FIXTURES.md`. They define
exact integer-mm source coordinates, explicit slope/orientation and vertical
reference intent, compound composition semantics, expected visible plane/edge
topology, ambiguity checks and non-integer intersection cases without assuming
the final persisted C struct.

The fixtures are now the acceptance gate for the numeric and source-model
prototypes that follow.

### 26C — choose and prove the roof slope/plane numeric contract — COMPLETE

`ROOF_NUMERIC_CONTRACT.md` and `roof/tests/test_roof_numeric_contract.c` freeze
the numeric decision: canonical slope/plane gradients use a fixed rise/run scale
of 1,000,000; derived planes use integer affine coefficients; and fractional
intersection coordinates remain exact rationals. The prototype exercises A0, C1
and F1 without introducing a production roof API.

### 26D — minimal roof-intent / geometry prototype — COMPLETE

`ROOF_GEOMETRY_PROTOTYPE.md` plus `roof/roof_geometry_prototype.[ch]` and its
fixture tests prove a minimal borrowed source intent for A0/A1, B0 and E0/E1.
The generator owns a transient exact plane/boundary/interior-edge snapshot and
derives ridge/hip/high-edge geometry rather than storing it in source authority.
The prototype is intentionally limited to axis-aligned rectangles and remains
isolated from project ownership/persistence until 26E tests compound roofs.

### 26E — compound/intersection prototype — COMPLETE

#### 26E1 — source portions + explicit intersection composition — COMPLETE

`ROOF_COMPOUND_PROTOTYPE.md` plus `roof/roof_compound_prototype.[ch]` and its
fixture tests extend the 26D source contract with an explicit composition edge
between source portions. C0/C1 now derive clipped visible planes, two valleys,
two ridges and the exact unequal-pitch junction without any persisted valley
coordinates or an `INTERSECTING_GABLE` roof type. Plan overlap alone remains
insufficient to trigger composition.

The E1 prototype intentionally supports one perpendicular intersecting-gable
relationship only. It proves the source/composition abstraction, not a general
polygon boolean or arbitrary roof solver.

#### 26E2 — Dutch-gable / termination semantics — COMPLETE

`ROOF_TERMINATION_PROTOTYPE.md` and
`roof/tests/test_roof_termination_prototype.c` extend the compound prototype
with primitive end-termination intent. D0 now derives three visible sloping
planes, two hips, a shortened ridge and an explicit termination/cut seam from
the source termination offset. The triangular gablet closure is available from
derived boundary geometry but is not misclassified as a sloping roof plane.

No `DUTCH_GABLE` core type or authoritative hip/cut coordinates were added.
Changing only the source termination offset regenerates all affected geometry,
and unsupported/failed termination builds preserve the previous output.

#### 26E3 — multi-level / abutment / intersection semantics — COMPLETE

`ROOF_MULTILEVEL_COMPOSITION_PROTOTYPE.md` and
`roof/tests/test_roof_multilevel_composition_prototype.c` complete the frozen
geometry fixtures. F0 adds explicit `ABUTS` semantics and preserves two physical
3D interface edges with the same plan projection but different Z values. F1
uses explicit `INTERSECTS` semantics to clip a gable and unequal-pitch skillion
at an exact rational plane seam.

E3 also exposes two source/derived-contract refinements: a single-slope source
must identify whether its low or high parallel boundary owns the authored Z
datum, and derived boundary topology ultimately needs exact rational endpoints
rather than the prototype's legacy integer-only `RoofPrototypeBoundary`. No
`OVERLAYS` relation was added because no frozen fixture currently requires it.

### 26F — structural-layout boundary prototype — COMPLETE

The A0 gable now feeds two structural strategies through a separate prototype
layer without changing `RoofPrototypeGeometry`. Conventional framing resolves
to two rafter fields meeting at the derived ridge; prefabricated truss framing
resolves to one bearing-to-bearing truss run spanning both envelope planes.

Bearing lines are explicit structural intent and are validated against the roof
geometry rather than inferred from eaves. Likewise, the structural role of a
conventional ridge is authored: a geometric ridge does not automatically become
a ridge beam/support. Layout references to plane/edge indices are snapshot-local
and must be rebuilt with regenerated roof geometry.

The prototype deliberately stops before individual member placement, member
sizing, RLW/ULW, reactions, load paths or standards compliance. See
`ROOF_STRUCTURAL_LAYOUT_PROTOTYPE.md`.

### 26G — ownership, IDs, commands and persistence — IN PROGRESS

26G is split so identity/ownership, command semantics and persistence do not
become one irreversible change.

#### 26G1 — authoritative roof domain + ownership — COMPLETE

`SiteHelperProject -> Storey -> RoofCollection -> Roof -> RoofDefinition` is now
production authority. `Roof` and `RoofPortionDefinition` receive global
`DomainId`s; composition and termination records are roof-owned relationships
keyed by stable portion IDs. Storeys may own multiple roofs. Project owner
lookup, global identity validation, destruction and test support now include
roofs/portions.

All A–F fixture families regenerate from this production source authority. The
still-experimental exact derived topology remains transient behind
`roof_build_derived_geometry()`; generated planes/edges receive no IDs.

Priority 26G3 now promotes roof authority into persistence v15. Versions 1–14
load with zero roofs; v15 serializes only Roof/portion source authority and
relationships, then regenerates derived geometry on load. See
`ROOF_PERSISTENCE.md`.

#### 26G2 — commands and history — COMPLETE

**26G2A — lifecycle commands is complete.** `CREATE_ROOF` and `DELETE_ROOF` now
participate in the existing command/history system. Create undo/redo preserves
the original Roof and initial-portion identities without rewinding the global ID
watermark. Delete history owns an authoritative deep Roof snapshot and restores
the exact Roof/portion IDs and Storey collection order; derived geometry is
regenerated rather than snapshotted. Identity collisions fail closed without
moving the history cursor. See `ROOF_COMMAND_HISTORY.md`.

**26G2B — source-edit commands is complete.** Portion, composition and
termination authority now mutates through candidate-copy -> validate ->
regenerate -> commit transactions with exact pre-edit Roof snapshots, exact-ID
redo for added portions and fail-closed divergence checks.

#### 26G3 — persistence — COMPLETE

Project format v15 serializes Storey-owned Roof authority only: Roof/portion IDs,
support polygons, generation/slope/vertical-reference intent, compositions and
terminations. Generated geometry and structural-layout snapshots are not saved.
Versions 1–14 load with zero roofs. Loads remain transactional and explicitly
regenerate/check roof geometry before replacing the destination project. See
`ROOF_PERSISTENCE.md`.

#### 26G4 — project/editor integration hardening — COMPLETE

Storey-local authoritative roof support polygons now have a deterministic Plan
query boundary, editor selection stores Roof/portion IDs only, and reconciliation
clears stale roof identity after edits/deletes/project replacement. Ordinary Plan
SELECT precedence is intentionally unchanged because Roofs and Slabs normally
overlap. The inexact prototype boundary representation is **not** promoted; the
transitional derived-geometry bridge remains explicit until rational derived
topology is introduced. See `ROOF_EDITOR_INTEGRATION.md`.

### Later engineering priorities — not Priority 26

Standards applicability, wind analysis, member sizing, tie-down/bracing and load
path should become dedicated engineering/analysis priorities after the physical
roof domain and structural-layout contracts exist. They should consume the roof
model rather than define it.

## Exit criteria for Priority 26 discovery

Priorities 26A through 26F are complete. All six frozen roof-geometry fixture
families have been exercised and the structural-layout boundary has been
prototyped. The project may proceed into 26G while preserving these constraints:

1. named roof styles are presets/classifications, not the sole core model;
2. roof envelope geometry, roof covering and structural roof system are separate
   concepts;
3. structural support/layout is distinct from both envelope geometry and
   individual generated members;
4. ridge/hip/valley networks are derived from source roof intent where possible;
5. slope magnitude, slope orientation, vertical reference, plan extent,
   boundary behaviour, projections and source-composition intent are all
   representable authoritative inputs;
6. support/bearing intent can be authored explicitly where it cannot be safely
   inferred;
7. no roof datum is silently inferred from Storey elevation, next Storey, or
   current wall stud height;
8. no raw Wall pointers or persistent derived topology are required;
9. true north, wind/site context and ground reference are project/site concerns,
   not hidden roof properties;
10. directional wind roles and pressure regions are derived analysis state;
11. geometry validity and standards-method applicability are separate results;
12. roof slope/plane arithmetic uses the 26C fixed 1,000,000 rise/run scale,
    integer affine planes and exact rational derived intersections;
13. the six fixtures above gate any proposed struct design;
14. persistence v15 stores only authoritative roof source/relationships and
    regenerates derived geometry on load; versions 1–14 load with zero roofs;
15. the 26D simple generator proves that support extent, generation/slope/
    direction intent and vertical reference can derive A/B/E planes, ridges,
    hips and boundary semantics without persistent derived geometry;
16. the 26E1 compound prototype proves that plan overlap is not composition and
    that explicit `INTERSECTS` source relationships can derive C0/C1 valley
    networks without an `INTERSECTING_GABLE` type;
17. the 26E2 termination prototype proves that an explicit end/station
    termination can derive D0 hips, ridge shortening and a cut seam without a
    `DUTCH_GABLE` type, while leaving the vertical closure to the envelope layer;
18. the 26E3 multi-level prototype proves that explicit `ABUTS` preserves two
    3D edges at one plan location while `INTERSECTS` derives an exact rational
    clipping seam; plan contact/overlap never chooses between those semantics;
19. a single-slope source must identify which parallel boundary owns its
    vertical datum; an unlabeled elevation is insufficient for F0/F1;
20. derived boundary/topology coordinates must eventually share the exact
    rational representation used by plane polygons and intersections; the
    prototype's integer-only `RoofPrototypeBoundary` is not suitable as a final
    contract;
21. `RoofPrototypeIntent` is not yet a persistence contract: its rectangular,
    axis-direction and one-relation restrictions are prototype constraints, not
    intended product limits;
22. the 26F structural-layout prototype proves that the same A0 envelope can
    resolve to two conventional rafter fields or one bearing-to-bearing truss
    run without modifying roof geometry;
23. geometric edges are not structural supports by implication: bearing lines
    and the structural role of a conventional ridge are explicit structural
    intent;
24. structural-layout references into derived roof planes/edges are transient
    snapshot-local references and are rebuilt with regenerated geometry;
25. full framing/engineering implementation starts only after roof geometry,
    structural-layout boundaries and authoritative ownership are cleanly
    separated.

Priority **26G1 — authoritative roof domain + ownership**, **26G2 — roof
commands/history**, **26G3 — roof persistence**, and **26G4 — project/editor
integration hardening** are complete. **Priority 26G is complete.**

## Standards and construction references consulted

The Priority 26A conclusions above were derived from the following source set:

- NCC 2022 Volume Two / Housing Provisions — structural/framing compliance
  pathways and roof-cladding separation;
- AS 1684.2:2021 including Amendment 1 — non-cyclonic residential timber
  framing, especially Clauses 1.3.6, 1.4, 2.6.4-2.6.5 and Section 7;
- AS 1684.3:2021 including Amendment 1 — cyclonic counterpart used to verify
  that the same core roof/support distinctions survive a different wind region;
- AS 1720.3:2016, reconfirmed 2026 — design criteria and structural models for
  conventional residential roof members;
- AS 1720.5:2015 including Amendment 1, reconfirmed 2026 — nailplated truss
  design, documentation, structural models and affected-area concepts;
- AS 4440:2004 — truss-system installation/layout terminology, support and
  girder/hip/valley relationships;
- AS 4055:2021 including Amendment 1 — housing wind classification and roof/wall
  pressure-zone context; and
- AS/NZS 1170.2:2021 including Amendments 1 and 2 — directional wind action,
  building orientation/reference height, tributary area, roof shape factors and
  local pressure regions.

WoodSolutions and the National Dictionary of Building & Plumbing Terms were
used earlier for general construction terminology only; the Australian Standards
and NCC sources above supersede them where normative concepts are involved.

This document deliberately records architectural consequences rather than
reproducing protected Standard text, tables or design equations. Any future
engineering/compliance implementation must identify the exact applicable
jurisdiction, NCC edition, Standard edition/amendments and licensed source data
at the time that feature is built.
