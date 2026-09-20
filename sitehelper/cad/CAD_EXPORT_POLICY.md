# Priority 30F — CAD export policy and representation

Priority 30F proves the reverse side of the CAD boundary without turning DXF
into SiteHelper persistence:

```text
SiteHelperProject / one Storey
          |
          v
explicit Plan export policy
          |
          v
CadExportDocument
(normalized integer-mm paths)
          |
          v
ASCII DXF writer
          |
          v
AC1032 model-space DXF
```

The export path is intentionally **asymmetric** with import. Import must preserve
external source decimals/provenance until mapping policy resolves them. Export
starts from already-authoritative SiteHelper integer-millimetre Plan geometry, so
its intermediate representation can remain smaller and normalized.

## 1. First supported export workflow

The first workflow exports exactly one existing Storey as simple 2D plan
linework. `CadPlanExportConfig` explicitly selects these representations:

- **wall centre-lines** -> `SITEHELPER_WALL_CENTERLINES`;
- **room separators** -> `SITEHELPER_ROOM_SEPARATORS`; and
- **slab outer outlines** -> `SITEHELPER_SLAB_OUTLINES`.

These are distinct export meanings. A Wall is not exported as faces, framing or
openings merely because those can also be drawn as lines. A Slab is not yet
exported with penetrations, replacement regions or edge rebates. Roofs and
project document annotations are also outside the first contract.

A zeroed config intentionally exports an empty drawing rather than silently
choosing categories for the caller.

## 2. Storey scope is explicit

`cad_plan_export_storey()` requires a stable Storey `DomainId`. It does not:

- infer a Storey from array position or elevation;
- combine multiple Storeys into overlapping model-space geometry;
- turn CAD layers into Storeys; or
- export the whole Project implicitly.

A future multi-level deliverable may choose separate files, explicit transforms,
layout/paper-space presentation or another convention. 30F does not guess that
policy.

## 3. CAD background references are never re-exported implicitly

Priority 30E references are ancillary source/background data. They are **not**
authoritative construction geometry and are not traversed by 30F.

This prevents an import/export cycle from accidentally duplicating arbitrary
source drawing content:

```text
imported DXF reference  --X-->  authoritative export by default
```

A future explicit "include attached reference" or xref-preservation workflow
would be a different export policy with clear provenance/resource semantics.

## 4. Output-side intermediate representation

`cad_export_ir.[ch]` introduces `CadExportDocument` as the smallest proven
output-side IR. It contains only:

- ordered 2D integer-millimetre vertices;
- open/closed path state; and
- a deep-owned logical layer name.

It deliberately contains no:

- `SiteHelperProject`, Storey, Wall, Slab or Room types;
- `DomainId`;
- DXF group codes, handles or version fields;
- import provenance;
- third-party CAD library objects; or
- floating-point coordinates.

The IR therefore sits between construction meaning and format encoding instead
of becoming another building model or a DXF object graph.

Coordinates use signed 64-bit millimetres in the IR even though the current Plan
model enters through `int` `PlanPosition`. This keeps the format boundary from
being tied unnecessarily to the host C `int` width and leaves room for future
explicit export transforms while preserving exact integer authority.

Mutation is deep-copy and transactional. Invalid paths or allocation failure do
not partially append an output path.

## 5. Exact first mapping policy

The 30F mapper performs no geometric transform:

```text
export_mm.x = PlanPosition.x
export_mm.y = PlanPosition.y
```

There is no translation, rotation, mirroring, scale, unit conversion or rounding.
The Project already owns canonical millimetres, so adding a float transform here
would weaken rather than improve the current contract.

The mapper validates the authoritative Project before traversal. It builds a
complete candidate `CadExportDocument` and replaces the caller's previous output
only after all requested paths have been copied successfully.

## 6. ASCII DXF representation

`dxf_ascii_export.[ch]` is a format adapter over `CadExportDocument`; it knows
nothing about Project or construction domains.

The first writer emits:

- ASCII DXF;
- `$ACADVER = AC1032`;
- `$INSUNITS = 4` (millimetres);
- unchanged SiteHelper Plan origin;
- model-space entities only;
- a LAYER table containing layer `0` plus used logical export layers;
- open two-vertex paths as `LINE`; and
- other open/closed paths as `LWPOLYLINE`.

No `DomainId`, SiteHelper object identifier or imported DXF handle is written as
an external entity handle. 30F makes no round-trip identity claim.

The writer uses integer text coordinates. It does not convert through `double`.
It is also transactional: allocation/validation failure leaves an existing output
buffer unchanged.

## 7. Why LINE vs LWPOLYLINE

The representation follows the published DXF meanings used by the 30C reader:
`LINE` carries start/end coordinates, while `LWPOLYLINE` carries a vertex count,
closed flag and repeated 2D vertices. The writer includes the post-R12 subclass
markers for its AC1032 entities.

The dedicated round-trip test proves that a 30F drawing can pass back through:

```text
30F writer
   -> 30C ASCII DXF decoder
   -> 30D exact Plan mapper
```

with the tested Plan geometry unchanged. That is an interoperability test, not a
claim that arbitrary DXF round trips preserve every SiteHelper semantic.

## 8. Not persistence

The export writer is not called by SiteHelper project persistence and 30F makes no
persistence-format change.

A SiteHelper save remains:

```text
SiteHelper authority -> SiteHelper persistence
```

A DXF export remains:

```text
SiteHelper authority -> chosen drawing representation -> DXF
```

The fact that SiteHelper can read its own exported linework later does not make
DXF an authoritative save format. The returned paths have no SiteHelper IDs,
construction specifications, opening definitions, slab thicknesses, roof intent,
history or other model semantics.

## 9. Failure semantics

`cad_plan_export_storey()` rejects:

- invalid arguments;
- a missing Storey;
- an invalid authoritative Project; and
- allocation/numeric/internal failures.

Every failure preserves the caller's previous export document.

`dxf_ascii_export_memory()` rejects malformed output IR and protects against
output-size/count overflow. Every failure preserves the caller's previous byte
buffer.

## 10. What 30F intentionally does not export

The first export contract includes no:

- imported CAD reference/background paths;
- wall faces, studs, noggins, plates or other generated framing;
- wall openings or door/window symbols;
- slab penetrations, regions or rebates;
- roof source/derived geometry;
- room fill/region geometry or room labels;
- dimensions, notes, symbols, callouts or revisions;
- paper-space layouts/viewports;
- blocks/inserts;
- colours/lineweights/linetypes beyond a simple CONTINUOUS layer definition;
- custom target units;
- arbitrary coordinate transforms;
- external entity handles mapped from `DomainId`; or
- DWG output.

Those should be added only as explicit representations with tests showing which
SiteHelper authority they communicate.

## 11. 30F acceptance cases

Tests cover:

- deep-owned, format-neutral export paths;
- invalid path/layer rejection;
- one-Storey scope;
- independent category selection;
- exact wall-centreline, room-separator and slab-outline geometry;
- attached CAD references not appearing in authoritative export;
- explicit stable layer mapping;
- AC1032 / millimetre DXF output;
- LINE and closed LWPOLYLINE encoding;
- output readable by the existing 30C decoder;
- exact re-mapping by 30D;
- valid empty DXF output;
- transactional mapper failure; and
- injected allocation failures in the export IR, mapping and writer paths.

## 12. Priority 30 exit point

30F completes the first narrow import/export architecture:

```text
ASCII DXF -> source IR -> exact Plan reference -> ancillary Project reference

SiteHelper Storey authority -> normalized export IR -> ASCII DXF
```

The next CAD work should be driven by user value rather than broadening the CAD
kernel. Candidates include reference rendering/reload resources, richer Plan
export representations, semantic import, or a DWG adapter. None requires changing
the core rule that external CAD types stop at the interoperability boundary.
