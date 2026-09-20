# Priority 30A — DXF plan reference workflow and format contract

Priority 30A chooses the first concrete CAD interoperability workflow. It is
intentionally narrower than "import a building from CAD":

> **Read simple 2D model-space linework from an ASCII DXF file as plan reference
> geometry, preserving enough source metadata to diagnose and filter the import,
> without creating authoritative SiteHelper construction objects.**

This workflow exists to prove the CAD boundary established in `cad/README.md`:
format decoding, external-drawing representation, unit/coordinate mapping and
SiteHelper authority remain separate concerns.

Semantic recognition of Walls, Rooms, Slabs, Roofs, openings or assemblies is
**not** part of 30A. A user should eventually be able to bring in a floor-plan
DXF, inspect/reference its supported linework and trace or map from it without
SiteHelper pretending the source drawing already has SiteHelper semantics.

## 1. Why reference import comes first

Reference import exercises the difficult interoperability boundaries without
prematurely inventing construction heuristics:

- external file/version validation;
- DXF group-code decoding;
- source units;
- source coordinates and plan conversion;
- layers and source provenance;
- unsupported/partially supported entities;
- exact integer-millimetre conversion policy; and
- diagnostics for information that cannot be represented safely.

It does not require the parser to answer questions such as whether two parallel
lines are wall faces or whether an `INSERT` represents a window. Those are later
mapping-policy problems.

Reference data is therefore **external drawing data**, not a second building
model and not generated construction.

## 2. User-visible 30A workflow

The eventual workflow established by this contract is:

```text
choose DXF file
     |
     v
validate supported DXF representation/version
     |
     v
decode supported 2D model-space source geometry
     |
     v
resolve drawing units explicitly
     |
     v
checked conversion to SiteHelper Plan millimetres
     |
     v
reference import result + diagnostics
```

The result may later be owned/rendered by the editor under 30E. Until that
lifecycle exists, 30B/30C should stop at the decoded/mapped reference result.
They must not mutate `SiteHelperProject` merely to make a parser testable.

A successful import may contain warnings and skipped source entities. Success
means that the supported subset was decoded and mapped according to the contract;
it does **not** mean every object in the DXF was reproduced.

## 3. File representation and version gate

### Text DXF only

30A supports **ASCII/text DXF only**. Binary DXF is rejected at the format gate
with a specific unsupported-representation diagnostic. A binary reader can be a
future adapter capability if it proves useful.

The adapter must consume the normal DXF group-code/value pair representation. It
must not shell out to AutoCAD, depend on an installed CAD application, or expose
third-party parser objects outside the adapter if a library is introduced later.

### Supported `$ACADVER`

For the first adapter, accept these drawing database versions:

| `$ACADVER` | AutoCAD format generation |
| --- | --- |
| `AC1015` | AutoCAD 2000 |
| `AC1018` | AutoCAD 2004 |
| `AC1021` | AutoCAD 2007 |
| `AC1024` | AutoCAD 2010 |
| `AC1027` | AutoCAD 2013 |
| `AC1032` | AutoCAD 2018 |

This deliberately starts at AutoCAD 2000. It gives the first workflow one modern
2D polyline representation and explicit drawing-unit metadata while covering the
DXF generations commonly produced by current AutoCAD save/export paths.

A missing, malformed or unlisted `$ACADVER` is a format-contract failure for 30A;
do not guess compatibility from the file extension.

Supporting a version means only that the **30A entity subset** is recognized in
that version. It does not claim support for every entity/object available in that
DXF generation.

## 4. Sections read by the first adapter

The first adapter needs only enough of the DXF document to establish the
contract:

- `HEADER` — `$ACADVER` and `$INSUNITS`;
- `ENTITIES` — supported model-space drawing entities; and
- entity-level layer/provenance fields required by this document.

Other sections may be structurally skipped while retaining enough information to
continue group-pair parsing safely.

30A does not interpret:

- `BLOCKS` / block definitions;
- `OBJECTS` dictionaries;
- `CLASSES`;
- xrefs;
- application xdata;
- dimension styles;
- text styles; or
- layout/paper-space content as plan reference geometry.

If a later supported entity requires another section, add that dependency because
of a concrete workflow rather than making the first parser a general DXF database
reader.

## 5. Supported source geometry

The first import subset is deliberately just straight 2D path geometry.

### `LINE`

Accept a `LINE` when all of the following hold:

- start and end X/Y coordinates are present and finite;
- start/end Z are absent or exactly zero;
- thickness is absent or zero; and
- extrusion direction is absent/default `(0, 0, 1)`.

A zero-length source line may be decoded for provenance but is not emitted as
reference path geometry; report it as a skipped degenerate entity.

### `LWPOLYLINE`

Accept a `LWPOLYLINE` when all of the following hold:

- its declared vertex count and actual X/Y vertex data are consistent;
- it contains at least two vertices for an open path, or enough distinct geometry
  to form a closed path;
- elevation is absent or exactly zero;
- extrusion direction is absent/default `(0, 0, 1)`; and
- every bulge value is absent or exactly zero.

The closed flag is preserved so the final vertex may connect back to the first
without the adapter manufacturing a duplicate source vertex.

Nonzero constant/vertex widths do not change the path centreline. The first
workflow may retain the centreline **only if it emits a visible lossy-style
warning** stating that polyline width is not reproduced. Width must never be
silently interpreted as wall thickness.

A nonzero bulge makes curved geometry. 30A does not chord/flatten that curve;
the entire source entity is skipped with an unsupported-curve diagnostic.

### Everything else

All other graphical entity types are unsupported by this first workflow,
including `ARC`, `CIRCLE`, old-style `POLYLINE`, `SPLINE`, `ELLIPSE`, `HATCH`,
`TEXT`, `MTEXT`, dimensions and `INSERT`/blocks.

They are not format errors merely because they occur in an otherwise supported
DXF. Skip them with counted diagnostics/provenance so a user can see that the
reference is incomplete.

Do **not** explode blocks, approximate splines, flatten arcs, derive hatch
boundaries or rasterize unsupported entities behind the user's back.

## 6. Model space only

30A imports model-space geometry only. Paper-space/layout entities are skipped
with diagnostics.

The parser must not infer that paper-space annotation belongs in building Plan
space merely because its numeric coordinates look plausible.

Likewise, entities that require a non-default object-coordinate-system transform
are outside the first subset. The only accepted `LWPOLYLINE` object coordinate
system is the ordinary +Z planar orientation described above.

This keeps the first mapping rule explicit:

```text
accepted source XY plane
        -> unit conversion
        -> Plan X/Y millimetres
```

rather than quietly embedding arbitrary 3D/OCS projection in the format parser.

## 7. Unit resolution

SiteHelper's measurement contract remains authoritative: Plan coordinates that
cross into SiteHelper-owned plan/reference geometry are exact integer
millimetres. 30A never assumes that a raw DXF number is a millimetre.

Resolve source units in this order:

1. an explicit import-unit override supplied by the caller/user; otherwise
2. supported `$INSUNITS` metadata; otherwise
3. fail unit resolution and require explicit input.

The first mapping policy recognizes these physical source units:

| `$INSUNITS` | Unit |
| --- | --- |
| `1` | inch |
| `2` | foot |
| `4` | millimetre |
| `5` | centimetre |
| `6` | metre |

`$INSUNITS = 0`, a missing `$INSUNITS`, or another unit code is **not guessed**.
The import result must say that an explicit supported unit is required.

An explicit override intentionally wins over file metadata because real-world
DXF files can contain missing or incorrect unit declarations. The result must
record that an override was used and, when it conflicts with a nonzero supported
`$INSUNITS`, emit a warning rather than hiding the discrepancy.

## 8. Numeric conversion: exact millimetres, no accidental rounding

The format adapter should preserve source numeric values well enough for the
mapping layer to determine whether conversion is exact. Do not make an early
`double -> int` cast the unit policy.

Conceptually:

```text
DXF decimal coordinate
       |
       v
source numeric value + known unit
       |
       v
exact/checkable conversion to millimetres
       |
       +---- non-whole mm / overflow ----> diagnostic + entity not emitted
       |
       v
integer Plan coordinate
```

Examples:

- `4200` in a millimetre drawing -> `4200 mm`;
- `4.2` in a metre drawing -> `4200 mm`;
- `100` in a centimetre drawing -> `1000 mm`;
- `10` inches -> `254 mm`;
- `1` inch -> `25.4 mm`, which is **not** an authoritative integer Plan
  coordinate and is therefore unrepresentable under 30A.

30A adds no quantization tolerance. If a later reference workflow intentionally
allows snapping imported coordinates to a tolerance, that must be a named,
visible lossy mapping policy with diagnostics and tests.

NaN, infinities, numeric parse overflow and integer-millimetre overflow are hard
numeric diagnostics. They must never reach SiteHelper geometry APIs.

## 9. Plan transform for the first workflow

30A intentionally uses the smallest useful coordinate transform:

```text
source XY
   -> source-unit-to-mm conversion
   -> optional integer-mm translation
   -> SiteHelper Plan X/Y
```

The translation is caller/import configuration and is expressed in SiteHelper
integer millimetres. It can place a large-coordinate drawing near the local
project origin without changing scale.

30A does **not** add:

- arbitrary rotation;
- axis mirroring;
- nonuniform scale;
- survey/geodetic coordinate systems;
- automatic use of `$INSBASE` as SiteHelper origin; or
- a generic transform matrix in the model.

Those can be introduced by mapping policy when a real import requires them.
Arbitrary rotation in particular must not be added by simply rotating in floating
point and rounding back to Plan integers.

Translation overflow is checked after unit conversion.

## 10. Layers are metadata, not construction meaning

For each supported source entity, preserve its DXF layer name as source metadata.
The first workflow may allow include/exclude filtering by exact layer name before
reference geometry is emitted.

No layer naming convention is built into 30A. In particular:

```text
"WALLS"      != Wall
"DOORS"      != Opening
"LEVEL 1"    != Storey
"STRUCTURAL" != structural approval/authority
```

Colour, linetype and lineweight are not part of the first geometry contract.
Ignoring them is acceptable only as documented reference-style loss; they cannot
be used as hidden semantic classifiers.

Layer names are source metadata/provenance and must not become `DomainId`s or
Project ownership keys.

## 11. Provenance required for every decoded entity

30B's minimal intermediate representation must be able to associate supported or
skipped source items with enough value-only provenance to make diagnostics useful.
For 30A that means, where present/available:

- source entity ordinal in parse order;
- entity kind (`LINE`, `LWPOLYLINE`, unsupported kind);
- layer name;
- opaque DXF handle string; and
- diagnostic kind/severity associated with that source item.

The handle is informational only. It is never parsed as, converted into, or
reserved as a SiteHelper `DomainId`.

Do not retain pointers into parser buffers or a third-party DXF object graph in
provenance.

## 12. Result and diagnostic semantics

The first workflow needs a distinction between **file failure**, **mapping
failure**, and **partial success**.

### File-level failure

No reference result is produced for conditions such as:

- file cannot be read;
- binary DXF under the 30A text-only contract;
- malformed group-code/value structure that prevents safe parsing;
- missing/unsupported `$ACADVER`; or
- required section structure is irrecoverably malformed.

### Mapping-level blocking failure

The DXF may decode, but mapped Plan geometry cannot be emitted until the caller
fixes a global condition such as unresolved source units.

The decoded source result may still be useful for reporting layers/entities, but
there is no pretence that its raw coordinates are SiteHelper Plan millimetres.

### Partial success

A reference result may succeed while individual entities are skipped. Examples:

- unsupported `ARC` beside supported lines;
- a paper-space entity;
- a bulged `LWPOLYLINE`;
- one coordinate that cannot convert to whole millimetres; or
- a degenerate line.

The result must expose counts for decoded, emitted, skipped and warning/error
items so the caller can make incompleteness visible.

Suggested stable diagnostic *categories* for later implementation are:

- `FORMAT_UNSUPPORTED_REPRESENTATION`;
- `FORMAT_UNSUPPORTED_VERSION`;
- `FORMAT_MALFORMED`;
- `UNITS_REQUIRED`;
- `UNITS_OVERRIDE_CONFLICT`;
- `ENTITY_UNSUPPORTED_KIND`;
- `ENTITY_UNSUPPORTED_CURVE`;
- `ENTITY_UNSUPPORTED_3D_OR_OCS`;
- `ENTITY_PAPER_SPACE`;
- `ENTITY_DEGENERATE`;
- `STYLE_NOT_PRESERVED`;
- `NUMERIC_INVALID`;
- `NUMERIC_NON_WHOLE_MM`; and
- `NUMERIC_OVERFLOW`.

These names are contract vocabulary, not yet a frozen public C enum. 30B should
choose the smallest API that can express them without forcing every future CAD
adapter into DXF-specific diagnostics.

## 13. No SiteHelper semantic objects in 30A

The following would be an architectural failure in the first adapter:

```c
/* Wrong layer: parser invents SiteHelper meaning. */
sitehelper_project_add_wall(project, ...);
```

or:

```c
/* Wrong identity boundary. */
DomainId id = domain_id_from_dxf_handle(handle);
```

30A reference linework has no Wall/Room/Slab/Roof identity and causes no framing,
topology, take-off, cut-list or structural calculation.

If later semantic import recognizes a wall, that later mapping stage should
create a normal SiteHelper Wall through normal authoritative APIs and validation.
The DXF parser itself remains unchanged.

## 14. Export under 30A

30A deliberately does **not** implement or freeze the DXF export representation.
Priority 30F will choose an export policy from actual SiteHelper authority through
the proven intermediate boundary.

The read contract here should not be contorted to make a future writer trivially
mirror it. Import/reference source data and exported SiteHelper drawing data have
different authority and provenance semantics even when both use DXF primitives.

## 15. 30B constraints derived from this contract

Priority 30B may now introduce a minimal SiteHelper-owned interchange/reference
representation, but it must be no larger than this workflow requires.

It needs to express approximately these concepts without freezing these exact
names/layouts:

```text
source document metadata
  - source format/version
  - declared/resolved unit information

source path geometry
  - LINE path
  - open/closed straight LWPOLYLINE path
  - source-space coordinates before authoritative mapping

source provenance
  - ordinal
  - layer
  - optional opaque handle

import diagnostics/result accounting
```

30B does **not** need a universal `CadEntity` union covering circles, splines,
text, blocks, hatches or 3D solids. Unsupported source entities can initially be
represented by provenance/diagnostic records rather than fake geometry nodes.

The 30B representation must not depend on:

- `SiteHelperProject`;
- Wall/Room/Slab/Roof headers;
- command/history;
- editor/rendering;
- persistence; or
- any external DXF/DWG library.

## 16. Acceptance matrix for 30B/30C

The implementation following this contract should eventually prove at least the
following fixtures/cases:

| Case | Expected result |
| --- | --- |
| `AC1015`, mm, one valid `LINE` | one emitted straight reference path |
| `AC1032`, metres, exact decimal coordinates | exact integer-mm Plan path |
| `$INSUNITS = 0`, no override | mapping blocked with `UNITS_REQUIRED` |
| unitless file + explicit mm override | succeeds; override recorded |
| supported nonzero file unit + conflicting override | override used + warning |
| inches producing whole mm | succeeds exactly |
| inches producing fractional mm | affected entity skipped; `NUMERIC_NON_WHOLE_MM` |
| open straight `LWPOLYLINE` | ordered open path |
| closed straight `LWPOLYLINE` | ordered closed path, no invented duplicate vertex |
| `LWPOLYLINE` with bulge | skipped; unsupported-curve diagnostic |
| nonzero polyline width | path retained only with style-loss warning |
| 3D/non-default extrusion entity | skipped; 3D/OCS diagnostic |
| paper-space supported entity | skipped; paper-space diagnostic |
| unsupported `ARC` among valid lines | valid lines survive; partial-success diagnostic |
| zero-length line | skipped as degenerate |
| numeric overflow / NaN / infinity | rejected before Plan integer creation |
| translation overflows `int` | affected mapped geometry rejected |
| DXF handle resembles numeric DomainId | remains opaque provenance only |
| malformed group-pair structure | file-level failure; no partial authoritative mutation |
| binary DXF | unsupported-representation failure |

No test should require an SDL window, renderer or live `SiteHelperProject`.

## 17. Priority 30A exit criteria

30A is complete when this contract is accepted as the next implementation target
and later CAD work can answer these questions without guessing:

1. **What is the first workflow?** 2D DXF model-space reference linework.
2. **Which format representation?** ASCII/text DXF.
3. **Which versions?** `AC1015` through `AC1032` generations listed above.
4. **Which geometry?** straight 2D `LINE` and straight `LWPOLYLINE` only.
5. **Which units?** explicit override or supported `$INSUNITS`; never guessed.
6. **Which SiteHelper coordinates?** exact checked integer Plan millimetres after
   optional integer-mm translation.
7. **What happens to unsupported content?** skip/report without approximation or
   semantic invention.
8. **What source metadata survives?** layer + value-only provenance/handle when
   available.
9. **Does import create Walls/etc.?** no.
10. **What is next?** 30B builds the smallest independent intermediate
    representation needed to make this contract executable.

## 18. External DXF references used by this contract

The external format facts above are based on Autodesk's DXF documentation, in
particular:

- HEADER variables (`$ACADVER`, `$INSUNITS`):
  <https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-A85E8E67-27CD-4C59-BE61-4DC9FADBE74A.htm>
- `LINE` group codes:
  <https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-FCEF5726-53AE-4C43-B4EA-C84EB8686A66.htm>
- `LWPOLYLINE` group codes:
  <https://help.autodesk.com/cloudhelp/2026/CSY/AutoCAD-DXF/files/GUID-748FC305-F3F2-4F74-825A-61F04D757A50.htm>
- AutoCAD drawing-format compatibility:
  <https://help.autodesk.com/view/ACADWEB/ENU/?caas=caas%2Fsfdcarticles%2Fsfdcarticles%2FAutoCAD-drawing-file-format.html>

These references define the external syntax/metadata only. They do not define
SiteHelper construction semantics or ownership.
