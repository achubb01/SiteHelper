# Priority 30C — ASCII DXF format adapter

Priority 30C implements the first concrete CAD format boundary defined by 30A.
It decodes the supported **ASCII/text DXF** subset into the Priority 30B
`CadIrDocument` representation and stops there.

```text
ASCII DXF bytes
      |
      v
 dxf_ascii.c
      |
      v
CadIrDocument
(source units + exact source decimals + provenance + diagnostics)
      |
      v
Priority 30D mapping
```

The adapter does not know about `SiteHelperProject`, `DomainId`, Walls, Rooms,
Slabs, Roofs, editor state, persistence, rendering or Plan millimetres.

## 1. Public boundary

`dxf_ascii_decode_memory()` accepts one complete borrowed byte sequence and an
initialized `CadIrDocument` destination.

The operation is transactional:

- success destroys/replaces the destination with a newly decoded document;
- any file-level, allocation or internal IR failure leaves the destination
  unchanged; and
- the resulting document owns all retained strings and geometry independently of
  the source buffer.

No path/file-system API is frozen in 30C. File selection and byte acquisition are
application concerns; this adapter owns DXF representation decoding only.

## 2. Representation gate

30C accepts the ASCII group-code/value representation and rejects Autodesk's
22-byte binary-DXF sentinel as `DXF_ASCII_DECODE_UNSUPPORTED_REPRESENTATION`.
Embedded NUL bytes are also rejected at this boundary rather than being passed
through a text parser.

The parser accepts LF and CRLF line endings, whitespace-padded group-code lines,
and DXF group-999 comments. A dangling group-code line, invalid group-code line,
broken section structure, missing required sections or missing EOF marker is a
file-level malformed result.

## 3. HEADER decoding

The adapter reads only the metadata required by 30A:

- `$ACADVER` (group 1); and
- `$INSUNITS` (group 70).

Supported `$ACADVER` values are:

```text
AC1015 AC1018 AC1021 AC1024 AC1027 AC1032
```

Missing version metadata and unsupported versions are distinct file-level results.

`$INSUNITS` is deliberately **not** converted into Plan millimetres here. The
adapter maps only the source declaration into the 30B unit vocabulary:

```text
0 / missing -> UNSPECIFIED
1           -> INCH
2           -> FOOT
4           -> MILLIMETRE
5           -> CENTIMETRE
6           -> METRE
other       -> OTHER
```

Priority 30D owns explicit override handling, unit conflict warnings and exact
source-unit-to-millimetre conversion.

## 4. Exact source numbers

DXF real values are parsed without `strtod()` and without an intermediate
`double`. The adapter converts finite decimal/scientific notation directly into:

```text
CadIrDecimal = coefficient * 10^exponent10
```

Examples:

```text
4.2e3     -> 42 * 10^2
-1.2500   -> -125 * 10^-2
5000.000  -> 5 * 10^3
```

The representation is normalized by removing insignificant leading/trailing
zeroes. Numeric syntax errors and values that cannot fit the 30B exact-decimal
contract are entity diagnostics (`NUMERIC_INVALID` / `NUMERIC_OVERFLOW`) rather
than floating-point approximations.

This is intentionally still **source-space** data. Whether an exact source value
can become an integer SiteHelper millimetre is a 30D decision.

## 5. `LINE`

A `LINE` is retained as a two-vertex open `CadIrPath` only when:

- start X/Y and end X/Y are present and valid;
- start/end Z are absent or exactly zero;
- thickness is absent or exactly zero;
- extrusion is absent/default `(0, 0, 1)`;
- it is model-space geometry; and
- its two source points are distinct.

Otherwise the source entity remains represented by provenance + diagnostic and
increments the skipped count.

## 6. `LWPOLYLINE`

A `LWPOLYLINE` is decoded into one ordered open/closed `CadIrPath` when:

- group 90 is present and matches the decoded vertex count;
- every vertex has an exact X/Y pair;
- elevation and thickness are absent/zero;
- extrusion is absent/default `(0, 0, 1)`;
- all bulges are absent/zero;
- it is model-space geometry; and
- it contains enough distinct source points for the requested open/closed shape.

The group-70 closed bit is preserved without manufacturing a repeated final
vertex.

Nonzero constant/per-vertex widths do not become wall thickness. The centreline
path is retained and a `STYLE_NOT_PRESERVED` warning is emitted.

Any nonzero bulge causes the entire entity to be skipped with
`ENTITY_UNSUPPORTED_CURVE`; 30C does not tessellate or chord curves.

## 7. Unsupported and non-model content

Every other ENTITIES-record kind is counted and reported as
`ENTITY_UNSUPPORTED_KIND`. Unsupported records are not expanded, approximated or
converted into placeholder geometry.

Supported geometry is also skipped when it is paper-space/layout content or when
it requires non-default 3D/OCS handling. The adapter recognizes the relevant
common entity fields (`67` and `410`) and the entity extrusion fields needed by
the 30A subset.

## 8. Provenance

Every emitted path and source-tied diagnostic carries copied provenance:

- one-based source entity ordinal within `ENTITIES`;
- DXF entity kind;
- layer name (defaulting to `"0"` when absent); and
- optional opaque DXF handle.

Handles remain strings even when they look numeric. They are never parsed as or
converted to `DomainId`.

## 9. Result accounting

`decoded_entity_count` counts every entity record encountered in `ENTITIES`.
`skipped_entity_count` counts entities for which no source path was retained.
`path_count` is the number of supported retained straight paths.

A retained polyline with a width-loss warning is not skipped. Unsupported,
curved, 3D, paper-space, degenerate, malformed and numerically invalid entities
are skipped individually while otherwise valid entities survive.

This gives 30D/application code enough information to distinguish a clean decode
from a partial reference decode without pretending that all source content was
reproduced.

## 10. Allocation and failure behavior

Temporary parser storage and all `CadIrDocument` mutations are staged in a local
document. If any parser allocation, IR deep-copy allocation or capacity growth
fails, the local result is destroyed and the caller's destination remains
unchanged.

The test suite injects allocator failures through the complete adapter + IR call
path to verify this property.

## 11. Deliberate omissions

30C does not add:

- DWG support;
- binary DXF decoding;
- old-style `POLYLINE`;
- `ARC`, `CIRCLE`, `SPLINE`, `ELLIPSE`, `HATCH`, text or dimensions;
- `INSERT`/block expansion;
- xrefs/xdata interpretation;
- arbitrary OCS/WCS transforms;
- layer filtering;
- unit overrides;
- source-to-mm conversion;
- import translation/rotation;
- reference-object ownership in `SiteHelperProject`; or
- semantic CAD-to-building recognition.

Those are later mapping/workflow capabilities, not parser conveniences.

## 12. 30C verification target

The dedicated tests cover:

- all supported version metadata paths;
- source unit metadata;
- exact decimal/scientific parsing;
- `LINE` decoding;
- open/closed `LWPOLYLINE` decoding;
- width-loss warnings;
- bulge rejection;
- model-space filtering;
- 3D/OCS rejection;
- unsupported entity accounting/provenance;
- degenerate geometry;
- malformed/numeric source entities;
- malformed file structure;
- missing/unsupported versions;
- binary DXF rejection;
- destination replacement on success;
- destination preservation on failure; and
- injected allocation failure throughout the adapter/IR stack.

Priority 30C is complete when this decoder remains format-only and 30D can consume
`CadIrDocument` without knowing anything about raw DXF group codes.
