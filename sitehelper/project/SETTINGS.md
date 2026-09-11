# Priority 9B — Construction settings ownership and resolution

Project defaults plus an explicit Storey stud-height override now resolve into
one transient construction configuration. Priority 9A containment and identity
rules are unchanged. Only `stud_height` has a Storey override.

## Stored and resolved state

`SiteHelperProject.settings` remains a complete, authoritative `BuildSettings`
default value. `Storey.settings` is a `StoreyBuildSettings` containing
`bool has_stud_height_override` and `int stud_height`. New Storeys have no
override. Inherited state has a canonical zero payload; the boolean selects
inheritance, never a magic numeric height. An enabled override must be positive.
An explicit override equal to the Project default remains explicitly enabled.

`BuildSettings` is also the complete transient resolved value passed downward.
This avoids a duplicate scalar type. Its roles are documented in the model,
Project, Wall and editor API headers. There is no stored effective value in
Wall, Storey, editor or persistence, and no settings cache.

The sole resolution rule is `project_resolve_build_settings` in the Project
subsystem. It validates Project defaults and the override representation, copies
all defaults, and replaces only `stud_height` when the flag is enabled. All
other current fields resolve directly from Project defaults. Opening custom
allowances retain their existing precedence inside the Opening subsystem.

Resolution allocates nothing and does not mutate authoritative state. Failure
does not change the output. The public resolver finds the Storey by ID; the
internal helper also accepts staged settings during mutation and parsing.
This is the same rule, not a second inheritance path. In particular, legacy
parsing can resolve its provisional zero-ID Storey before migration assigns
that Storey's permanent identity.

Elevation does not participate. Existing millimetre units, top-plate placement,
stud lengths, opening limits and integer rounding are preserved. The name
`stud_height` is retained; it must not be interpreted as ceiling height,
floor-to-floor height or an overall assembled wall height. The existing noggin
ceiling-division/product intermediates were widened to `int64_t` to avoid
overflow for valid large positive heights, without changing their rounding.

## Public APIs and mutation transaction

All new Project functions use the existing `int` success/failure convention:

- `sitehelper_project_resolve_storey_build_settings(project, storey_id, output)`
- `sitehelper_project_set_build_settings(project, defaults)`
- `sitehelper_project_set_stud_height(project, stud_height)`
- `sitehelper_project_set_storey_stud_height(project, storey_id, stud_height)`
- `sitehelper_project_clear_storey_stud_height(project, storey_id)`

The resolver rejects null arguments, invalid/unknown Storey IDs and invalid
settings. Output is an independent complete `BuildSettings` value. Like the
existing Project lookup APIs, it assumes coherent collection metadata.

Mutations validate the existing authoritative Project and proposed scalar
settings first. They compare each Storey's old and proposed resolved values.
Only Walls whose effective configuration changes receive replacement framing.
A height-only Project-default change therefore leaves explicitly overridden
Storeys untouched. Setting/changing/clearing one override affects only that
Storey's Walls. Changing override presence at an equal effective height commits
the presence change without allocations or regeneration. Empty Projects and
empty Storeys need no framing allocations.

For affected Walls, the transaction borrows definitions and builds separately
owned framing in stored Storey/Wall order. Existing opening validation runs
against the proposed resolved configuration before generation. All candidate
framing must succeed before any stored settings or live framing is replaced.
Failure frees staged framing and preserves settings, definitions, IDs, allocator,
all original framing allocations and collection pointers. Success commits the
settings and framing together, leaving authoritative Wall definitions unchanged.
The temporary pending-Wall pointers never escape this synchronous operation;
no Storey/Wall collection can reallocate inside it.

The complete-default setter is deliberately a scalar transaction, not a generic
inheritance framework. It is needed because the existing CLI settings form edits
height together with other defaults. The form now collects checked input into
a local value and commits once; partial input and regeneration failures leave
the Project unchanged. The CLI spacing action uses the same boundary.

Settings changes were not command-backed and remain outside history. Successful
application mutations reconcile editor selections and transient previews.
Future callers must do likewise after successful settings mutations.

## Construction, editor and history integration

Wall creation commands and redo resolve the command's target Storey. Opening
execute/undo/redo and Wall endpoint execute/undo/redo find the existing Wall's
owner and resolve current settings. DeleteWall restoration resolves the captured
Storey ID before validating/restoring its Openings and framing. CLI generation
and length changes also resolve the active Storey; length changes now validate
and regenerate transactionally rather than directly changing the segment.

The core `sitehelper_project_add_wall` keeps its existing definition-only
semantics: it does not generate framing. Whenever construction is requested,
the creation-command/CLI paths pass resolved settings. Project validation still
permits authoritative Walls with no generated framing.

`sitehelper_editor_pointer_move_in_project` resolves the active Storey and passes
the local current Wall plus resolved settings to the existing low-level pointer
helper. SDL uses this entry point. Opening clicks revalidate the candidate so a
settings change between pointer movement and command creation cannot leave a
stale validity decision. No authoritative defaults/overrides are copied into
editor state. The Wall Tool's plan preview remains a segment with no vertical
framing; its creation command resolves the same active Storey on execution.

History snapshots retain authoritative definitions/IDs, not generation settings.
Undo/redo therefore uses the current effective settings. For example, redoing
an Opening or restoring a deleted Wall can fail if a subsequent height reduction
makes the old Opening invalid. Such failure preserves the history cursor and
Project and can be retried after a compatible settings change.

Wall/model construction code never looks up Project or Storey ownership. The
model only validates scalar defaults/override representation; resolution lives
in Project orchestration. No topology or WallJunction changes were needed.

## Validation and persistence

Project validation remains read-only, allocation-free and deterministic. Passes
are Project defaults, all collection metadata, Storey settings in stored order,
global IDs/watermark, and local geometry/Openings. The new
`SITEHELPER_PROJECT_INVALID_STOREY_SETTINGS` result identifies the Storey.
Project defaults must remain valid even if every Storey overrides height.
Storey settings reject nonpositive enabled overrides and nonzero inherited
payloads. Opening validation uses each Storey's resolved settings. Generated
framing is not a validation input.

Persistence v9 adds exactly one record after each Storey's elevation:

```text
storey ID elevation MM
stud_height inherit
```

or:

```text
storey ID elevation MM
stud_height override 2700
```

The existing Walls/Rooms/separators records follow. Only defaults and explicit
override state are saved, never effective height or framing. Opening parsing,
final Project validation and framing regeneration all use resolved settings.
Load still stages the complete Project and replaces its destination only after
parse, validation and regeneration succeed.

Versions 1–7 retain their original ID-preserving single-Storey migration. Version
8 retains its existing Storeys, elevations, order and IDs. Every Storey loaded
from versions 1–8 inherits Project defaults; no override is invented. v9 preserves
the distinction between inheritance and an equal-valued explicit override.

## Coverage and remaining limits

`test_project_settings.c` covers allocation-free complete resolution; unknown
IDs/nulls; set/change/clear/equal overrides; signed/equal elevations; numeric and
canonical-state validation; independent Storey heights; Project-default and
Storey-only regeneration; unchanged definitions/IDs and unaffected framing
pointers; no-op mutation without allocations; Opening preview/command agreement;
custom allowances; stale-preview revalidation; Wall Tool creation; current-setting
Wall/Opening/endpoint/DeleteWall history; v9 mixed overrides; v8 inheritance;
malformed override loads; and late Opening-validation/generation failures after
an earlier Wall has successfully staged replacement framing.

Allocation injection exercises every failure point until a mutation succeeds,
checks all original framing pointers and independent semantic snapshots, and
retries. Existing Storey/persistence suites retain load fault injection and all
v1–7 grammar coverage, now explicitly asserting absent overrides. Shared Project
clone/equality helpers include Storey settings. Output-version assertions were
updated while legacy-input tests retain their original version numbers.

The C structs remain public. Direct assignments can still bypass regeneration;
headers reserve such writes for initialization, staged parsing and test fixtures.
All live production settings edits now use transactional APIs. Broad encapsulation
was intentionally avoided. Low-level standalone Wall/editor helpers likewise
require callers to supply an already resolved value; application paths do so.
No manual interactive GUI session was performed.

Deferred scopes: stud width/depth remain Project defaults, with possible future
Wall/type material specifications. Stud spacing/mode and nog spacing remain
Project defaults, with possible future Wall/type layout choices. Opening
allowances remain Project defaults plus explicit Opening custom allowances.
No Wall/type overrides, company settings, generic hierarchy, or elevation-derived
height is implemented.

## Verification and source audit

Final verification results, exact commands, production `stud_height` use
classification and the complete changed-file inventory follow below.

Fresh Debug configurations were created in separate directories. All final
code changes were then rebuilt and the full suites rerun:

| Configuration | Directory | Result |
| --- | --- | --- |
| Normal | `/tmp/p9b-normal` | 50/50 CTest tests passed, 0 failures |
| ASan + UBSan | `/tmp/p9b-sanitize` | 50/50 passed, 0 failures, no sanitizer/leak reports |
| Topology disabled | `/tmp/p9b-no-topology` | 47/47 passed, 0 failures |
| Strict diagnostic audit | Normal compilation database, all first-party C | 115/115 translation units passed |

Configuration/build/test commands (run from the repository root):

```sh
cmake -S . -B /tmp/p9b-normal -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p9b-normal -j 4
ctest --test-dir /tmp/p9b-normal --output-on-failure

cmake -S . -B /tmp/p9b-sanitize -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS=-no-pie
cmake --build /tmp/p9b-sanitize -j 4
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --test-dir /tmp/p9b-sanitize --output-on-failure

cmake -S . -B /tmp/p9b-no-topology -DCMAKE_BUILD_TYPE=Debug \
  -DSITEHELPER_BUILD_TOPOLOGY=OFF
cmake --build /tmp/p9b-no-topology -j 4
ctest --test-dir /tmp/p9b-no-topology --output-on-failure

git diff --check
```

The strict audit used each first-party entry in the normal compilation database,
replacing object output with `-fsyntax-only` and adding
`-Wall -Wextra -Wpedantic -Werror`. It includes unchanged consumers of modified
headers. Three pre-existing diagnostics were minimally corrected: the empty
editor-action translation unit now includes its own header, one unused test
parameter is explicitly marked unused, and one unused test-local settings value
was removed. No warnings were disabled. No compiler warnings occurred in the
normal, sanitizer or topology-disabled builds. SDL configuration still reports
the environment's missing optional libdecor development library.

Logs: `/tmp/p9b-{normal,sanitize,off}-{configure,build,rebuild,tests}.log` and
`/tmp/p9b-warnings.log`. `git diff --check` passes.

Settings mutation allocation-failure totals: Project height 34, Storey set/change
23, Storey clear 21, complete defaults 42 (120 total). Existing load fault
injection also passes: 30 v9 failures and 18 v7 migration failures. No automatic
commit was made.

Every production C/header occurrence of `stud_height` is classified below;
line numbers refer to the final source at implementation time. Test directories
are intentionally excluded and covered by the test report above.

| File | Lines | Intentional role |
| --- | --- | --- |
| `sitehelper/cli/actions.c` | 36, 65, 422 | Checked input into a local proposed default; two read-only displays explicitly describing Project/default stud dimensions. Construction calls use resolved values. |
| `sitehelper/model/build_settings.c` | 7, 8, 13 | Scalar validity: positive default/enabled override; canonical zero payload when inherited. |
| `sitehelper/model/build_settings.h` | 16, 32, 33 | Stored Project defaults / transient complete configuration field; explicit Storey override presence and payload declarations. |
| `sitehelper/persistence/sitehelper_persistence.c` | 301, 745, 750, 752, 838, 854, 856, 857 | Read/write stored Project default and explicit Storey override state; no persisted effective height. |
| `sitehelper/persistence/sitehelper_persistence.h` | 20 | Documentation of the persisted override grammar. |
| `sitehelper/project/project_settings.c` | 13, 27, 107, 111, 115, 116, 120, 123 | The sole resolution rule; resolved-value comparison for regeneration scope; staged Project/Storey mutation values and public setter implementations. |
| `sitehelper/project/sitehelper_project.c` | 17 | Initial stored Project default (2400 mm). No live mutation or generation. |
| `sitehelper/project/sitehelper_project.h` | 80, 81, 82, 83 | Public mutation API declarations and parameter names. |
| `sitehelper/wall/wall_generation.c` | 32, 60, 129, 203 | Resolved-settings consumer: positivity guards, top-plate Z and full stud length. |
| `sitehelper/wall/wall_members.c` | 107 | Resolved-settings consumer: header vertical limit. |
| `sitehelper/wall/wall_noggins.c` | 23, 33, 43 | Resolved-settings consumer: positivity, row count and row placement with widened intermediate arithmetic. |
| `sitehelper/wall/wall_openings.c` | 89, 774 | Resolved-settings consumer: opening vertical limit and upper-cripple length. |
| `sitehelper/wall/wall_studs.c` | 18 | Resolved-settings consumer: full stud length. |

No `stud_height` occurrence remains unclassified. Model/Wall code has no Project
or Storey lookup dependency. Settings changes do not touch topology or derive
heights from elevations. All direct live settings mutation call sites were
replaced; the remaining direct field-write capability is the documented public-C
struct limitation, not an application bypass.

## Complete file inventory

27 files modified, 4 added, none removed. Paths are repository-relative.

Added:

- `sitehelper/project/SETTINGS.md`
- `sitehelper/project/project_settings.c`
- `sitehelper/project/project_settings_internal.h`
- `sitehelper/project/tests/test_project_settings.c`

Modified:

- `sitehelper/CMakeLists.txt`
- `sitehelper/app/sitehelper_sdl.c`
- `sitehelper/cli/actions.c`
- `sitehelper/command/delete_wall_command.c`
- `sitehelper/command/move_wall_endpoint_command.c`
- `sitehelper/command/opening_command.c`
- `sitehelper/command/sitehelper_command.c`
- `sitehelper/command/tests/test_move_wall_endpoint_command.c`
- `sitehelper/command/wall_command.c`
- `sitehelper/editor/editor_action.c`
- `sitehelper/editor/sitehelper_editor.c`
- `sitehelper/editor/sitehelper_editor.h`
- `sitehelper/model/build_settings.c`
- `sitehelper/model/build_settings.h`
- `sitehelper/model/storey.h`
- `sitehelper/persistence/sitehelper_persistence.c`
- `sitehelper/persistence/sitehelper_persistence.h`
- `sitehelper/persistence/tests/test_sitehelper_persistence.c`
- `sitehelper/project/README.md`
- `sitehelper/project/project_validation.c`
- `sitehelper/project/sitehelper_project.h`
- `sitehelper/project/tests/test_storeys.c`
- `sitehelper/tests/test_sitehelper.c`
- `sitehelper/tests/test_support.h`
- `sitehelper/wall/tests/test_wall_render.c`
- `sitehelper/wall/wall.h`
- `sitehelper/wall/wall_noggins.c`
