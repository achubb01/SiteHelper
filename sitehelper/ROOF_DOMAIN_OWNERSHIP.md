# Priority 26G1 — Authoritative Roof Domain and Ownership

Priority 26G1 promotes the roof source concepts proven by 26B–26F into the real
SiteHelper project model. It does **not** yet add roof commands, GUI tools,
persistence records, covering/product data, or structural engineering.

## Ownership

Authoritative containment is:

```text
SiteHelperProject
└── Storey[]
    └── RoofCollection
        └── Roof (global DomainId)
            └── RoofDefinition
                ├── RoofPortionDefinition[] (global DomainId each)
                ├── RoofComposition[]       (roof-owned value relationships)
                └── RoofTermination[]       (roof-owned value relationships)
```

A Storey may own zero, one, or many Roofs. Storey elevation is only the Storey
reference plane; it does not infer a roof datum. Every RoofPortion carries its
own authored `reference_z_mm` under the already-proven source contract.

Only objects that are independently editable/referable receive global identity
in G1: `Roof` and `RoofPortionDefinition`. Composition and termination records
are value relationships keyed by stable portion IDs. Generated planes, ridges,
hips, valleys, seams, step interfaces, and structural-layout results remain
transient and receive no DomainIds.

## Production source authority

`model/roof_types.h` promotes the source concepts that survived all six fixture
families:

- owned integer-mm support vertices;
- generation/form intent (`OPPOSING_SLOPES`, `ALL_BOUNDARY_SLOPES`, or
  `SINGLE_SLOPE`), still treated as primitive generation intent rather than a
  global named-roof taxonomy;
- canonical fixed slope in parts-per-million using `ROOF_SLOPE_SCALE`;
- explicit integer-mm vertical reference;
- explicit plan direction where required;
- explicit low-edge/high-edge ownership for a single-slope vertical datum;
- explicit `INTERSECTS` / `ABUTS` composition keyed by portion IDs; and
- explicit end/station termination keyed by portion ID.

The current rectangle/axis restrictions are deliberately the subset proven by
26D–26E. They are not a claim that future SiteHelper roofs are restricted to
rectangles or axis directions.

## Transactional creation

A newly created Roof enters the project together with one valid source portion.
This consumes two IDs atomically. If allocation, source validation, or geometry
regeneration fails, neither object enters the Storey and the global ID generator
is unchanged.

Adding a connected second portion is also atomic: the new portion and the
explicit relationship that composes it with an existing portion are staged and
validated together. The API deliberately avoids a successful intermediate state
where a multi-portion Roof exists without enough composition intent to
regenerate.

Compositions and terminations added to an existing Roof use candidate-copy,
mutate, regenerate, commit semantics. Successful mutation may invalidate any
borrowed Roof/portion pointers because owned arrays may be replaced.

## Derived geometry boundary

G1 makes the **source domain** production code. The exact derived topology is
not yet promoted because 26E3 proved that the legacy prototype boundary record
cannot represent rational clipped endpoints exactly.

Therefore `roof_build_derived_geometry()` is an explicit transitional bridge:
production `RoofDefinition` is converted into the proven 26D–26E prototype
kernel and produces a transient `RoofPrototypeGeometry` snapshot. This keeps all
six fixtures executable from the real source domain without prematurely
freezing the final derived-topology API.

This dependency should disappear when the exact rational derived geometry is
promoted. It is not a persistence contract.

## Project identity and validation

Roof and portion IDs participate in the same global namespace as Storeys,
Rooms, Walls, Openings, RoomSeparators, and Slabs. Owner lookup resolves either
a Roof ID or one of its portion IDs back to the containing Storey.

Project validation remains allocation-free and checks authoritative roof
collection metadata, source validity/reference integrity, roof/portion IDs and
the global allocator watermark. It does not build a derived snapshot.
Transactional roof mutation separately regenerates the candidate before commit,
matching the existing distinction between project authority validation and
generated framing. Derived snapshots are not stored in the project and are not
project-validation authority.

## Persistence after 26G3

Priority 26G3 advances project persistence to **version 15**. Roof and portion
identity plus authoritative source geometry/relationships are now serialized.
Generated geometry and structural-layout snapshots remain excluded. Versions
1–14 continue to load with zero roofs and no inferred migration. See
`ROOF_PERSISTENCE.md`.

## What G1 intentionally does not do

G1 does not add:

- command-history integration;
- persistence v15;
- roof rendering and dedicated roof editing tools;
- roof covering specifications;
- structural strategy persistence;
- member generation, spacing, sizing, reactions, RLW/ULW, wind or compliance;
- IDs for derived topology; or
- automatic roofs inferred from walls/rooms.

## Exit criteria

G1 is complete when:

1. Storeys exclusively own zero or more authoritative Roofs;
2. Roof and portion identities use the project-global allocator and owner lookup;
3. composition/termination references use stable portion IDs rather than array
   indexes;
4. A–F fixture families regenerate from production source authority;
5. failed source mutation preserves authority and the ID watermark;
6. allocation-free project validation includes roof metadata, identity, source
   validity and reference integrity, while transactional mutation separately
   proves regeneratability;
7. project destruction releases all roof-owned storage;
8. format 14 cannot silently discard Roofs; and
9. no generated geometry or structural-layout snapshot has become project
   authority.

Priorities 26G2 commands/history, 26G3 persistence v15 and 26G4 project/editor
integration are now complete. See `ROOF_SOURCE_EDIT_COMMANDS.md`,
`ROOF_PERSISTENCE.md` and `ROOF_EDITOR_INTEGRATION.md`.
