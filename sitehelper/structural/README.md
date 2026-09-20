# Priority 29 — Structural calculation boundaries

Priority 29 preserves the boundary around future lintel, beam and member-sizing
work without introducing a structural design engine yet. SiteHelper already has
enough generated framing and roof structural-layout terminology that a future
feature could otherwise blur three different meanings:

```text
project geometry + explicit loading/support assumptions
                    |
                    v
          structural calculation engine
                    |
                    v
          structural calculation result

                    !=

      structural engineering approval
```

The calculation path may inform project design. It must never silently create,
represent or imply engineering approval.

## Four distinct kinds of state

Future structural work should keep these concepts separate even when a workflow
shows them together.

### 1. Project/design authority

The normal SiteHelper model owns authored construction intent: geometry, stable
identity, explicit structural/support intent once such a domain exists, and any
member specification the user deliberately commits to the design.

A committed beam or lintel size can therefore become **project design authority**
through an explicit command. That means "this is the member the project intends
to use". It does not mean that SiteHelper has proved the member adequate or that
an engineer has approved it.

Generated Wall framing is not a shortcut around this rule. In particular,
`TIMBER_HEADER` currently means a generated opening-framing member role. Its
section comes from the Wall framing settings and its length from opening geometry;
it is not an engineered lintel selection.

### 2. Calculation input

A structural calculation consumes a bounded input snapshot assembled from
project/design authority plus explicit analysis assumptions. Inputs may include,
for example:

- geometry and spans;
- support/bearing intent and load-path assumptions;
- permanent/imposed/wind or other loading inputs;
- material/member candidates; and
- the jurisdiction, design basis, Standard/source edition and any licensed table
  or product data required by the concrete engine.

Do not let a calculation engine reach through arbitrary editor state or mutate
`SiteHelperProject` while calculating. Prefer value/snapshot inputs or narrow
read-only adapters whose provenance is explicit. A missing structural assumption
must produce an incomplete/unsupported result rather than be invented from
nearby geometry.

This extends the roof rule already proven by Priority 26F: a geometric ridge,
eave, hip or valley does not become a structural support/member by implication.

### 3. Calculation result

A calculation result is derived evidence about one exact input set. It is not
model authority merely because it was produced successfully.

A future result should carry enough provenance to answer at least:

- what input snapshot/objects and assumptions were checked;
- which engine and engine version produced the result;
- which design basis/source-data versions were used;
- what member candidate/check was evaluated;
- the numeric outputs, governing checks and diagnostics; and
- whether the result is complete, unsupported, indeterminate or failed to
  calculate.

Prefer caller-owned value/snapshot results. Do not give ordinary calculation
results global `DomainId`s, command history or persistence merely to make them
convenient to display. If caching is later justified, cached results need an
input/provenance fingerprint and must become stale when any relevant authoritative
input changes.

Avoid an unqualified result field named `approved`, `engineered` or similar. A
calculation engine may report its own scoped check outcome, but that outcome must
remain visibly tied to the inputs, method and limitations that produced it.

### 4. Engineering approval / external assurance

If SiteHelper later records engineer approval, certification or other external
assurance, that is a separate authoritative record with its own explicit
lifecycle. It is not a boolean promoted from a calculation result.

That future record will need a concrete requirement before its schema is frozen,
but likely concerns provenance such as the approving party/document, scope,
date/version, referenced project/design state and supporting artefacts. Whether
such a record belongs in the document layer or a dedicated assurance/compliance
area should be decided from the first real workflow rather than assumed here.

The critical invariant is independent of storage location:

```text
calculation succeeded
        |
        +---- does not create ----> engineering approval

user committed a calculated member size
        |
        +---- does not create ----> engineering approval
```

## Intended dependency direction

Keep structural calculation downstream of authoritative spatial/design domains:

```text
SiteHelperProject / domain authority
        |
        +--> derived geometry / structural layout
        |          |
        |          v
        +----> calculation-input adapter
                    |
                    v
             calculation engine
                    |
                    v
             calculation result
                    |
                    +--> UI/report/export
                    |
                    +--> explicit user command may copy a chosen design value
                         into project authority

external engineering approval / assurance
        |
        +--> separately authored/imported record, optionally referencing
             project/design state and/or a calculation artefact
```

The core project/model, command validation and persistence layers must not depend
on a calculation result in order to consider ordinary project geometry valid.
Likewise, a structural engine should not depend on GUI/editor/render code.

## Relationship to existing SiteHelper subsystems

### Wall framing

`WallFraming` and `Timber` describe generated physical framing geometry. Headers,
studs, plates, sills and noggins may later become inputs to structural checks, but
the current generator does not establish their structural adequacy. A sizing
engine must not reinterpret `TIMBER_HEADER` as "approved lintel".

### Roof structural layout

Priority 26F remains upstream topology/intent. It resolves structural fields and
support intent but deliberately does not calculate spans, reactions, member sizes
or compliance. Those later calculations belong behind this Priority 29 boundary.

### Take-off and cut list

Take-off and cut-list queries report committed/generated construction. They do
not validate structural adequacy. A future calculator may propose a different
member, but reporting/optimisation should consume that member only after an
explicit design-authority mutation has committed it. Calculation results must not
silently change purchasing or saw-bench output.

### Project validation and persistence

Current project validation checks authoritative model integrity; it is not a
structural design verifier. Do not make project load/save or ordinary validity
depend on having current structural calculations or engineering approval.

If structural calculation artefacts are ever persisted, they require explicit
staleness/provenance semantics and must remain distinguishable from both project
design authority and engineering approval.

## API rules for the later calculation subsystem

When the first real structural calculation is implemented:

1. create the subsystem for the concrete calculation need, not a generic
   `engineering_repository`;
2. make inputs explicit and read-only during a calculation;
3. return owned/value results with typed diagnostics and provenance;
4. make unsupported/missing assumptions fail visibly rather than heuristically
   inventing authority;
5. keep calculation execution free of Project mutation;
6. use a separate command if the user chooses to commit a calculated member or
   specification into project design authority;
7. keep any approval/assurance record separately authored/imported; and
8. test that authoritative mutation invalidates or makes stale any cached result
   that depended on the changed input.

A pure function is not required if a future engine needs tables, catalogues or a
solver context, but the observable boundary should still behave like a query:
inputs in, derived result out, no hidden project mutation.

## What Priority 29 intentionally does not add

Priority 29 adds no:

- lintel/beam sizing equations or span tables;
- load combinations, wind calculations or reaction calculation;
- structural member/product catalogue;
- structural `DomainId` family;
- structural commands or editor tools;
- persistence format change;
- `approved`/`certified` project flag;
- automatic bearing-wall inference;
- automatic conversion of generated headers into selected lintels; or
- claim of standards compliance.

Those features require concrete requirements and, where applicable, the exact
jurisdiction/source-data/version contract of the engine being built.

## Priority 29 exit decision

The existing architecture can preserve the required safety boundary without a
new production code target. Priority 29 is complete when future work has an
unambiguous rule to follow:

1. generated geometry and structural-layout state are not sizing approval;
2. calculation inputs are explicit snapshots/assumptions;
3. calculation results are derived and carry provenance;
4. committing a calculated design value is an explicit authoritative mutation;
5. engineering approval/assurance is a distinct record and cannot be inferred
   from calculation success or project design state; and
6. take-off, cut-list, persistence and project validation cannot silently promote
   a calculation result into authority.
