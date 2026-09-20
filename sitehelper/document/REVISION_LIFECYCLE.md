# Revision lifecycle metadata — Priority 28G2

Priority 28G2 adds the first lifecycle/grouping authority for revision markup
without inventing a sheet-issue or collaboration model that SiteHelper does not
yet have.

## Project-level revision record

`DocumentRevision` is project-owned document metadata, not a Plan object. It
owns:

- a globally stable `DomainId`;
- a non-empty human-facing identifier such as `A`, `B`, `P1` or `1`;
- an optional description (stored as an owned string, which may be empty).

A revision record has no Storey scope and is therefore deliberately excluded
from `DocumentObjectRef` and Plan selection. It can group revision clouds across
multiple Storeys.

Identifiers are not required to be globally unique. Stable identity is the
`DomainId`; future sheet/document scope may legitimately permit the same visible
revision identifier in different publication contexts.

## Cloud association

`DocumentPlanRevisionCloud.revision_id` is an optional weak stable-ID reference
to a `DocumentRevision`. `DOMAIN_ID_INVALID` means the cloud is unassigned.

Explicit assignment requires a live revision record. Once stored, however, the
association is weak: deleting the revision record leaves the cloud valid and
retains the revision ID. Undo/restoration of the same stable ID reconnects the
cloud automatically. If that ID is live as a non-revision object, Project
validation rejects the state.

Editing cloud geometry preserves its existing revision assignment. Cloud
creation remains unassigned until a revision-management/properties workflow
chooses a record. `SetPlanRevisionCloudRevisionCommand` is the reversible
command-history boundary for assigning, reassigning or clearing that grouping;
the Project setter remains the lower-level mutation primitive.

## Persistence

Persistence v22 adds a project-level `revisions` collection before
`revision_clouds`, and each v22 cloud stores its optional `revision_id`.

Versions v1-v21 load with zero revision records. A v21 revision cloud has no
revision field and therefore loads as unassigned.

Revision strings use byte-counted hexadecimal encoding, matching the existing
owned-text persistence approach and allowing whitespace/newlines in the
optional description.

## Deliberately deferred lifecycle fields

Priority 28G2 does **not** add:

- author/user identity;
- created/modified timestamps;
- issue date;
- `draft` / `issued` / `superseded` state;
- sheet/document issue membership;
- approvals or reviewer assignments;
- per-cloud comments.

Those concepts depend on models SiteHelper does not yet possess. In particular,
`issued` is a publication/sheet concept while `open/resolved` is a review-task
concept; collapsing both into one status enum would prematurely mix two
workflows.

The next UI that exposes revisions should use the typed revision-record commands
and `SetPlanRevisionCloudRevisionCommand` rather than direct application-layer
writes. The project/document APIs in 28G2 remain the lower-level
transactional/persistence boundary.
