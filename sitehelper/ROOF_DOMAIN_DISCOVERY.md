# Priority 26 — Roof Domain Discovery

Status: discovery only. This document intentionally introduces no `Roof` C type,
no persistence grammar, no editor tool and no framing generator.

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
Authoritative roof intent
          |
          v
Derived roof geometry / plane network
          |
          +----------------------+
          |                      |
          v                      v
roofing/surface consumers   framing-system input
                                 |
                                 v
                         derived roof framing
```

The exact structs and module boundaries are deliberately deferred. The important
contract is that ridges, hips, valleys and plane intersections generated from
higher-level intent should not be copied back into a second competing source of
truth.

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

Do not persist a naked `double pitch_degrees` as the first implementation just
because UI plans use degrees. SiteHelper currently gives authoritative values
stable, explicit representations. Before implementation, evaluate:

- a normalized rational rise/run representation;
- a fixed-point angle representation with a defined conversion contract;
- import/display round-trip requirements for common degree pitches such as
  22.5 degrees.

The chosen representation must support deterministic generation and checked
numeric behaviour on all supported compilers. This is an unresolved Priority 26
result, not permission to guess in the first roof struct.

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

- prefabricated trussed framing;
- conventional/cut rafter framing;
- mixed/special conditions where one strategy does not describe the whole roof.

Member sizes, spacing, truss families, girder/truncated truss choices, strutting
and engineering are **not** roof-envelope geometry. They belong in a framing
specification/strategy layer consumed after roof geometry exists.

The initial roof geometry work must therefore avoid types such as `Rafter` or
`Truss` in `RoofDefinition`.

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
- roofing/cladding layouts and quantities.

As with `PlanTopology`, a derived roof-geometry snapshot should have an explicit
lifetime and become stale after authoritative roof edits. It should not allocate
DomainIds for incidental intersections or generated edges unless a future user
workflow demonstrates that stable identity is genuinely needed.

## Information explicitly deferred

Priority 26 does **not** decide or implement:

- exact C structs or enum names;
- roof persistence/version changes;
- roof editor tools or UI presets;
- a general 3D coordinate type;
- the final pitch numeric representation;
- automatic roof generation from rooms/walls;
- roof support/load-path engineering;
- AS 1684 member sizing or compliance calculations;
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

**Verdict:** strongest candidate. Do not freeze its struct shape until a geometry
prototype proves the necessary fields.

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

A future framing generator will eventually need support information: walls,
beams, hangers/bearers, girder trusses, load-bearing lines and possibly explicit
engineering decisions. That is a later structural-support problem.

The roof envelope generator should first be able to answer "what is the intended
roof shape?" without requiring every support/member decision. Conversely, a
framing generator must not assume that every boundary of a roof plane is a
load-bearing wall.

This separation is particularly important for prefabricated trusses, which can
span across non-load-bearing internal walls.

## Validation direction

Future validation should be layered rather than one giant `roof_valid()` rule:

1. validate authoritative source geometry and numeric ranges;
2. validate generation intent (slope, orientation, boundary/form rules);
3. validate composition relationships between source portions;
4. derive a complete roof-plane network transactionally;
5. validate the derived network is finite, closed/coherent where required and
   free of unsupported degeneracies;
6. only then run framing-system-specific validation/generation.

Project validation should inspect authoritative state. It should not reject a
project merely because a derived snapshot has not been built, just as current
project validation does not persist or require `PlanTopology`/wall junctions.

## Persistence direction

Do not bump persistence during discovery. Format 14 remains correct while no new
authoritative roof data exists.

When roof authority is eventually introduced, persist only the source intent
needed to regenerate roof geometry and framing. Never persist both source intent
and a generated plane/ridge/valley/member network as co-equal truth.

Legacy projects should load with zero roofs unless a later migration has a
well-defined non-heuristic source for creating one. Do not infer roofs from wall
loops during persistence migration.

## Recommended Priority 26 implementation sequence

Discovery should turn into code in narrow steps rather than jumping directly to
rafters:

### 26A — freeze discovery fixtures as geometry acceptance cases

Write test-level fixture descriptions for the six roofs above, including plan
coordinates, pitch/orientation intent, reference levels and expected topological
features. No domain structs yet if the representation remains unsettled.

### 26B — choose and prove the roof slope/plane numeric contract

Prototype only the mathematics needed to represent one sloping plane and
intersect two planes deterministically. Decide pitch authority (rational versus
fixed-point angle) based on common input round trips and cross-platform tests.

### 26C — minimal roof-intent prototype

Implement the smallest source representation capable of gable, hip and skillion
without storing derived ridges/hips. Generate an owned transient roof-geometry
snapshot.

### 26D — compound/intersection prototype

Add the intersecting-gable valley case and then Dutch-gable termination. If the
26C representation needs special-case fields for each named roof style, stop and
revise the abstraction before persistence or editor integration.

### 26E — ownership, IDs, commands and persistence

Only after the six fixtures fit naturally should roof authority enter
`SiteHelperProject`/Storey, global identity, command history and a new persistence
version.

### 26F — framing strategy boundary

Define the input/output contract between derived roof geometry and framing
strategies. Prove that both a conventional-rafter strategy and a truss-layout
strategy can consume the same roof geometry before implementing either deeply.

## Exit criteria for Priority 26 discovery

Priority 26 discovery is complete when the project agrees on these constraints:

1. named roof styles are presets/classifications, not the sole core model;
2. roof envelope geometry is separate from roof framing system;
3. ridge/hip/valley networks are derived from source roof intent where possible;
4. slope magnitude, slope orientation, vertical reference, plan extent,
   boundary behaviour, projections and source-composition intent are all
   representable authoritative inputs;
5. no roof datum is silently inferred from Storey elevation, next Storey, or
   current wall stud height;
6. no raw Wall pointers or persistent derived topology are required;
7. pitch numeric representation remains deliberately unresolved until a small
   deterministic geometry prototype tests it;
8. the six fixtures above gate any proposed struct design;
9. persistence remains v14 until an authoritative representation is proven;
10. framing implementation starts only after roof geometry can represent the
    compound fixtures cleanly.

The next coding task should therefore be **26A/26B, not a roof framing
generator**.

## External construction references consulted

- National Construction Code, Volume Two / Housing Provisions framing material:
  roof framing terminology and separate pathways for timber framing and
  nail-plated timber roof trusses.
- WoodSolutions, *Lightweight Timber Framing Guide*: Australian residential
  context for prefabricated roof trusses and conventional roof framing.
- National Dictionary of Building & Plumbing Terms: Australian definitions for
  skillion and Dutch-gable roof forms.

These references inform terminology and construction context only. SiteHelper's
future engineering/compliance rules must be implemented against the applicable
current standards and project jurisdiction rather than copied from this discovery
note.
