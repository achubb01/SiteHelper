Priority 11 adds transient text/numeric entry and exact Wall length placement. It adds no persisted data or save-format changes and creates no git commit.

1. **Inspection and baseline.** Before editing, inspected the SDL event translator and tests; renderer core/backend/SDL implementation; application event loop, views and viewport input; GUI controls and target boundaries; editor selection, snapping, properties and `editor/PROPERTIES.md`; Wall tool/model representation; Wall commands/history/completion/reconciliation; and current tests. Native SDL events previously became platform-neutral key/mouse events, with keyboard shortcuts handled directly in `sitehelper_sdl.c`. Both mouse placement and the new workflow produce typed `EditorAction` values. Commands/history retain sole responsibility for Project mutation.

   The existing Debug build succeeded and all **54/54 baseline tests passed** before changes. Pre-existing changes in `WINDOWS_BUILD.md` and the SDL submodule were left untouched.

2. **Ownership and targets.** `sitehelper_gui` now contains two independent reusable helpers: `TextEdit` and `length_parse_mm`. They use no SDL, platform events, Project, editor or Wall types. There is no new library target or dependency. `sitehelper_app_core` contains `AppInput` (focus, session lifecycle, parsing, event routing and typed intent connection) and a separate HUD presentation helper. Its added `platform_event` dependency is a header-only interface, not the SDL adapter. CLI/domain libraries do not acquire an SDL dependency.

   `AppKeyboardFocus` explicitly distinguishes no owner from tool-length ownership. `app_input_route` routes focused events before decoding global shortcuts, returning an `AppInputResult` plus an optional typed `EditorAction`. Future property owners can extend this controller without changing the SDL event loop. No keyboard events or strings enter WallTool/editor intent APIs, command/history, Project or model state.

3. **Platform and SDL adaptation.** `PLATFORM_EVENT_TEXT_INPUT` owns a 256-byte UTF-8 payload including NUL. Oversize native events set an explicit overflow flag and carry no truncated text. `platform_event_sdl_translate` copies `SDL_EVENT_TEXT_INPUT`, preserving payload lifetime independently of SDL. Enter (including keypad Enter), Escape, Backspace, Delete, Home and End join the existing arrows/Tab/Z/Y semantic keys. No text is reconstructed from keycodes.

   `renderer2d_sdl_set_text_input(backend, enabled)` is the narrow window adapter: that backend already owns the native window, so application/platform-neutral headers need no SDL window types or handles. It idempotently starts/stops text input and reports failure. Application synchronization runs before polling each next event, including after events that use `continue`. SDL input is enabled as soon as Wall placement has a start, allowing the first character to arrive, but keyboard focus is acquired only on committed text. It remains enabled after cancelling just the entry and stops after placement completion/cancellation, pointer leave, tool/view changes or reconciliation. Destruction also stops it. Activation failure produces a visible message.

   This follows SDL's explicit activation requirement ([SDL_StartTextInput documentation](https://wiki.libsdl.org/SDL3/SDL_StartTextInput)). Committed text is supported; IME preedit display, selection, clipboard integration and text-local undo are later work.

4. **Reusable buffer.** `TextEdit` has a fixed 128-byte capacity including NUL, an explicit active flag, byte length and cursor. `text_edit_begin`, `text_edit_clear`, `text_edit_end`, `text_edit_insert` and `text_edit_apply` cover lifecycle, atomic insertion, deletion and movement. Insertion validates UTF-8 scalar encodings, rejects controls and returns OK/FULL/INVALID. Cursor movement and deletion preserve UTF-8 code point boundaries. This is a small editing buffer, not a grapheme/IME/widget framework. Failed insertions preserve the existing buffer and cursor. The application owns the buffer through `AppInput`; it is never persisted.

5. **Parser and units.** `length_parse_mm(text, &millimetres)` returns `LENGTH_PARSE_OK`, `EMPTY`, `INVALID`, `OVERFLOW` or `FRACTIONAL_MM`. Output remains unchanged on failure. Accepted grammar is a signed decimal with optional lowercase `mm` or `m`, ASCII whitespace around the input and optionally between number and unit. A fractional part requires digits after the dot; `.001m` is accepted, `4.` remains incomplete/invalid. Bare numbers mean millimetres. For example, `4200`, `4200mm`, `4.2m`, `4.2000 m` and `4200.000mm` all give 4200.

   Conversion uses decimal digit arithmetic with checked accumulation and exact removal of fractional trailing zeros, never `atoi`, binary floating-point conversion or partial-string parsing. The result must fit signed `int` millimetres. `0.1mm` and `0.0001m` fail as fractional millimetres; malformed suffixes, exponents, commas, imperial units, NaN and infinity are invalid. Zero and negative integers are valid parsed values for reuse by coordinate/property fields; WallTool rejects nonpositive lengths. Units do not cross the typed editor boundary.

6. **Wall tool and editor APIs.** `WallTool.endpoint` retains the normal snapped mouse endpoint. A separate unconstrained `pointer` supplies numeric direction; editor pointer motion updates it from raw view coordinates after updating the normal snap preview. Optional `length_mm` is a transient constraint, cleared on begin/cancel. It is not an authoritative Wall property. Normal `wall_tool_command_data` returns the mouse segment without a constraint and resolves the exact segment with a constraint.

   The added tool APIs are `wall_tool_update_direction`, `wall_tool_clear_length`, `wall_tool_set_length` and pure `wall_tool_resolve_length`. `WallLengthStatus` distinguishes OK, INACTIVE, NONPOSITIVE, DIRECTIONLESS and OUT_OF_RANGE. A positive preview constraint may remain active while direction is invalid, so moving the pointer can resolve it without retyping. Resolution failure leaves the output segment untouched.

   The application uses `sitehelper_editor_clear_wall_length`, `sitehelper_editor_set_wall_length`, `sitehelper_editor_create_wall_length_action` and `sitehelper_editor_cancel_wall_placement`. Preview/commit receive only typed integer millimetres. Failed action construction clears its output. A shared private `editor_wall_action` helper builds the ordinary `WallCommand` and `SITEHELPER_COMMAND_ADD_WALL` action for both completion methods.

7. **Exact-length contract and strategy.** Successful numeric resolution guarantees:

   ```c
   wall_plan_segment_length_mm(segment) == requested_length_mm
   ```

   The start uses the existing mouse conversion to integer coordinates. Direction is normalized from raw pointer minus start, scaling by its largest component before `hypot`, which handles very large and subnormal finite directions safely. The ideal endpoint is that integer start plus normalized direction times requested length. The resolver examines the four floor/ceil corners surrounding this ideal point, filters out coordinates outside `int`, and uses the model length function to retain only exact matches. It selects the retained corner nearest the ideal endpoint, with deterministic iteration-order tie breaking.

   For unrestricted integer coordinates, the nearest-to-start and farthest-from-start corners bracket the ideal radius. A path along the cell edges connects them; each unit edge changes Euclidean distance by at most 1 mm. Such a path cannot skip the nearest-integer length bin for the requested integer radius. Thus a suitable corner exists, and the selected corner is at most approximately sqrt(2) mm from the ideal endpoint. Floating-point calculations only propose candidates; the existing model function is always the final acceptance gate. Numeric input therefore cannot succeed as 4199 or 4201 when 4200 was requested.

   Coordinate boundaries can remove the suitable nearby corners. In that case resolution fails visibly; it never clamps a coordinate or substitutes a very different direction. Nonfinite/unrepresentable starts, nonpositive length and zero direction fail without creating commands. No stored geometry representation, length derivation, coordinate convention or general constraint solver changes were necessary.

8. **User interaction and previews.** Activate Wall, click a start, move approximately along the desired direction, type `4200` (or `4.2m`) and press Enter. Valid text constrains only the transient preview. Movement continues to update direction. Malformed/incomplete text remains editable and restores the normal snapped preview. Valid text with an unusable direction/endpoint produces feedback and no constrained segment. Raw text never mutates Project state.

   Left/Right/Home/End move the cursor; Backspace/Delete edit; Enter attempts commit; Escape clears text/focus/constraint and restores the same in-progress mouse placement. Another Escape cancels that placement without switching tools. Key repeat edits normally, but repeated Enter/Escape do not repeatedly commit/cancel. Focus consumes all keys, including Tab, arrow-pan and Ctrl+Z/Y/Shift+Z; there is no text-local undo in this priority. Without focus, the existing global shortcut behavior is retained. While focused, viewport primary clicks do not commit a wall. Toolbar selection continues to work and invalidates the session through the existing editor lifecycle.

   Existing pointer-leave, resize/tool/view-change and undo/redo reconciliation conventions invalidate placement; refresh then drops numeric focus. Normal camera zoom/middle-button pan, snapping, selection and opening workflows retain their existing routing. Starting text capture after the first Wall point does not itself suppress keyboard shortcuts.

9. **History and failure.** Both mouse and numeric actions use the same application `sitehelper_app_execute_action`: execute `SiteHelperCommandHistory`, complete the editor action, reconcile editor state and refresh input. Numeric completion has no special mutation path. History stores only the ordinary endpoint-based add-wall command; undo/redo preserve those exact endpoints and derived length. Domain generation/validation remains in the existing Wall/command layers. If execution fails, the text and focus remain for correction/retry, with visible feedback and no consumed Project IDs/history entry. Escaping or merely typing also leaves Project and history untouched.

10. **Temporary HUD.** `renderer2d_draw_screen_text` forwards screen-space coordinates and colour to an optional backend callback without camera transforms. The SDL callback uses the bundled ASCII debug font; there is no external font dependency. `app_input_draw_hud` separately formats a clipped viewport HUD with text, cursor, validation colour and feedback. The visible value scrolls around the cursor when it exceeds the available columns. Valid entries are green; invalid/unresolved/failed entries are orange.

    SDL explicitly describes this font as debug-quality ([SDL_RenderDebugText documentation](https://wiki.libsdl.org/SDL3/SDL_RenderDebugText)). This is temporary typography for numeric entry only. Non-ASCII code points are displayed as `?` while preserved in the buffer and rejected by the numeric grammar. Font sizing/rendering and HUD formatting can be replaced without changing focus, parsing, typed intent or command semantics. No property panel or general typography system was introduced.

11. **Tests and verification.** Three CTest executables were added; existing tests were retained and extended:

| Test | Coverage |
| --- | --- |
| `platform_event_sdl_tests` | Text payload copy/lifetime, UTF-8, whole-event overflow, Enter/keypad Enter and editing keys/repeat, existing modifiers/mouse events. |
| `text_input_tests` (new) | Buffer insertion/deletion/navigation/boundaries/capacity, UTF-8 code point editing and malformed encoding rejection; valid units, signs, whitespace, fractional-mm rejection, malformed complete strings, signed integer limits and overflow. |
| `wall_tool_tests` | Axis/diagonal directions, lengths 1 through INT_MAX, over 31,000 direction/length combinations, exact model invariant and endpoint proximity, directionless/nonpositive/nonfinite/range failures, very large/subnormal direction normalization, unchanged mouse/cancel contracts. |
| `app_input_tests` (new) | Focus routing, raw direction independent of snapped preview, preview updates/restoration, two-stage Escape, nonmutating invalid/overflow text, normal add-wall action, command/history/completion/reconciliation, exact endpoint undo/redo, mouse placement, pointer/view lifecycle and restored global intents. |
| `renderer2d_tests` | Text forwards screen coordinates/colour unchanged despite camera state; optional/null backend handling. |
| `sdl_input_tests` (new) | Real application event loop under SDL dummy video/software rendering: toolbar activation, mouse start/direction, four separate SDL text events spelling 4200, HUD text/cursor/colour and actual backend rendering, exact commit, command failure preservation/retry, activation/deactivation, focus suppression/restoration, invalid input, Escape, mouse-only creation, history undo/redo, toolbar selection, view/opening-tool switching, wheel zoom and middle drag. |
| Existing suite | Selection, opening placement, snapping, toolbar/layout, view/camera state, commands/history, properties, Project/persistence/topology invariants. |

    The sanitizer run exposed an existing signed-overflow bug in the topology test's minimum-integer oracle: `-(2^126 + 2^126)` overflows before constructing the intended value. The test now constructs it as `-2^126 - 2^126`, with identical expected limbs and all assertions retained. No production topology code changed.

    Final results are recorded below. The SDL workflow was tested headlessly, not by a manual desktop session.

| Check | Result |
| --- | --- |
| Baseline Debug build + CTest | 54/54 passed before edits. |
| Final Debug build + complete CTest | 57/57 passed. |
| Final ASan + UBSan + leak detection, complete CTest | 57/57 passed; no sanitizer/leak diagnostics. |
| First-party strict compiler diagnostics | 129/129 translation units passed `-Wall -Wextra -Wpedantic -Werror`. |
| `git diff --check` | Passed. |

    Commands run from the repository root:

    ```sh
    cmake --build build -j 4
    ctest --test-dir build --output-on-failure

    cmake -S . -B /tmp/p11-sanitize -DCMAKE_BUILD_TYPE=Debug \
      '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
      -DCMAKE_EXE_LINKER_FLAGS=-no-pie
    cmake --build /tmp/p11-sanitize -j 4
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
      ctest --test-dir /tmp/p11-sanitize --output-on-failure

    python3 /tmp/p11-audit.py
    git diff --check
    ```

    Logs: `/tmp/sitehelper-baseline-{build,tests}.log`, `/tmp/p11-{build,tests}.log`, `/tmp/p11-sanitize-{configure,rebuild,tests}.log`, `/tmp/p11-warnings.log`. The strict audit reads `build/compile_commands.json`, skips external files, and invokes each first-party compilation with `-fsyntax-only` plus the warning flags above.

12. **Complete file inventory.** Paths are repository-relative. Only the following files were changed for Priority 11:

| File | Change |
| --- | --- |
| `platform_event/platform_event.h` | Owned neutral text event and semantic editing keys. |
| `platform_event/platform_event_sdl.c` | SDL text and editing-key translation. |
| `platform_event/test_platform_event_sdl.c` | Translation/lifetime/capacity tests. |
| `renderer2d/renderer2d.h` | Screen-text API. |
| `renderer2d/renderer2d.c` | Screen-text forwarding. |
| `renderer2d/renderer2d_backend.h` | Optional text callback. |
| `renderer2d/renderer2d_sdl.h` | Opaque-backend text activation API. |
| `renderer2d/renderer2d_sdl.c` | Isolated SDL text activation and temporary debug-font drawing. |
| `renderer2d/test_renderer2d.c` | Screen-text abstraction tests. |
| `sitehelper/CMakeLists.txt` | Register helper/controller/HUD sources, header-only platform dependency and three tests. |
| `sitehelper/gui/text_edit.h` (new) | Bounded reusable editing contract. |
| `sitehelper/gui/text_edit.c` (new) | Atomic UTF-8 insertion, cursor/deletion and lifecycle. |
| `sitehelper/gui/length_parse.h` (new) | Typed length parser/status contract. |
| `sitehelper/gui/length_parse.c` (new) | Exact decimal/unit conversion. |
| `sitehelper/gui/tests/test_text_input.c` (new) | Buffer/parser tests. |
| `sitehelper/app/app_input.h` (new) | Application focus/session/routing contract. |
| `sitehelper/app/app_input.c` (new) | Focus-first routing, parsing, preview and typed action connection. |
| `sitehelper/app/app_input_hud.h` (new) | Temporary HUD API. |
| `sitehelper/app/app_input_hud.c` (new) | Scrolling value, cursor and status presentation through renderer2d. |
| `sitehelper/app/sitehelper_sdl.c` | Input activation/routing, HUD integration and shared command execution. |
| `sitehelper/app/tests/test_app_input.c` (new) | Application/editor/history integration tests. |
| `sitehelper/app/tests/test_sdl_input.c` (new) | Headless real SDL application event-loop/rendering tests. |
| `sitehelper/app/INPUT.md` (new) | This architecture/API/behavior/verification/file report. |
| `sitehelper/editor/wall_tool.h` | Pointer/constraint state and typed resolver/status APIs. |
| `sitehelper/editor/wall_tool.c` | Safe exact-length endpoint resolution and preview lifecycle. |
| `sitehelper/editor/sitehelper_editor.h` | Typed numeric preview/action/cancellation APIs. |
| `sitehelper/editor/sitehelper_editor.c` | Raw direction updates and shared normal Wall action creation. |
| `sitehelper/editor/tests/test_wall_tool.c` | Exact-length and numeric edge tests alongside existing mouse tests. |
| `sitehelper/topology/tests/test_plan_topology.c` | Correct pre-existing minimum-integer oracle without signed overflow; assertions unchanged. |
