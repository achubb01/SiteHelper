Priority 10 establishes the value snapshot and typed command boundary for the future properties panel. It adds no authoritative persisted data and leaves persistence version 9 unchanged. No commit was created.

1. **Architecture found before implementation.** `SiteHelperProject` owns Storeys; each Storey owns independent Room, Wall and room-separator collections. A Wall owns its stable ID, ordered plan segment, authoritative Opening definitions and separately generated framing. Openings own stable IDs but live in their Wall's reallocatable collection. Wall length is rounded to whole millimetres from the ordered segment. Project settings provide construction defaults; the only Storey construction override is stud height. Live operations resolve the owning Storey's complete `BuildSettings` before entering Wall code.

   `EditorSelection` previously represented only generated members, using a Wall ID and a copied `WallSelection`/Timber value. Member reconciliation matches geometry and type rather than allocating persistent member IDs. Plan selection only changed `current_wall_id`. Navigation, view, tools and previews are transient editor state. `appstate` resolves navigation on demand; `app_view` renders plan navigation highlighting or selected elevation members. GUI infrastructure supplies buttons, layout and coloured rectangles, with no text-entry or font system.

   Existing endpoint commands call `wall_apply_plan_segment`, which validates each opening against its predecessors and generates candidate framing before commit. Add-opening commands already stage definitions, framing and ID allocation. History reserves storage and captures owned undo state before executing; failed operations do not advance its cursor or discard redo entries. SDL already reconciled undo/redo; ordinary successful command execution now also reconciles after completing the editor action.

2. **Selection representation.** `EditorSelectionKind` now distinguishes `NONE`, `WALL`, `OPENING` and the existing `WALL_MEMBER`. The struct stores `wall_id`, `opening_id` (only meaningful for OPENING), and the existing by-value `WallSelection`. Setters clear irrelevant state. It contains no persistent collection pointers. `editor_selection_set_wall` and `editor_selection_set_opening` do not alter navigation. Plan Select sets navigation and authoritative Wall selection as separate operations, retaining the existing nearest-wall/tie policy.

   `wall_find_opening_at_position` returns a stable opening ID from its authoritative clear framed U/Z rectangle, using `opening_frame_width`/`opening_frame_height` with effective allowances. It requires no framing. The project-aware elevation Select path keeps existing member hit precedence, then selects an opening in clear space. The lower-level primary action has no resolved settings parameter and retains member-only elevation selection. Nonfinite/unrepresentable elevation coordinates are rejected before conversion to integers.

3. **Inspector projection.** `sitehelper_editor_inspect_properties(editor, project, output)` in `editor_properties.h` returns a caller-owned `EditorProperties` tagged value. `EditorWallProperties` contains `wall_id`, ordered `segment`, derived `length_mm`, `resolved_stud_height` and `resolved_stud_spacing`. `EditorOpeningProperties` contains owner `wall_id` and a complete by-value `Opening definition`, including its ID, type, U, Z, dimensions, custom flag and stored allowances. Header comments identify editable and read-only fields.

   Resolution is scoped to `current_storey_id`, independent of `current_wall_id`. Every request reads current project definitions/settings; editor state stores no property cache. NONE, member selection, missing IDs and invalid context return zero with a cleared snapshot. An opening snapshot reports stored allowance values even when custom allowances are disabled; hit testing and generation use effective allowances instead.

4. **Property intents.** `sitehelper_editor_create_property_command(editor, project, edit, output)` takes `EditorPropertyEdit`, a small field enum plus a typed union of integer millimetres, `OpeningType` or boolean custom-allowance state. It projects the selected object, copies its current definition, and replaces exactly the requested component. Wall edits produce `MoveWallEndpointCommand` with the other coordinate preserved. Opening edits produce `EditOpeningCommand` containing the full proposed definition. Failed intent creation clears the output command. There are no string keys, parsing, reflection or domain geometry checks here.

   The application consumes the API through its existing history boundary:

   ```c
   EditorPropertyEdit edit = {
       .property = EDITOR_PROPERTY_OPENING_WIDTH,
       .value.millimetres = 1200
   };
   SiteHelperCommand command;
   SiteHelperCommandResult result;
   if (sitehelper_editor_create_property_command(editor, project, &edit, &command) &&
       sitehelper_command_history_execute(history, project, &command, &result)) {
       sitehelper_editor_reconcile(editor, project);
   }
   ```

   Future controls should request fresh snapshots for display, parse text into typed values, build an intent at acceptance time, execute through history and reconcile after success. No GUI access to mutable Wall/Opening/settings structures is needed.

5. **Opening command and Wall contract.** `EditOpeningCommand` stores owning Wall ID, Opening ID and a complete proposed `Opening`. Its constructor checks identity consistency only. `edit_opening_command_execute` resolves the Wall, its owning Storey and effective settings, then calls `wall_apply_opening_definition`. Public command results contain IDs only. No project ID is allocated or advanced.

   `wall_apply_opening_definition(wall, settings, opening_id, definition)` requires the replacement ID to match the target. It copies the ordered opening collection and substitutes the target at the same index. The existing `wall_apply_plan_segment` transaction validates that candidate's unchanged segment and all proposed definitions with `wall_validate_opening`, comparing each pair once. This checks both earlier and later openings and never compares an opening against itself. Generation uses the same effective settings. Definition and framing commit together after success. Borrowing the existing Opening as the input is safe.

6. **Transactional failure.** All candidate opening storage and framing are independently owned. No live definition, framing, collection allocation or allocator state is changed while validation/allocation/generation can fail. Failed staging destroys only the candidate. Success swaps in the candidate Wall and destroys the previous owned allocations; no fallible step remains after commit begins. History reserves its entry storage and captures the old definition before invoking the command. A failed execute may grow history capacity, as before, but preserves entries, count, cursor and redo branch. Failed undo/redo preserve all project state and the history cursor.

   Geometry rules remain in `wall_validate_opening` and the existing generator. Project resolves configuration; commands resolve identity/ownership and dispatch; editor maps typed intent; GUI will parse/display. No new geometry rule is duplicated across those layers. Existing structural ID checks in constructors and mutation APIs remain defensive identity checks, not competing geometry validators.

7. **Undo/redo.** The private history undo-state union stores an `EditOpeningCommand` with the previous complete Opening by value. Undo executes that prior definition through `edit_opening_command_execute`; redo executes the originally proposed definition through the same function. Both preserve the original ID and collection index. The entire previous type, U, Z, width, height, custom state and stored allowances are restored. Framing is regenerated deterministically with the owning Storey's settings at undo/redo time. If settings have changed incompatibly, the operation fails without moving the cursor. Compact public undo intentionally cannot reverse this command without history-owned state.

8. **Editable properties.** Wall start X/Y and end X/Y; Opening type, frame position U, frame bottom Z, width, height, custom allowance flag, width allowance and height allowance. Identity and ownership cannot be changed. The lower-level edit-opening command can replace several definition fields atomically; the property intent changes one field per command.

9. **Read-only properties.** Wall length is derived from its ordered endpoints; editing it needs a future anchoring/direction policy. Stud height is resolved from Project/Storey configuration, and stud spacing belongs to Project construction settings. No implicit per-wall overrides, stored length or wall-type concept were introduced. No new persisted fields or persistence changes were needed.

10. **Reconciliation.** Project reconciliation resolves the selected Wall within the active Storey, then resolves an Opening within that explicit owner or matches a generated member by value. Missing owners/openings and unknown selection kinds clear selection. Regeneration alone leaves existing Wall/Opening IDs selected even if all framing allocations change. Missing generated members still clear. Storey switching clears navigation/selection/previews as before; view switching retains its existing selection-clear policy. Removal followed by undo does not resurrect cleared selection. Snapshot lookup independently rejects stale IDs even before reconciliation. Ordinary SDL command success now runs reconciliation, as undo/redo already did.

11. **Files added/modified.** Paths below are relative to `sitehelper/`.

| File | Change |
| --- | --- |
| `command/edit_opening_command.h` | Added ID-addressed full-definition command contract. |
| `command/edit_opening_command.c` | Added owning-Storey resolution and Wall dispatch. |
| `command/tests/test_edit_opening_command.c` | Added domain/command/history transaction tests. |
| `editor/editor_properties.h` | Added snapshot and typed property-intent contracts. |
| `editor/editor_properties.c` | Added fresh projection and command construction. |
| `editor/tests/test_editor_properties.c` | Added selection, snapshot, intent and hit-testing tests. |
| `editor/PROPERTIES.md` | Added architecture, integration and verification report. |
| `CMakeLists.txt` | Registered new sources, tests and allocation-failure wrappers. |
| `command/sitehelper_command.h` | Added command tag, value payload, ID result and wrapper declaration. |
| `command/sitehelper_command.c` | Added execute/redo dispatch and history-owned old definition. |
| `editor/editor_selection.h` | Added authoritative selection kinds and IDs/setters. |
| `editor/editor_selection.c` | Added setters and complete clearing of irrelevant state. |
| `editor/sitehelper_editor.h` | Documented project-aware selection integration. |
| `editor/sitehelper_editor.c` | Added authoritative reconciliation and plan/elevation selection integration. |
| `editor/tests/test_multiwall_editor.c` | Updated plan-selection assertions to require Wall IDs. |
| `wall/wall.h` | Declared transactional opening replacement. |
| `wall/wall_openings.c` | Added candidate-definition replacement and atomic commit. |
| `wall/wall_query.h` | Declared authoritative opening hit query. |
| `wall/wall_query.c` | Added effective-rectangle stable-ID hit query. |
| `app/sitehelper_sdl.c` | Reconciles after ordinary command success. |

12. **Tests added.** Two CTest executables extend the existing assert-based conventions and shared semantic comparison helpers.

| Test function | Coverage |
| --- | --- |
| `test_complete_definition_history` | Combined edit, complete old/new definition and framing, repeated undo/redo, ID results and invalid public result. |
| `test_invalid_edits_and_redo_branch` | Overlap against preceding/following entries, wall bounds, excessive height, invalid dimensions/type/bottom/allowances, integer overflow, generator-only failure, unchanged definitions/framing/pointers and retained redo branch. |
| `test_identity_and_domain_contract` | Missing/invalid ownership and IDs, identity mismatch, null inputs, aliased source, same-definition self-exclusion, Storey-specific height. |
| `test_failed_undo_redo_use_current_settings` | Valid live Storey settings changes that make undo/redo invalid, preserved cursor/model/framing and successful retry. |
| `test_history_storage_and_branch_replacement` | Twelve edits across history reallocation, complete reversal/replay and successful redo-branch replacement. |
| `test_all_allocation_failures` | Every allocation failure in execute/undo/redo, unchanged model/framing allocations and history state, successful retry. |
| `test_snapshots_are_fresh_and_navigation_independent` | Ordered/reversed diagonal endpoints, derived length, Storey override/spacing, fresh settings, complete Opening snapshot and output isolation. |
| `test_selection_relocation_regeneration_and_stale_ids` | Storey/Wall/Opening collection growth, framing regeneration, stable selection, missing opening/owner, cleared snapshots before reconciliation. |
| `test_context_and_member_selection` | Member-by-value behavior and reconciliation, Storey/view context clearing, cross-Storey rejection and invalid selection IDs. |
| `test_wall_component_intents_and_history` | All endpoint components with untouched coordinate, mutation-free construction, history execution and geometry/framing undo/redo. |
| `test_each_opening_property_intent` | Every editable field independently, full untouched-field preservation, fresh post-edit snapshot, ID selection through regeneration, history undo and domain-only invalid-value rejection. |
| `test_opening_hit_testing_and_select_tool` | Custom/default allowances, query without framing, geometric misses, project-aware Select, member precedence, invalid pointer coordinates and Plan Wall selection. |

   Existing `test_positioned_walls_select_by_stable_identity` now requires authoritative Wall selection through collection growth. All other pre-existing tests remain intact, including the low-level null-Wall primary-action behavior.

13. **Verification.** Exact final commands and results are recorded below. Configuration/build/test logs use `/tmp/p10-{normal,sanitize,off}-*.log`; diagnostic audit uses `/tmp/p10-warnings.log`.

| Workflow | Result |
| --- | --- |
| Normal Debug, topology enabled | 52/52 CTest tests passed; 0 failures. |
| ASan + UBSan with leak detection | 52/52 passed; 0 failures; no sanitizer/leak diagnostics. |
| Topology disabled | 49/49 passed; 0 failures. |
| Strict diagnostics | 119/119 first-party C translation units passed. |
| Allocation-failure sweep | 14 execute, 14 undo, 12 redo failure positions; all preserved state and allowed retry. |
| `git diff --check` | Passed. |

Commands run from the repository root (the build commands below are the final incremental builds after initial complete builds):

```sh
cmake -S . -B /tmp/p10-normal -DCMAKE_BUILD_TYPE=Debug > /tmp/p10-normal-configure.log 2>&1
cmake --build /tmp/p10-normal -j 4 > /tmp/p10-normal-rebuild.log 2>&1
ctest --test-dir /tmp/p10-normal --output-on-failure > /tmp/p10-normal-tests.log 2>&1

cmake -S . -B /tmp/p10-sanitize -DCMAKE_BUILD_TYPE=Debug \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS=-no-pie > /tmp/p10-sanitize-configure.log 2>&1
cmake --build /tmp/p10-sanitize -j 4 > /tmp/p10-sanitize-rebuild.log 2>&1
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --test-dir /tmp/p10-sanitize --output-on-failure > /tmp/p10-sanitize-tests.log 2>&1

cmake -S . -B /tmp/p10-no-topology -DCMAKE_BUILD_TYPE=Debug \
  -DSITEHELPER_BUILD_TOPOLOGY=OFF > /tmp/p10-off-configure.log 2>&1
cmake --build /tmp/p10-no-topology -j 4 > /tmp/p10-off-rebuild.log 2>&1
ctest --test-dir /tmp/p10-no-topology --output-on-failure > /tmp/p10-off-tests.log 2>&1

/tmp/p10-normal/sitehelper/test_edit_opening_command
python3 /tmp/p10-audit.py > /tmp/p10-warnings.log 2>&1
git diff --check
```

The strict audit script reads `/tmp/p10-normal/compile_commands.json`, skips external dependencies, removes `-c`/object output arguments and invokes each compiler command with `-fsyntax-only -Wall -Wextra -Wpedantic -Werror`. No warnings were suppressed. Initial full-build logs are `/tmp/p10-{normal,sanitize,off}-build.log`; final incremental logs are the `rebuild.log` files above. No compiler warnings occurred. SDL configuration reports the existing optional libdecor dependency warning. The first normal test run caught an unnecessary change to the low-level null-Wall selection contract; that change was reverted and all final runs pass. No interactive SDL session was run; selection integration is tested through the project-aware editor API.

14. **Later priorities and existing limitations.** Build actual property controls, parsing, validation feedback and opening-selection highlighting on these APIs. The current rectangle-only properties panel remains visually unchanged; elevation opening selection is functional but has no dedicated highlight yet. Decide an explicit anchoring policy before making length editable. Settings commands/history belong to a future configuration-editing priority; existing live settings setters still operate outside history.

   Existing door framing uses a floor-based trimmer height even though `frame_bottom` is stored and used by opening validation. The hit query consistently uses the authoritative U/Z rectangle; it does not infer geometry from Timber. A future door-domain policy should align bottom semantics and door generation without silently rewriting definitions here. Existing window generation can reject proposals that pass `wall_validate_opening` (for example a zero-bottom window cannot generate lower cripples). This operation correctly rolls back those generation failures, but future work should improve preview/error diagnostics and align validation with generator capabilities in the Wall subsystem. Existing add-opening command code also stages candidate definitions locally; a later internal consolidation could share Wall transaction mechanics, while retaining atomic Project ID allocation. None of these issues requires another validation implementation in the GUI, editor or command layer.
