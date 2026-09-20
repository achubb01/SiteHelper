# Priority 28F — Document layer architecture review

Priority 28F reviews the concrete production document families established by
Priorities 28A–28E before extracting further common infrastructure. The result is
intentionally narrow: SiteHelper now has enough evidence to share **document
identity/context**, but not enough semantic similarity to justify a universal
annotation payload, command, geometry model or persistence record.

## Concrete families reviewed

At the time of the 28F review the production document layer owned four sibling payload families:

- plan notes: one Plan anchor, authored text and an optional weak model-object ID;
- persistent dimensions: two typed references plus a drafting offset, with the
  measured value derived from authoritative geometry;
- plan symbols: a concrete symbol kind, Plan anchor and kind-specific orientation;
- plan callouts: target point, label anchor and authored text.

All four are Project-owned non-physical authority. Each has a globally stable
`DomainId`, an explicit Plan Storey scope, command/history lifecycle, Plan
selection/rendering and persistence. None has physical Storey ownership.

## Abstraction that has been earned: document identity

The editor previously represented the four families as four selection enum values
and four mutually exclusive ID fields. Reconciliation repeated the same question
four times: "does this stable document ID still exist on the current Plan
Storey?"

Priority 28F introduces the narrow value type:

```c
DocumentObjectRef {
    DocumentObjectKind kind;
    DomainId id;
}
```

`DocumentObjectKind` distinguishes note, dimension, symbol and callout identity.
`document_model_object_storey_id()` resolves only the object's explicit document
Storey scope; it does not claim physical ownership.

The editor now stores all four families as `EDITOR_SELECTION_DOCUMENT` plus one
`DocumentObjectRef`. Family-specific selection setter functions remain as small
readability wrappers, but the selection container and reconciliation logic no
longer grow a new field/enum branch for every document family.

This is deliberately an **identity abstraction**, not a payload base class.

## Abstractions deliberately rejected

### Universal annotation payload

A common tagged union/base payload would not remove real distinctions. Notes own
text and a weak model association; dimensions own typed measurement references;
symbols have kind-specific orientation; callouts own two-point leader geometry
and text. A generic payload would simply move those branches into a larger union
and make validation/persistence coupling stronger.

Keep the four authoritative payload families separate.

### Generic document mutation command

Create/edit/delete command shapes look superficially similar, but ownership and
validation are not. Note/callout commands deep-own text, dimension commands carry
reference semantics, and symbol commands validate concrete symbol kinds. A
`EDIT_DOCUMENT_OBJECT` command would still require family-specific clone,
destroy, validation and project mutation branches.

Keep typed commands and typed history snapshots.

### Generic Plan hit-test geometry

The actual selectable geometry differs:

- notes: anchor proximity;
- symbols: anchor proximity (presentation can extend beyond the anchor);
- callouts: leader segment distance;
- dimensions: resolved/derived drafting geometry requiring Project context.

The current family-specific query functions make those semantics explicit.
Selection precedence remains editor/application policy rather than a document
"repository" query.

### Generic persistence record

The persisted authority differs materially by family and has evolved at different
format versions. Preserve explicit typed records. A generic record would trade
clear migration logic for tagged-payload parsing without reducing the amount of
family-specific code.

### Shared styling/layer base

All document renderers currently repeat some presentation choices, but SiteHelper
does not yet have a real paper/sheet, layer, print-style, visibility or text-style
model. Extracting style fields now would freeze assumptions from temporary screen
rendering. Wait for a concrete sheet/printing or visibility requirement.

### Generic authoring tool state

The workflows are genuinely different: one-click point markers, two-click
oriented symbols, two-point-plus-offset dimensions, two-point-plus-text callouts,
and click-plus-text notes. Shared transient state would be more complicated than
the concrete tools it replaced.

## Naming debt retained intentionally

`DocumentAnnotation` / `annotations` currently mean the original plan-note family,
not the whole modern document layer. That naming predates dimensions, symbols and
callouts and is now misleading. Renaming it to a concrete `DocumentPlanNote`
family would improve terminology, but it is a broad mechanical API/persistence
code churn with no architectural behavior change. Priority 28F records this as
bounded naming debt rather than mixing a repository-wide rename into the identity
refactor.

A future cleanup may rename the in-memory APIs while retaining the established
persistence grammar for backward compatibility.

## Boundaries after 28F

The intended dependency direction remains:

```text
physical model authority
        ↓ optional weak/stable references
project-owned document payloads
        ↓ family-specific derived geometry / queries
editor selection: DocumentObjectRef
        ↓
application rendering / authoring UI
```

Document identity must not become a generic Project repository abstraction. It is
only the common value shape needed by editor/document infrastructure.

## What should trigger the next abstraction

Do not add another common document base field merely because a fifth family
appears. Extract shared state only when at least two concrete features require the
same authoritative semantics. Likely future triggers include:

- a real visibility/layer system used consistently by multiple document families;
- sheet/paper-space placement and print styling;
- reusable text-style authority used by both notes and callouts;
- stable cross-document references (for example section/detail markers targeting
  a real sheet/view object);
- revision/markup lifecycle that proves common review-state metadata.

Until one of those exists, concrete document families plus shared identity is the
preferred architecture.


## 28G1 follow-up: revision markup does not invalidate the 28F verdict

Priority 28G1 subsequently adds a fifth identity kind for revision clouds. It
reuses `DocumentObjectRef` exactly as intended, but its payload is a deep-owned
closed Plan boundary and its hit/render geometry differs again from the existing
families. This is further evidence for keeping payloads/commands typed rather
than introducing a universal annotation base.

The revision-cloud slice deliberately does not invent common review-state
metadata. A later review-status/revision workflow may establish shared fields
(author, issue, status, revision linkage), but one geometric markup type is not
enough evidence to freeze that model.


### 28G2 evidence

Revision lifecycle metadata reinforces the 28F boundary. `DocumentRevision` is
shared project-level document metadata rather than another Plan payload and is
therefore not forced into `DocumentObjectRef`. Revision clouds retain their typed
geometry and carry only a weak stable-ID grouping link. This is another case
where stable identity is reusable infrastructure but payload/workflow semantics
should remain explicit.
