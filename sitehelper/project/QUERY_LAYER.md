# Project query boundary (Priority 27)

Priority 27 deliberately does **not** add a generic `project_query` or repository
abstraction. The current domain model already has the right query boundaries,
and centralising them now would couple unrelated domains to `SiteHelperProject`
without removing meaningful duplication.

## What belongs at Project scope

`SiteHelperProject` owns the global DomainId namespace and the Storey hierarchy.
Queries whose answer depends on those two facts belong at Project scope:

- find a Storey by stable ID;
- find a globally identified authoritative object by its concrete type and ID;
- find the owning Storey for a globally identified object;
- test whether a DomainId is already occupied anywhere in the project, including
  project-owned document annotations.

These are identity/ownership lookups, not a general-purpose query layer. The
current typed functions in `sitehelper_project.h` are intentionally explicit.
Do not replace them with an untyped object union, string/type key, generic
repository, callback/predicate API, or central registry of borrowed pointers.

Project-wide typed lookup is also the correct boundary for commands that receive
only a stable object ID. Returned pointers are borrowed and must be reacquired
after collection mutation or project replacement.

Priority 28 adds one important distinction: a globally contained ID need not have
an owning Storey. Document annotations are owned directly by `SiteHelperProject`;
their explicit Storey-plan anchor is scope, not containment. Therefore
`sitehelper_project_contains_domain_id()` includes annotations while
`sitehelper_project_find_owning_storey*()` remains a physical ownership query and
returns no owner for annotation IDs. Do not collapse those two meanings again.

## What stays subsystem-local

Queries that interpret domain meaning stay with the subsystem that owns that
meaning:

| Need | Current / intended owner |
| --- | --- |
| Walls on one Storey | `Storey.structure` / wall model traversal |
| Openings in or by ID within one Wall | wall APIs (`wall_find_opening_by_id*`) |
| Wall elevation hit testing | `wall_query` |
| Slab plan hit testing / nearest slab edge | `slab_plan_query` |
| Roof plan hit testing | `roof_plan_query` |
| Plan connectivity and faces | `plan_topology` |
| Neighbouring/intersecting physical walls | topology-derived `wall_junctions` |
| Room spatial region | `room_region` over `PlanTopology` |
| Future room perimeter | topology / room-region query code, derived from the resolved face |
| Generated framing material totals | `framing_takeoff` |
| Required generated pieces | `cut_list` |
| Future stock optimisation | `optimisation`, consuming required pieces rather than Project internals |
| Future structural calculations | dedicated structural calculation subsystem consuming explicit snapshots/assumptions; results remain derived |
| Annotation lookup/content | `document`, with Project providing only global typed identity access |

A feature may accept `SiteHelperProject` as a convenience input when it needs to
resolve an owner or aggregate across Storeys. That does not make the feature a
Project query. The interpretation and result types still belong to the feature
subsystem.

## Storey-local first

Physical plan relationships are Storey-local unless a future requirement makes a
cross-Storey relationship explicit. A caller that already knows the Storey should
query that Storey directly rather than scan the whole Project and filter the
answer afterwards.

This is especially important for topology, snapping and hit testing: identical
plan coordinates on different Storeys are independent inputs. Project-wide
aggregation is appropriate for reports such as take-off/cut-list output because
those subsystems explicitly define aggregation semantics.

## Result and ownership rules

Prefer query results that are one of:

1. stable DomainIds;
2. value-only result structs/snapshots owned by the caller; or
3. short-lived borrowed pointers with an explicit invalidation contract.

Do not create a long-lived Project index of raw pointers into Storey/domain
collections. Existing collection mutation can reallocate storage, while stable
DomainIds already provide the durable reference mechanism.

Derived topology, junctions, hit-test results, take-offs and cut lists remain
non-authoritative and are rebuilt/requeried after authoritative mutation. They
must not be persisted merely to accelerate lookup.

## When a broader module becomes justified

Revisit a dedicated project-query module only after concrete production call
sites demonstrate a shared operation rather than merely similar loops. Good
signals are:

- the same cross-Storey or cross-domain traversal appears in several independent
  features with the same filtering, ordering and error semantics;
- callers are duplicating a non-trivial ownership/identity rule that Project must
  enforce consistently;
- a shared value result can be defined without exposing domain internals or
  returning a generic "object" type; and
- moving the operation would reduce coupling rather than make Project depend on
  topology, rendering, editor or reporting policy.

Even then, prefer one narrow named query with a concrete result over a generic
repository abstraction. The module should be earned by repeated semantics, not
by the number of domain types in the project.

## Priority 27 review result

As of the Priority 27 snapshot, no broader project-query module is warranted.
The apparent query families are intentionally different:

- global typed ID/owner lookup is Project identity policy;
- Storey/domain collection lookup is local authoritative-model access;
- topology and room relationships are derived spatial queries;
- plan selection is view/tool-specific query policy; and
- take-off/cut-list operations are reporting/derivation pipelines.

Keeping those boundaries separate preserves low coupling and leaves room for a
future query module if real repeated cross-domain semantics emerge.
