# Priority 26G2A — Roof Lifecycle Commands and History

Priority 26G2A puts the first production roof lifecycle operations behind the
existing SiteHelper command/history contract. It intentionally covers
`CREATE_ROOF` and `DELETE_ROOF` only. Authoritative portion, composition and
termination edits are implemented separately by completed Priority 26G2B;
structural intent remains outside this lifecycle document.

## Command boundary

`CreateRoofCommand` owns a deep copy of the initial `RoofPortionSpec` support
polygon. Executing it delegates to the 26G1 transactional project mutation and
returns both stable identities:

```text
CREATE_ROOF
    -> Roof DomainId
    -> initial RoofPortion DomainId
```

The history entry owns an independent clone of the command, so the caller may
destroy its command immediately after execution.

`DeleteRoofCommand` carries only the target Roof ID. Before deletion, command
history captures an exclusively owned deep `Roof` snapshot plus its Storey ID
and collection index. The snapshot contains authoritative roof source intent
only. It never snapshots generated planes, ridges, hips, valleys, seams,
interfaces or structural-layout results.

## Exact identity semantics

Undoing `CREATE_ROOF` removes the created Roof only when the expected Storey,
Roof ID and initial portion ID still match. The global ID generator watermark is
not rewound. Redo reconstructs the source with the exact original Roof and
portion IDs and inserts it only if both identities are still free and below the
current watermark.

Undoing `DELETE_ROOF` restores the complete authoritative Roof with every
original portion ID, composition/termination relationship and original Storey
collection position. Any collision with the Roof ID or any nested portion ID
causes undo to fail closed without moving the history cursor.

This follows the existing command-history identity rule: history restores prior
identity; it never steals an identity from a newer live object.

## Project restoration primitive

`sitehelper_project_insert_roof_at()` is the narrow restoration/redo primitive.
It deep-copies the supplied Roof, validates its authoritative source, proves it
can regenerate through the current roof geometry kernel, checks every Roof and
portion identity against the project-global namespace, and inserts at an exact
Storey-local collection index.

The function does not advance the global ID watermark. History callers are
responsible for proving restored IDs are older than the current watermark.

## Transactional guarantees

A failed command/history operation preserves:

- project roof authority;
- the global ID watermark;
- command-history count and cursor;
- existing live IDs; and
- the previous `SiteHelperCommandResult` failure contract (`NONE`).

Undo/redo move the history cursor only after the domain mutation succeeds.

## Persistence boundary

Priority 26G3 now persists the authoritative Roof state manipulated by these
commands in project format v15. Command history still never serializes derived
geometry. See `ROOF_PERSISTENCE.md`.

## Follow-on: 26G2B complete

Priority 26G2B now implements authoritative source editing as an owned
`RoofSourceEditCommand` family with exact-authority history snapshots. See
`ROOF_SOURCE_EDIT_COMMANDS.md`. Derived roof geometry remains excluded from
history in both lifecycle and source-edit commands.
