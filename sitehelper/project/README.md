# Project and Storey ownership (Priority 9A)

This is the preserved Priority 9A implementation report. The subsequent
[Priority 9B settings ownership and resolution report](SETTINGS.md) describes
the current Storey override APIs and persistence v9. The current query ownership
policy is documented separately in [Priority 27 project query boundary](QUERY_LAYER.md).

`SiteHelperProject` owns project-wide `BuildSettings`, a global DomainIdGenerator,
and an ordered `Storey *storeys` array with count/capacity. It owns zero or more
Storeys. `Storey` holds `id`, signed `elevation_mm`, and one owned `BuildStructure`.
The local structure owns Rooms, Walls and RoomSeparators; each Wall owns its
openings and generated framing. Containment is the only Storey membership.
There are no redundant per-object Storey IDs, Room-to-Wall ownership, Buildings,
persistent topology or persistent junctions.

Elevation is the vertical offset of the Storey's reference plane from project
datum. Wall-local Z=0 is relative to that plane. Negative elevations, equal
elevations, empty Storeys and zero-Storey projects are valid. Elevation never
selects an ID, implies array order, or determines a wall height. No floor,
ceiling, slab or 3D geometry is implied; 2D plan and wall-local elevation rendering
retain their existing coordinate conventions.

## Lifecycle, identity and APIs

`sitehelper_project_init` still allocates nothing. Application startup explicitly
adds an elevation-zero Storey, checks failure and selects it in the editor.
`sitehelper_project_destroy` recursively frees every local structure and the
Storey array, then zeros the project. NULL and repeated destruction are safe.
Owning structs cannot be shallow-copied into another owner. Borrowed lookup
pointers must be reacquired after collection mutation or project replacement;
commands and editor navigation retain IDs, never those pointers.

Public APIs added:

- `sitehelper_project_add_storey(project, elevation_mm)` returns its fresh ID.
- `sitehelper_project_find_storey_by_id[_const](project, id)` finds a Storey.
- `sitehelper_project_find_owning_storey[_const](project, id)` resolves the owner
  of any nested object, including openings; a Storey ID resolves itself.
- `sitehelper_project_find_room_by_id[_const]`, `find_wall_by_id[_const]` and
  `find_room_separator_by_id[_const]` provide project-wide typed ID lookups.
- `sitehelper_project_find_room_with_owner_const` returns a Room and its borrowed
  owning Storey without inspecting physical geometry. RoomRegion uses this API;
  its own Storey traversal only validates Room collection metadata.
- `sitehelper_project_contains_domain_id` searches the entire namespace.
- `sitehelper_project_remove_wall_by_id` resolves ownership before removal.
- `sitehelper_project_insert_storey(project, id, elevation_mm)` appends an empty
  Storey with a supplied identity for loading/restoration. It does not advance
  the watermark; the caller must establish it before treating the project as
  valid. `sitehelper_project_insert_room_separator(project, storey_id, value,
  index)` likewise restores an existing identity into the specified Storey.

Creation APIs now require `storey_id`: `sitehelper_project_add_room`,
`sitehelper_project_add_wall`, and `sitehelper_project_add_room_separator`.
There is no default-first-Storey core mutation. Existing-object mutations retain
stable object IDs. Low-level `build_*` operations still concern one structure;
project-level operations additionally enforce the global identity boundary.
Queries assume coherent authoritative metadata, as the original build queries do.

Storeys, Rooms, Walls, Openings and RoomSeparators all share the same namespace,
including objects on different Storeys. Creation stages the allocator and commits
it only after successful insertion. Restoration checks all live identities.
The watermark must be nonzero and greater than every live ID. Allocating the
last representable ID would violate that invariant, so project creation rejects
exhaustion transactionally; the underlying allocator implementation is unchanged.

Validation is read-only and allocation-free. Its deterministic passes are
settings, Storey/nested collection metadata, global identities/watermark, then
local wall/opening/separator geometry. Each pass uses stored Storey order;
identity traversal visits Storey, Rooms, Walls and their Openings, then separators.
All nested metadata is checked before any global identity traversal. New errors
are `INVALID_STOREY_COLLECTION` and `INVALID_STOREY_ID`, prefixed
`SITEHELPER_PROJECT_`. Framing, elevation uniqueness and derived topology are
not integrity invariants.

## Commands and editor

`wall_command_create(storey_id, segment, output)` and
`add_room_separator_command_create(storey_id, segment, output)` capture the target
Storey in their command values. Add undo/redo use that Storey. DeleteWall snapshots
retain the owner's Storey ID plus the existing authoritative wall/opening copy;
separator deletion snapshots retain Storey ID and local stored index. Undo fails
if the owner is missing or an identity is occupied anywhere else. Existing-object
opening, endpoint, placement and separator commands use global stable-ID lookups.
History retains its exclusive undo-state ownership, commit ordering and retry
semantics. No Storey deletion, cascade, reparenting or Storey command is added.

`SiteHelperEditor.current_storey_id` is transient navigation. Use
`sitehelper_editor_set_current_storey(editor, project, id)` to switch; invalid ID
zero clears navigation, while a missing nonzero ID fails unchanged. Switching
clears current Room/Wall, selection, snaps and previews. Reconciliation checks all
navigation/selection against only the active Storey and clears missing owners.
`sitehelper_editor_primary_action_in_project` now accepts the project and resolves
the active local structure for hit testing and Wall Tool creation. App current
Room/Wall access and plan rendering are similarly local; identical XY Walls on
other Storeys cannot win selection or render. Elevation uses one current local
Wall. Plan snapping remains the existing grid policy; wall-member snapping uses
the resolved current Wall. There is no production RoomSeparator tool to rework;
its creation commands are now explicitly scoped. No Storey-management GUI is
introduced; both applications explicitly create/select an initial Storey.

## Persistence and spatial queries

Version 8 grammar retains the existing settings and inner definition records:

```text
sitehelper_project 8
domain_id_next N
settings ...
storeys COUNT
storey ID elevation MM
walls COUNT
wall ... openings COUNT
opening ...
rooms COUNT
room ... placement ...
end_room
room_separators COUNT
room_separator ...
end_storey
end_project
```

Every collection retains stored order and stable identities. Loading parses into
an owned candidate, validates the whole hierarchy and regenerates every Wall
using project settings before replacing the destination. Any failure destroys
the candidate and preserves the destination. Save validates before opening the
file. Generated framing and editor state are never serialized.

Versions 1–7 parse into one temporary, elevation-zero Storey. After parsing,
the old allocator watermark supplies a fresh Storey ID and advances once.
Collision, a stale watermark or exhaustion fails transactionally. Existing
Room/Wall/Opening/Separator IDs are never remapped. Empty legacy projects also
receive this Storey. Earlier wall-reference, placement and separator migration
rules remain intact. Version 8 can represent zero Storeys without synthesizing
one on load.

`plan_topology_build_from_storey` replaces `plan_topology_build_from_project`.
The generic exact engine is unchanged. RoomRegion convenience resolution builds
only its Room's Storey; supplied snapshots have an explicit same-Storey caller
contract. WallJunctionSet consumes that local topology unchanged. There are no
cross-Storey relationships or persisted topology identifiers.

## Settings direction for Priority 9B

Priority 9A keeps one authoritative `project.settings.stud_height` and all other
BuildSettings project-wide. Generation and opening validation are unchanged.
Do not infer wall height from the next Storey's elevation or add another
Storey height while generators continue to read the project value.

The future ownership distinction is:

1. Application/company defaults initialize new projects only.
2. Project settings are authoritative defaults within that project.
3. Storey settings express level-specific assumptions.
4. Wall settings hold physical-wall-specific overrides.
5. Opening settings hold opening-specific overrides.

For 9B, first define whether the existing `stud_height` means stud cut length,
wall framing height, or reference-plane-to-top-plate height. Its current use spans
stud generation, plate Z, noggin layout, opening-height validation and regeneration,
so renaming or moving it without agreeing on plate allowances would be ambiguous.
Recommend a project default plus an optional Storey override for framing height,
resolved once into a transient Wall generation configuration. Pass the resolved
value consistently to generation and opening validation, retaining wall-local Z
relative to the owning Storey. Migrate old values to the project default with no
overrides, preserving existing output. Persist only the defaults/explicit
overrides, never a second competing effective height. Settings-change commands
must validate affected openings and regenerate all affected Walls transactionally.
Floor-to-floor elevation differences remain independent of framing height.
Do not add Wall height overrides in 9B without a concrete requirement for walls
of different heights on the same Storey; such a later override must participate
in the same resolver, rather than become a competing source of truth.
A generic inheritance framework is unnecessary until concrete settings need it.

Likely long-term scopes of the remaining current fields:

| Fields | Scope and resolution |
| --- | --- |
| `stud_depth`, `stud_width` | Project defaults for a Wall framing specification; explicit Wall/type choices when different framing is needed. Storey is not intrinsically the material owner. |
| `stud_spacing`, `stud_spacing_mode`, `nog_spacing` | Project layout defaults resolved for each Wall, with Wall/type-specific choices when required. Add Storey overrides only for a demonstrated level-wide requirement. |
| `opening_width_allowance`, `opening_height_allowance` | Project opening defaults with the existing explicit Opening custom allowances; resolve consistently for validation and framing. No automatic Storey override is needed. |

For 9A all eight fields remain solely project-owned. Storey adds only elevation,
identity and containment; no settings inheritance or Building scope is implemented.

## Final verification audit — 2026-09-11

The interrupted implementation was recovered from the working tree and saved
logs, then verified with fresh Debug configurations in separate build directories.
After the final code/test corrections, all three builds and full CTest suites
were rerun successfully. Totals below count CTest executables, not assertions.

| Configuration | Build directory | Full suite |
| --- | --- | --- |
| Normal, topology enabled | `/tmp/p9a-final-normal` | 49/49 passed; 0 failures |
| ASan + UBSan, topology enabled | `/tmp/p9a-final-sanitize` | 49/49 passed; 0 failures; no sanitizer or leak reports |
| `SITEHELPER_BUILD_TOPOLOGY=OFF` | `/tmp/p9a-final-no-topology` | 46/46 passed; 0 failures |

All configurations used `cmake -S . -B DIRECTORY -DCMAKE_BUILD_TYPE=Debug`,
`cmake --build DIRECTORY -j 4`, and `ctest --test-dir DIRECTORY --output-on-failure`.
The disabled build additionally passed `-DSITEHELPER_BUILD_TOPOLOGY=OFF`.
Sanitizer configuration additionally passed:

```text
-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie
-DCMAKE_EXE_LINKER_FLAGS=-no-pie
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

The environment variables applied to the full sanitizer CTest run. Configure,
build, final rebuild and test logs remain under `/tmp/p9a-final-*.log`.
All 40 changed C translation units also passed a separate syntax/diagnostic check
using their normal compilation database flags plus
`-Wall -Wextra -Wpedantic -Werror`; results are in
`/tmp/p9a-final-warnings.log`. No compiler warnings occurred in the builds.
SDL configuration reports the environment's missing optional libdecor development
library for GNOME/Weston decorations; this is not a compiler diagnostic.
`git diff --check` passes, including `sitehelper_project.c`.

Coverage reviewed and retained includes multi-Storey lifecycle, signed/equal
elevations, global IDs across all entity types, project-wide owner lookup,
allocation/exhaustion/collision transactions, independent identical-XY topology,
RoomRegion and WallJunction isolation, Storey-specific Wall/Separator history,
DeleteWall restoration after Storey collection reallocation, active-Storey
rendering and selection, and v8 plus every v1–7 migration grammar.

The final audit added Room-and-owner lookup coverage, reversed Opening and
RoomSeparator order round-trips, v8 Storey watermark failures, collision/exhaustion
and truncated-load checks across every legacy version, explicit snap/opening
preview clearing on switching, actual Wall Tool command ownership after Storey
reallocation, stale wall-elevation navigation, and zero-Storey editor rendering
and selection. RoomRegion also proves that malformed Walls on another Storey do
not affect the queried Room. Existing persistence fixtures now use explicit
rollback values or deep authoritative clones rather than independent shallow
Project ownership snapshots. The remaining top-level byte comparison in the
validation test is explicitly borrowed and never destroyed as an owner.

Allocation fault injection checks 5 core creation failures (including the empty
Project), 30 v8 load failures, 18 v7 migration failures, and 22 history failures.
Each failed operation preserves authoritative state and allocator/history state;
load and history tests also retry successfully. Existing command and persistence
suites continue to exercise regeneration and malformed-input transactions.

The architecture review found no production `project.structure` or
`project->structure` references. The sole production `storeys[0]` access assigns
the synthesized legacy Storey's ID after parsing. Other production Storey-array
traversals belong to project lookup/lifecycle/validation, persistence, or
RoomRegion's explicitly Room-only metadata validation. No Storey-specific logic
was introduced into Wall generation, and no raw collection pointers are retained
by commands or editor navigation. Topology/junction state stays derived.

No known Priority 9A correctness issue remains. The supplied-topology RoomRegion
API deliberately relies on its documented same-Storey/freshness caller contract;
the convenience API resolves the owner automatically. SDL/editor wiring was
verified through build, source audit and automated app/editor tests; no manual
interactive GUI session was performed. There is no RoomSeparator GUI tool or
Storey-management GUI in this priority. Priority 9B remains a recommendation only.
No commit was created.

## Complete changed-file inventory

51 existing files modified, 3 files added, no files removed. Paths below are
relative to the repository root.

Added:

- `sitehelper/model/storey.h`
- `sitehelper/project/README.md`
- `sitehelper/project/tests/test_storeys.c`

Modified:

- `sitehelper/CMakeLists.txt`
- `sitehelper/app/app_view.c`
- `sitehelper/app/appstate.c`
- `sitehelper/app/main.c`
- `sitehelper/app/sitehelper_sdl.c`
- `sitehelper/app/tests/test_app_view.c`
- `sitehelper/app/tests/test_appstate.c`
- `sitehelper/cli/actions.c`
- `sitehelper/command/delete_wall_command.c`
- `sitehelper/command/delete_wall_command_internal.h`
- `sitehelper/command/move_wall_endpoint_command.c`
- `sitehelper/command/opening_command.c`
- `sitehelper/command/room_separator_command.c`
- `sitehelper/command/room_separator_command.h`
- `sitehelper/command/sitehelper_command.c`
- `sitehelper/command/tests/test_command_history.c`
- `sitehelper/command/tests/test_command_invariants.c`
- `sitehelper/command/tests/test_delete_wall_command.c`
- `sitehelper/command/tests/test_move_wall_endpoint_command.c`
- `sitehelper/command/tests/test_opening_command.c`
- `sitehelper/command/tests/test_room_location_command.c`
- `sitehelper/command/tests/test_room_separator_command.c`
- `sitehelper/command/tests/test_sitehelper_command.c`
- `sitehelper/command/tests/test_wall_command.c`
- `sitehelper/command/wall_command.c`
- `sitehelper/command/wall_command.h`
- `sitehelper/editor/sitehelper_editor.c`
- `sitehelper/editor/sitehelper_editor.h`
- `sitehelper/editor/tests/test_multiwall_editor.c`
- `sitehelper/editor/tests/test_sitehelper_editor.c`
- `sitehelper/model/build_structure.h`
- `sitehelper/persistence/sitehelper_persistence.c`
- `sitehelper/persistence/sitehelper_persistence.h`
- `sitehelper/persistence/tests/test_sitehelper_persistence.c`
- `sitehelper/project/project_validation.c`
- `sitehelper/project/room_separator.c`
- `sitehelper/project/sitehelper_project.c`
- `sitehelper/project/sitehelper_project.h`
- `sitehelper/project/tests/test_project_validation.c`
- `sitehelper/project/tests/test_room_location.c`
- `sitehelper/project/tests/test_room_separator.c`
- `sitehelper/project/tests/test_sitehelper_project.c`
- `sitehelper/tests/test_support.h`
- `sitehelper/topology/README.md`
- `sitehelper/topology/plan_topology.h`
- `sitehelper/topology/project_topology.c`
- `sitehelper/topology/room_region.c`
- `sitehelper/topology/room_region.h`
- `sitehelper/topology/tests/test_plan_topology.c`
- `sitehelper/topology/tests/test_room_region.c`
- `sitehelper/topology/tests/test_wall_junctions.c`


Priority 26G1 adds Storey-owned `RoofCollection` authority. Roofs and roof portions use the same project-global DomainId namespace; compositions/terminations are roof-owned value relationships. Derived roof geometry is regenerated and is not project authority. Persistence remains v14 and refuses roof-bearing saves until 26G3.


## Priority 28A — document / annotation authority

`SiteHelperProject` now also owns a `DocumentModel` beside the Storey hierarchy.
This is deliberate non-physical authority: structural Storeys/Walls/Rooms/Slabs/
Roofs do not gain note or drafting fields. The first concrete annotation is a
Storey-plan note with a stable global ID, integer-mm plan anchor, text and an
optional weak target ID. Live target creation must resolve to physical authority
on the same Storey; after target deletion the stable ID may remain unresolved so
undo/restoration can re-associate it without cross-domain mutation. Annotation
IDs participate in the global identity namespace but annotations have no owning
Storey. See `../document/README.md` and `QUERY_LAYER.md`.

Persistence format 16 stores the project annotation section after all Storeys.
Versions 1–15 load with an empty document model. Priority 28B adds transactional
plan-note updates plus a history-restoration replacement primitive; rendering,
selection and command/history integration live above Project and keep using the
annotation's explicit Storey anchor. Dimensions, symbols, revision clouds, paper
sheets and production style systems remain deferred.

## Priority 28G2 — revision lifecycle grouping

`DocumentRevision` is project-level non-physical metadata with a stable global
ID, human-facing identifier and optional description. It has no Storey owner and
is not a Plan-selectable `DocumentObjectRef`. Revision clouds may carry an
optional weak `revision_id`, allowing one revision record to group clouds across
Storeys while geometry remains independently authored. Explicit assignment
requires a live revision; deletion leaves the weak ID intact so undo/restoration
reconnects automatically. Persistence v22 stores revision records before the
revision-cloud collection; v21 clouds load unassigned.
