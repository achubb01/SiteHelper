# Priority 30B — Minimal CAD intermediate representation

Priority 30B makes the first CAD boundary executable without turning SiteHelper
into a CAD database. The production representation introduced here exists only to
carry the source information required by the 30A DXF plan-reference workflow
between format decoding and later mapping.

The boundary remains:

```text
ASCII DXF
   |
   v
30C format adapter
   |
   v
CadIrDocument                 <- Priority 30B
   |
   v
30D mapping / exact Plan-mm conversion
   |
   v
reference geometry / later semantic proposal
```

`CadIrDocument` is SiteHelper-owned data, but it is **not** `SiteHelperProject`
authority and it deliberately has no dependency on project/domain code.

## 1. What 30B represents

The first IR carries only four families of information proven necessary by 30A:

1. source-document metadata;
2. exact 2D straight source paths;
3. value-only source provenance; and
4. diagnostics plus source-entity accounting.

There is no universal CAD entity hierarchy.

In particular, 30B adds no representation for circles, arcs, splines, blocks,
text, hatches, dimensions, meshes, solids or xrefs. 30C will report unsupported
DXF objects as diagnostics/provenance instead of manufacturing empty `CadEntity`
variants for them.

## 2. Source metadata

A `CadIrDocument` owns:

```text
source_format       e.g. "DXF"
source_version      e.g. "AC1032"
declared_unit       normalized source-unit classification
```

The unit classification distinguishes:

- unspecified;
- inch;
- foot;
- millimetre;
- centimetre;
- metre; and
- other/unsupported.

This is **declared source metadata**, not the final import unit decision.
Priority 30D still owns override precedence, conflict reporting and checked
conversion into SiteHelper millimetres.

A format-specific raw unit code need not become a permanent IR field. If it is
useful for an unsupported-unit diagnostic, the adapter can preserve it in the
diagnostic detail text.

## 3. Exact source decimal values

The key numeric decision in 30B is:

```c
CadIrDecimal = coefficient * 10^exponent10
```

with a signed 64-bit coefficient and signed decimal exponent.

This is intentional. The source representation must not perform this hidden
conversion:

```text
DXF text -> double -> integer millimetres
```

because a binary floating-point parse can make the later question "was this
coordinate exactly representable as an integer millimetre?" unnecessarily
ambiguous.

For example, the source values below can remain exact in the IR:

```text
4.2       -> coefficient 42, exponent -1
0.001     -> coefficient 1,  exponent -3
4200      -> coefficient 4200, exponent 0
1e6       -> coefficient 1, exponent 6
```

30B does not itself convert those values to millimetres. Priority 30C will parse
supported DXF numeric spelling into this exact representation or issue a numeric
diagnostic when the source token cannot fit the representation. Priority 30D
then applies unit conversion and translation with checked integer arithmetic.

No `double`, generic geometry `Vec2`, `PlanPosition` or implicit rounding enters
the 30B contract.

## 4. One straight-path geometry type

Both 30A source primitives normalize naturally to one representation:

```text
DXF LINE
  -> open path with two vertices

straight DXF LWPOLYLINE
  -> ordered open/closed path with N vertices
```

Therefore 30B owns `CadIrPath`, not a `LINE` struct plus a `LWPOLYLINE` struct and
not a universal entity union.

Each path contains:

- an owned array of exact source-space XY vertices;
- vertex count;
- open/closed state; and
- copied source provenance.

An open path requires at least two vertices. A closed path requires at least
three. The closed flag does not require or cause a duplicate first vertex at the
end of the array.

Format-specific validity still belongs to the adapter. For example, 30C is
responsible for rejecting a non-zero DXF bulge before appending a path. The IR
does not need a `bulge` field merely so it can carry unsupported geometry.

## 5. Provenance is copied value data

Every retained path has:

```text
entity ordinal
entity kind
layer
optional opaque handle
```

The strings are deep copies owned by the IR. They may outlive:

- the source file buffer;
- parser scratch storage; and
- any future third-party library object graph.

The DXF handle remains an opaque string. Even if a handle looks like a decimal or
hexadecimal integer, no 30B API parses it into `DomainId` and the IR does not
include `domain_id.h`.

Provenance is diagnostic/reference context only. Layer is not Storey ownership;
entity kind is not SiteHelper construction type.

## 6. Diagnostics are extensible without a DXF enum leaking everywhere

`CadIrDiagnostic` stores:

- severity (`INFO`, `WARNING`, `ERROR`);
- an owned stable code string;
- optional owned detail text; and
- optional copied source provenance.

The code is a string rather than a closed C enum. This lets 30C use the 30A
contract vocabulary such as:

```text
ENTITY_UNSUPPORTED_KIND
ENTITY_UNSUPPORTED_CURVE
STYLE_NOT_PRESERVED
FORMAT_MALFORMED
```

without forcing a future DWG or other adapter to extend a giant DXF-shaped core
enum before it can report its own adapter-specific conditions.

Severity is intentionally typed because result accounting needs a common,
format-independent distinction between informational, warning and error items.

A diagnostic may be document/global rather than entity-specific. This supports
conditions such as unresolved units or file-level metadata warnings without
inventing fake entity provenance.

## 7. Source accounting is separate from path storage

The IR stores adapter-reported:

```text
decoded_entity_count
skipped_entity_count
```

and derives:

```text
source_path_count = path_count
info/warning/error counts = diagnostics by severity
```

The counts are deliberately not forced into an equation such as:

```text
decoded == path_count + skipped
```

Even though the initial DXF subset is mostly one-entity-to-one-path, that
assumption is not an architectural property worth freezing into the
intermediate representation.

Likewise, `path_count` is not yet 30A's final "emitted Plan path" count. These are
still source-coordinate paths. Priority 30D will own mapped/reference emission
and its own exact-conversion failures.

## 8. Ownership and mutation contract

`CadIrDocument` is a caller-owned aggregate containing owned nested allocations.
It must be zero-initialized or initialized with `cad_ir_document_init()`.

The 30B mutation API consists of:

```text
set metadata
append path
append diagnostic
set source-entity counts
```

All strings, vertices and provenance are deep copied.

Each operation is transactional with respect to the existing document:

```text
success -> requested mutation committed
failure -> existing document contents/counts unchanged
```

This includes allocation failure. Candidate nested values are allocated before a
collection resize is committed, and there is no fallible operation after a
successful resize.

`cad_ir_document_destroy()` releases all nested ownership, zeros the document,
and is safe for `NULL`, zero values and repeated calls.

The structure exposes capacities because the current C codebase uses explicit
owned dynamic arrays and append helpers. Capacity is implementation bookkeeping,
not source semantics. Callers should not mutate it directly.

## 9. Dependency boundary

The production target is intentionally independent:

```text
sitehelper_cad_ir
    |
    +-- C standard library only
```

`cad_ir.h` includes only standard size/integer headers. It does not include or
link:

- `sitehelper_project.h`;
- `domain_id.h`;
- Wall/Room/Slab/Roof headers;
- commands/history;
- editor/rendering;
- persistence;
- SDL; or
- an external DWG/DXF library.

The 30C DXF adapter may depend **on** this target. The dependency must never be
reversed.

## 10. What 30B deliberately does not solve

Priority 30B does not implement:

- file I/O;
- DXF group-code parsing;
- `$ACADVER` validation;
- `$INSUNITS` decoding;
- binary DXF rejection;
- model/paper-space checks;
- bulge/extrusion/width interpretation;
- source decimal text parsing;
- exact source-unit-to-mm arithmetic;
- translation into Plan coordinates;
- layer filtering;
- reference rendering/ownership in the editor/project;
- semantic wall/opening recognition; or
- DXF export.

Those responsibilities remain in the stages already allocated by Priority 30.

## 11. 30C contract

Priority 30C can now be implemented without inventing another data model.
For the initial ASCII DXF adapter it should:

1. validate text representation and supported `$ACADVER`;
2. decode `$INSUNITS` into `CadIrSourceUnit`;
3. parse supported numeric tokens into `CadIrDecimal` exactly;
4. append accepted `LINE` as two-vertex open paths;
5. append accepted straight `LWPOLYLINE` as ordered open/closed paths;
6. deep-copy layer/kind/handle provenance through the IR API;
7. append diagnostics for unsupported/malformed/skipped entities;
8. set decoded/skipped accounting; and
9. return the IR without touching `SiteHelperProject` or `PlanPosition`.

30C should not add fields to `CadIrDocument` simply because another DXF group
code exists. A field belongs in the IR only if a supported workflow needs it
across the adapter boundary.

## 12. 30B verification

The tests cover:

- initialization and repeated destruction;
- metadata deep-copy/replacement;
- exact decimal preservation without floating point;
- open and closed source paths;
- no duplicate closing vertex requirement;
- opaque numeric-looking handles;
- input-buffer lifetime independence;
- global and source-specific diagnostics;
- result/statistics accounting;
- invalid argument/state rejection;
- transactional failures; and
- injected `malloc`/`realloc` failures.

The test target requires no SDL, renderer or project model.

## Priority 30B exit criteria

30B is complete when:

1. the first CAD IR is production code rather than documentation only;
2. it carries only the 30A source-path subset and required metadata/provenance;
3. source decimals remain exact until 30D mapping;
4. external handles remain opaque strings;
5. every nested value is owned independently from parser storage;
6. allocation failure cannot partially append/replace data;
7. no SiteHelper construction/domain type appears in the target; and
8. Priority 30C can parse ASCII DXF entirely above this boundary.
