# Priority 30D — CAD Plan mapping / reference proposal

Priority 30D is the first boundary that turns external CAD source coordinates into
canonical SiteHelper Plan coordinates. It remains a **reference-geometry mapping
stage**, not semantic building import.

The implemented path is:

```text
CadIrDocument
    |
    v
unit resolution + exact layer policy
    |
    v
checked exact source-unit -> integer-mm conversion
    |
    v
optional integer-mm translation
    |
    v
CadPlanReference (PlanPosition paths + diagnostics)
```

No Wall, Room, Slab, Roof, Storey, DomainId, command, persistence or renderer API
is used by this layer.

## 1. Why this is a separate layer

The DXF adapter from 30C knows how to read source format syntax. It deliberately
does not know whether a raw value is suitable as a SiteHelper coordinate.

30D owns that policy. In particular it decides:

- which physical source unit applies;
- whether an explicit unit override supersedes source metadata;
- whether a source coordinate is exactly representable as a whole millimetre;
- whether an optional Plan translation remains in the canonical `int` range;
- which exact layer names participate; and
- which source paths must be skipped because their numeric mapping is unsafe.

This keeps unit/coordinate policy out of DXF parsing and keeps CAD concepts out of
SiteHelper's authoritative domains.

## 2. Canonical output coordinates

30D is the point where the CAD pipeline intentionally crosses into the canonical
SiteHelper coordinate type:

```c
PlanPosition
```

The mapped result does not invent a second integer point type. `PlanPosition`
continues to mean physical building-plan coordinates in millimetres.

The result is still reference data rather than Project authority. Using
`PlanPosition` means only that its coordinates satisfy the same numeric contract
that later SiteHelper APIs expect.

## 3. Unit resolution

`CadPlanMappingConfig` may contain an explicit supported physical-unit override.
Resolution follows the 30A order:

1. explicit override, when supplied;
2. otherwise a supported physical `CadIrDocument.declared_unit`;
3. otherwise mapping is blocked with `UNITS_REQUIRED`.

Supported physical units are:

- inch;
- foot;
- millimetre;
- centimetre; and
- metre.

An override is recorded through `CAD_PLAN_UNIT_FROM_OVERRIDE`. Source metadata is
recorded through `CAD_PLAN_UNIT_FROM_SOURCE`.

When both values are supported, nonzero physical declarations disagree, and an
override is used, mapping proceeds with the override but adds
`UNITS_OVERRIDE_CONFLICT` as a warning. Unspecified/unsupported source metadata
plus an explicit override is not called a conflict: the override is the required
resolution.

A globally unresolved unit is not an allocator/parser failure. 30D returns a
self-contained `CAD_PLAN_REFERENCE_BLOCKED_UNITS` proposal with `UNITS_REQUIRED`
and no mapped paths. The caller may present that proposal and rerun mapping with
an explicit unit choice.

## 4. Exact decimal-to-millimetre conversion

30B source numbers are exact finite decimals:

```text
coefficient * 10^exponent10
```

30D never converts them through `double`.

The physical-unit scales used by the mapper are exact decimal identities:

```text
1 mm   = 1 mm
1 cm   = 10 mm
1 m    = 1000 mm
1 in   = 25.4 mm = 254 * 10^-1 mm
1 ft   = 304.8 mm = 3048 * 10^-1 mm
```

For negative decimal exponents, denominator factors are cancelled exactly against
the integer coefficient and unit multiplier. If a denominator remains, the
coordinate is not a whole millimetre.

Examples:

```text
4.2 m    -> 4200 mm
100 cm   -> 1000 mm
10 in    -> 254 mm
5 ft     -> 1524 mm
1 in     -> 25.4 mm -> not representable
0.5 in   -> 12.7 mm -> not representable
```

No epsilon and no rounding tolerance exists in 30D.

A path containing any coordinate that cannot become an exact whole millimetre is
omitted with `NUMERIC_NON_WHOLE_MM`. A coordinate or translation outside the
`PlanPosition` integer range omits that path with `NUMERIC_OVERFLOW`.

Numeric failure is path-local, so valid paths in the same source document still
survive as a partial mapping result.

## 5. Translation only

After exact unit conversion, 30D may apply one caller-provided integer-mm
translation:

```text
mapped.x = exact_mm.x + translation_mm.x
mapped.y = exact_mm.y + translation_mm.y
```

Both additions are checked before narrowing to `int`.

30D deliberately does not add:

- arbitrary rotation;
- mirroring;
- nonuniform scale;
- floating-point transform matrices;
- survey/geodetic conversion; or
- automatic `$INSBASE` interpretation.

Those capabilities require a later explicit exact/lossy transform policy rather
than hidden float-and-round behaviour.

## 6. Exact layer filtering

`CadPlanLayerFilter` supports:

- all decoded source paths;
- an exact-name include set; or
- an exact-name exclude set.

Matching uses the decoded layer string exactly. There are no wildcard, regex,
case-folding or construction-semantic rules.

Filtering is not an error. Filtered paths are counted separately from paths
skipped by numeric mapping.

In particular, this stage still does not interpret:

```text
"WALLS" -> Wall
"DOORS" -> Opening
"LEVEL 1" -> Storey
```

## 7. Self-contained reference proposal

A `CadPlanReference` deep-owns:

- source format/version strings;
- declared and resolved unit information;
- translation configuration;
- mapped `PlanPosition` path arrays;
- copied source provenance for every emitted path;
- copied adapter diagnostics;
- mapping diagnostics; and
- decoded/emitted/filtered/skipped accounting.

This means the source `CadIrDocument` may be destroyed immediately after a
successful mapping call. No parser buffer or source provenance pointer is
borrowed by the mapped result.

DXF handles remain copied opaque strings. There is still no conversion to
`DomainId`.

## 8. Diagnostic composition

30D copies adapter diagnostics into the reference proposal before adding mapping
conditions. The final proposal can therefore carry both format/subset warnings
and mapping warnings/errors without retaining the source IR.

30D adds these contract diagnostics:

- `UNITS_REQUIRED`;
- `UNITS_OVERRIDE_CONFLICT`;
- `NUMERIC_NON_WHOLE_MM`; and
- `NUMERIC_OVERFLOW`.

Adapter diagnostics such as `ENTITY_UNSUPPORTED_KIND` and
`STYLE_NOT_PRESERVED` remain unchanged and preserve source provenance.

## 9. Accounting

`CadPlanReferenceStatistics` distinguishes:

```text
decoded_entity_count
adapter_skipped_entity_count
source_path_count
emitted_path_count
filtered_path_count
mapping_skipped_path_count
info / warning / error diagnostic counts
```

The categories are deliberately separate. A filtered path is not called an
invalid source entity, and an entity skipped by 30C is not double-counted as a
30D numeric failure.

## 10. Transactional ownership

`cad_plan_map_reference()` builds an independent candidate result first.

Technical failures such as allocation failure or malformed caller-owned IR leave
the caller's existing `CadPlanReference` unchanged. Only a fully constructed
READY or BLOCKED_UNITS proposal replaces it.

Entity-local numeric failures are ordinary partial-result diagnostics rather than
technical transaction failure.

## 11. 30D acceptance cases now covered

The dedicated tests cover:

- metre decimal -> exact integer Plan millimetres;
- centimetre override conversion;
- exact inch conversion;
- fractional-inch rejection;
- exact foot conversion;
- unresolved units -> blocked proposal;
- explicit override of unitless source;
- supported-unit override conflict warning;
- exact integer-mm translation;
- translation overflow;
- source-coordinate overflow;
- exact include/exclude layer filtering;
- adapter diagnostic copying;
- provenance lifetime after destroying the IR;
- end-to-end ASCII DXF -> IR -> Plan mapping; and
- injected allocation failure without partial output replacement.

No test requires SDL, rendering or a live `SiteHelperProject`.

## 12. What 30D still does not do

30D intentionally adds no:

- CAD reference object to `SiteHelperProject`;
- persistence for imported references;
- renderer/editor integration;
- Storey assignment;
- semantic wall/opening/room/slab/roof recognition;
- DomainIds for reference paths;
- automatic CAD-to-construction commands;
- rotation/mirroring/scaling; or
- export representation.

Those concerns belong to later slices rather than being smuggled into numeric
mapping.

## 13. Next boundary: 30E

30E can now decide how a mapped `CadPlanReference` is owned and applied.

For the first reference workflow, the next architectural question is whether the
reference should become a project/storey-owned background object, an editor/session
resource, or another explicitly scoped artifact. Whichever lifecycle is chosen
must preserve atomic replacement/import and keep reference geometry distinct from
construction authority.

Semantic conversion should remain a later mapping workflow. 30E should not turn
straight reference paths into Walls simply because 30D can now express them in
Plan millimetres.
