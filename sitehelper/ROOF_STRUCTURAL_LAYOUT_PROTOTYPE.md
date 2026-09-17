# Priority 26F — Structural-Layout Boundary Prototype

## Status

Complete as a deliberately narrow prototype.

Priority 26F proves one architectural boundary only:

> The same derived roof envelope can feed different structural layout
> strategies without changing roof geometry or treating geometric features as
> structural supports by implication.

It does **not** size roof members, calculate loads, place individual rafters or
trusses, select products, perform AS 1684/AS 1720 design, or establish project
ownership/persistence.

## Why this prototype exists

The standards review showed that roof geometry alone is not enough to determine
structural behaviour. Identical external roof forms can be framed using
conventional rafters, prefabricated trusses, or mixed systems, and the support
scheme changes quantities such as spans, roof load width, supported area and
reactions.

Therefore this dependency is rejected:

```text
DerivedRoofGeometry
        |
        +--> "there is a ridge, therefore the ridge is a bearing line"
```

The required boundary is instead:

```text
RoofEnvelopeDefinition
        |
        v
DerivedRoofGeometry
        |
        +-----------------------------+
        |                             |
        v                             v
Structural strategy A          Structural strategy B
+ authored support intent      + authored support intent
        |                             |
        v                             v
Resolved structural layout     Resolved structural layout
        |                             |
        +-------------+---------------+
                      |
                      v
             later member generation
                      |
                      v
             later structural queries
```

## Frozen 26F proof case

26F intentionally reuses the **A0 simple gable** already proven by 26D:

- support rectangle: 12,000 mm x 8,000 mm;
- ridge axis: X;
- pitch canonicalized to `414214 ppm`;
- eave/support datum: Z = 0;
- two derived roof planes;
- one derived ridge at Y = 4,000 mm.

The exact same `RoofPrototypeGeometry` snapshot is passed to both structural
strategies.

Two structural bearing lines are authored explicitly:

```text
south: (0,0)     -> (12000,0)
north: (0,8000)  -> (12000,8000)
```

The prototype validates those lines against derived eave geometry. It does not
create bearing lines merely because an eave exists.

## Conventional-rafter interpretation

For the conventional strategy the resolved layout contains **two rafter
fields**:

```text
south eave bearing -> south roof plane -> geometric ridge
north eave bearing -> north roof plane -> geometric ridge
```

Each field references one roof plane and the derived ridge where the two fields
meet.

Crucially, whether that ridge is structurally bearing is **authored structural
intent**, represented by:

```text
ROOF_PROTOTYPE_RIDGE_NONBEARING_MEETING
ROOF_PROTOTYPE_RIDGE_REQUIRES_BEARING_SUPPORT
```

Changing this structural role does not modify the envelope ridge or any roof
plane.

The second value means only that the structural layout requires bearing/support
at that location. 26F does not invent the beam, wall, post or load path that
would provide it.

## Prefabricated-truss interpretation

For the truss strategy the same geometry resolves to **one truss run** spanning
between the two authored bearing lines across both roof planes:

```text
south bearing -------------------------------- north bearing
                  /\
                 /  \
           geometric ridge
```

The derived ridge is a shape break in the roof envelope. It is **not** emitted
as a truss support line.

This is the core proof of 26F: one geometric ridge can be a meeting edge used by
one structural strategy while remaining non-supporting envelope geometry for
another.

## What is authoritative in this prototype

The structural prototype receives explicit intent for:

- structural strategy;
- bearing/support lines;
- span direction;
- conventional ridge structural role where applicable.

The roof envelope remains independently authoritative through the existing
26D/26E source intent.

The prototype copies authored bearing lines into its result so its ownership is
clear. In the final project model those supports will probably need semantic
references to actual supporting construction rather than permanently storing
unrelated copied plan lines. 26F deliberately does not choose that identity
model before Priority 26G.

## What is derived

The resolved structural layout derives:

- whether the geometry is supported by this narrow strategy prototype;
- which derived roof plane(s) participate in each structural field;
- which authored bearing line(s) bound each field;
- whether a field meets a derived ridge.

`plane_indices` and `interior_edge_index` are explicitly **snapshot-local**.
They have no `DomainId`, must not be persisted, and become invalid when the
consumed `RoofPrototypeGeometry` is destroyed or rebuilt.

That mirrors the existing SiteHelper rule against persistent pointers/identity
into regenerated framing.

## What 26F deliberately does not infer

The prototype does not infer any of the following from roof geometry:

- a wall is structurally bearing merely because it lies under an eave;
- a ridge is a ridge beam;
- a valley is a valley beam;
- a hip requires a particular support;
- truss type or truss station locations;
- girder-truss locations;
- underpurlins, strutting beams or struts;
- rafter/truss spacing;
- member sizes, grades, products or connections;
- spans, RLW, ULW, supported area, reactions or load paths;
- standards compliance.

Those require structural intent and, later, engineering/design queries.

## Prototype restrictions

26F intentionally supports only the A0-style two-plane axis-aligned gable. A
hip or compound roof returns `ROOF_PROTOTYPE_UNSUPPORTED_GEOMETRY` rather than
pretending the simple resolver is general.

This is important: the purpose is to prove the **layer boundary**, not to write
a framing generator during discovery.

The final structural-layout domain will need to support richer concepts such as
intermediate bearing, ridge beams, underpurlins, girder trusses, valley/saddle
systems, mixed framing and support relationships to walls/beams/posts. Those
belong after roof authority and identity are integrated cleanly.

## Resulting architecture

After 26F the architecture can be stated more precisely as:

```text
Authoritative roof envelope intent
            |
            v
    Derived roof geometry
            |
            +-------------------------------+
            |                               |
            v                               v
Roof structural specification      Roof covering specification
+ support/layout intent
            |
            v
Resolved roof structural layout
            |
            v
Derived physical roof framing       <-- later priority
            |
            v
Structural/load queries             <-- later engineering priority
```

`RoofStructuralLayout` is therefore neither roof-envelope authority nor a list
of physical timbers/trusses. It is the resolved structural topology between
those layers.

## 26F exit decision

The boundary is proven well enough to proceed to ownership/integration work:

1. envelope geometry remains independent of framing strategy;
2. support intent is explicit rather than inferred from geometric edges;
3. conventional and trussed layouts can consume the same geometry snapshot;
4. a geometric ridge has no universal structural meaning;
5. structural-layout references into derived geometry are transient;
6. individual generated members and engineering calculations remain downstream.

The next Priority 26 task is **26G — ownership, IDs, commands and persistence**.
