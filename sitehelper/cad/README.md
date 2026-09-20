# Priority 30 — CAD import/export boundary

Priority 30 preserves a strict interoperability boundary around DWG, DXF and
other external drawing formats. SiteHelper has now established explicit
integer-millimetre Plan coordinates, Storey ownership and authoritative domain
objects, so CAD conversion no longer needs to influence the core model.

The governing dependency direction is:

```text
DWG / DXF / other external format
            |
            v
      format adapter
            |
            v
  intermediate representation
            |
            v
    mapping/import policy
            |
            v
     SiteHelper model
```

Export follows the reverse architectural boundary:

```text
SiteHelper model
       |
       v
 export/mapping policy
       |
       v
intermediate representation
       |
       v
   format adapter
       |
       v
DXF / DWG / other external format
```

No external-format type, handle, layer object or library object becomes a
SiteHelper domain type merely because an adapter can decode it.

## 1. Four different responsibilities

### Format adapter

A format adapter knows one external representation. It may use a third-party DWG
library, parse a published DXF representation or later support another format,
but it ends at a SiteHelper-owned interoperability boundary.

Its responsibilities are format mechanics such as:

- opening/reading/writing the external representation;
- decoding entities, blocks, layers and format metadata;
- exposing declared or discoverable drawing units and coordinate metadata;
- preserving enough source provenance for diagnostics; and
- reporting unsupported/corrupt format features without inventing SiteHelper
  meaning.

It does **not** decide that a DXF `LINE` is a Wall, that a polyline is a Slab, or
that a layer name implies a construction assembly. It also does not allocate
`DomainId`s.

Adapter/library types must remain private to that adapter. A future library
change must not require edits to `SiteHelperProject`, `Wall`, `Slab`, `Roof`,
commands, persistence or editor code.

### Intermediate representation

The intermediate representation is SiteHelper-owned but is still **external
drawing data**, not the authoritative building model. Its purpose is to stop each
format adapter from being coupled directly to every SiteHelper domain and to give
mapping/import policy one normalized source vocabulary.

It should carry only concepts proven necessary by a concrete interoperability
workflow: source geometry, grouping/layer information, text or block information
when required, explicit unit/coordinate metadata, and diagnostic provenance.

Do not pre-emptively build a general CAD kernel or mirror the complete DWG object
model. In particular, Priority 30 does not yet freeze a generic `CadEntity` union
or a permanent `CadDocument` schema. The first concrete adapter/use case should
establish the smallest representation that can losslessly express the supported
subset.

External handles/record IDs may be retained as opaque provenance while mapping,
but they are never SiteHelper `DomainId`s and never determine the global domain
identity allocator.

### Mapping/import policy

Mapping policy owns interpretation. This is where a user-selected or
workflow-specific policy may decide, for example:

- which source layers/entities participate;
- whether geometry is reference/background information or is being converted to
  authoritative SiteHelper objects;
- which Storey receives plan objects;
- how source origin, axes and rotation map to the SiteHelper Plan coordinate
  space;
- how source units become integer millimetres;
- whether a line is a wall centreline, wall face, separator or unrelated CAD
  geometry;
- how paired faces, polylines, openings or other patterns are recognized when a
  concrete importer supports them; and
- what to do with ambiguous, unsupported or conflicting source geometry.

These decisions must be explicit and diagnosable. A format adapter must not bury
construction heuristics inside parsing code.

A future "automatic wall recognition" feature belongs here or downstream of
here. It is interpretation, not file decoding.

### SiteHelper authoritative model

Only successfully mapped and validated SiteHelper objects enter project
authority. From that point onward they are ordinary SiteHelper objects regardless
of whether they were drawn manually, created by a command, loaded from SiteHelper
persistence or imported from CAD.

Imported walls therefore use the same authoritative `WallPlanSegment`, openings,
Storey ownership, validation and generated framing path as manually authored
walls. Do not preserve a second CAD-specific wall representation beside them.

Generated framing is not imported as the semantic meaning of a wall. Source
studs, hatch lines or other drawing detail may be useful evidence during mapping,
but they must not silently replace SiteHelper's authoritative wall definition or
its deterministic generation rules.

## 2. Coordinate and measurement contract

The existing measurement contract remains authoritative:

```text
source CAD coordinate + source unit metadata
                |
                v
      explicit import transform
  (origin / axes / rotation / scale)
                |
                v
 checked SiteHelper integer millimetres
                |
                v
        Project/model API
```

The core model never asks whether a coordinate began as metres, centimetres,
inches, feet or unitless drawing units.

The importer must explicitly resolve unspecified/ambiguous source units. It must
also detect numeric overflow and transformed coordinates that are not exactly
representable under SiteHelper's integer-millimetre authority. Those cases are
reported; they are not silently rounded.

If a later workflow permits snapping/quantizing imported data, that is an
explicit lossy **mapping policy** with visible diagnostics/tolerance, not an
accidental consequence of converting a floating-point library value to `int`.

Large survey/georeferenced coordinates may require an explicit import origin
shift before conversion so useful local building coordinates remain representable.
That transform is import configuration, not persisted CAD state in the core
model.

`PlanPosition` remains SiteHelper authority. No generic transform matrix, CAD
coordinate wrapper or runtime unit tag is added to Project/model types for CAD.

## 3. Layers, blocks and source metadata are not domain ownership

CAD organizational constructs do not automatically become SiteHelper ownership
constructs:

- source layer != Storey;
- block != Room, Wall assembly or reusable SiteHelper object;
- external entity handle != `DomainId`;
- xref != SiteHelper Project reference;
- line style/colour != construction material/specification; and
- CAD grouping != topology/room boundary authority.

A particular mapping profile may use layer/block names as evidence, but the
policy must state that interpretation explicitly. Unrecognized source metadata
may be preserved in an import report/provenance object when needed; it should not
be sprinkled through unrelated domain structs.

## 4. Reference geometry and semantic conversion are different workflows

Opening a CAD drawing as reference/background geometry is fundamentally safer
than claiming arbitrary CAD linework is a correct building model. Keep these
workflows separate.

A future reference-drawing feature may retain imported drawing geometry for
visual comparison/tracing without creating Walls, Slabs, Roofs or Rooms. That
reference data would require its own explicit ownership/lifecycle if persisted.
It must not masquerade as construction authority.

A semantic import instead maps a supported subset into candidate SiteHelper
objects. Any entity that cannot be interpreted confidently remains skipped,
reference-only or diagnostic according to the chosen policy; it is not guessed
into the model merely to maximize conversion count.

## 5. Import must be transactional

CAD import is not a persistence back door. The adapter must not call internal
load/parser helpers or write directly into owning arrays.

For a whole-project import, prefer:

```text
external drawing
    -> adapter / IR / mapping
    -> candidate SiteHelperProject
    -> sitehelper_project_validate(candidate)
    -> accept candidate or reject with diagnostics
```

Failure must not leave a partially imported live project.

A future "merge into current project" workflow needs the same atomic property.
It may stage an import proposal and apply an explicit command/batch only after all
objects and conflicts are validated. Do not incrementally mutate the live project
while still discovering whether the file can be imported.

Imported objects receive fresh SiteHelper identities unless a future, explicit
round-trip protocol defines and validates stronger semantics. External CAD
handles never commandeer the SiteHelper ID namespace.

## 6. Diagnostics and ambiguity are first-class outputs

Successful parsing does not imply successful mapping. Import should eventually
be able to distinguish at least:

- format/decode failure;
- unsupported source feature;
- missing or ambiguous units;
- numeric overflow/unrepresentable integer-mm coordinate;
- unsupported coordinate transform;
- mapping ambiguity;
- invalid SiteHelper candidate geometry;
- object conflict during merge; and
- successful import with skipped/reference-only source items.

Diagnostics should identify source provenance where possible (for example layer,
entity kind and opaque source record reference) without exposing third-party
library objects across the adapter boundary.

Do not convert uncertainty into geometry silently. The user/import policy should
be able to see what was interpreted and what was not.

## 7. Export is a view/translation, not SiteHelper persistence

SiteHelper persistence continues to serialize SiteHelper authority. CAD export is
a separate representation generated from that authority for interoperability.

An export policy decides which SiteHelper concepts become which drawing entities,
which target units/origin/layers are used, and whether derived/generated geometry
is included. For example, exporting wall centre-lines, wall faces and generated
framing are three different representations and must not be conflated merely
because all can be drawn as lines.

Export adapters must not require CAD state to be stored in the project just to
write a file. A round trip through an external format is not assumed to preserve
all SiteHelper semantics unless a later, explicitly versioned round-trip contract
is designed for that purpose.

## 8. Dependency rules

The intended dependency direction is:

```text
format-specific adapter
        |
        v
CAD/interchange IR + diagnostics
        |
        v
mapping policy / import proposal
        |
        v
public SiteHelper project/domain APIs
```

and never:

```text
SiteHelperProject -> DWG library
Wall              -> DXF entity
persistence       -> CAD adapter
framing generator -> import layer names
```

The CAD/interoperability area may depend on public domain/project APIs when it
performs mapping. Core domain, project, command, editor, persistence, take-off,
cut-list and structural layers must not depend on a CAD library or format adapter.

## 9. What Priority 30 intentionally does not add yet

This boundary decision adds no:

- DWG library dependency;
- DXF parser/writer;
- generic `CadDocument`/`CadEntity` production schema;
- automatic wall/opening/room/slab/roof recognition;
- CAD reference/background object in `SiteHelperProject`;
- layer-to-Storey convention;
- import conflict-resolution UI;
- coordinate rounding tolerance;
- external-handle-to-`DomainId` mapping;
- persistence format change; or
- claim that a CAD round trip is lossless.

Those choices should be introduced by the first concrete interoperability
workflow rather than guessed at the architecture stage.

## 10. Recommended implementation sequence

The next CAD work should be split so format mechanics and construction
interpretation remain independently testable:

1. **30A — First workflow + format contract. COMPLETE.**
   [`DXF_PLAN_REFERENCE.md`](DXF_PLAN_REFERENCE.md) fixes the first workflow as
   ASCII DXF model-space reference import, with an explicit version/entity/unit
   subset, exact Plan-mm mapping and diagnostics. It creates no construction
   semantics.
2. **30B — Minimal intermediate representation.** Introduce only the geometry and
   metadata required by 30A, with ownership/destruction and allocation-failure
   tests. No Project dependency.
3. **30C — DXF format adapter.** Decode/encode the supported subset into/from the
   IR. Test with fixtures; no Wall/Slab/Room creation in parser code.
4. **30D — Plan mapping/import proposal.** Resolve units/origin/rotation/layers,
   convert to checked integer millimetres and produce explicit candidate
   SiteHelper semantics plus ambiguity diagnostics.
5. **30E — Transactional application/reference ownership.** Add the actual
   project/reference lifecycle required by the chosen workflow and make import
   atomic with validation/history semantics.
6. **30F — Export policy.** Produce the supported external representation from
   SiteHelper authority through the same IR boundary. Only then assess DWG and
   richer round-trip needs.

DWG support can later be another format adapter over the same proven
interoperability concepts. Do not choose a DWG SDK first and then let its object
model define SiteHelper's architecture.

## Priority 30 exit decision

The architecture is ready for CAD work when every implementation can preserve
these invariants:

1. external format/library types stop at the adapter boundary;
2. the intermediate representation is SiteHelper-owned but is not building-model
   authority;
3. mapping policy, not parsing, assigns construction meaning;
4. all authoritative coordinates enter the model as checked integer millimetres;
5. external handles/layers/groups never become implicit SiteHelper identity or
   ownership;
6. import is transactional and project validation remains the final authority
   check;
7. unsupported/ambiguous/lossy interpretation is diagnosed rather than hidden;
8. export is an adapter/view of SiteHelper authority, not a replacement for
   SiteHelper persistence; and
9. core model/domain code remains completely independent of DWG/DXF libraries.
