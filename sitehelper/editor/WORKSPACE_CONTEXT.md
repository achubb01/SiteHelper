# Priority 32A — Workspace / Editor Context Semantics

## Status

Semantic contract established. Runtime workspace state and workspace UI are
intentionally deferred to the next Priority 32 step so the model is not frozen
before the responsibilities are explicit.

## Problem

The current editor answers tool availability almost entirely from `EditorView`:

```text
Plan            -> wall / measure / slab / documentation / ...
Wall elevation  -> select / opening
```

That is a valid **coordinate/presentation** restriction, but it is not a useful
user workflow model. A user working on a slab, roof, framing layout or drawing
annotation should not receive one flat list containing every tool that happens to
operate in the same Plan coordinate system.

Priority 32 introduces a separate transient concept: **workspace/editor context**.
The final C type/name is deliberately not frozen by 32A. `EditorWorkspace` is a
reasonable implementation name, while the user-facing labels may evolve.

## Three independent concepts

SiteHelper must keep these meanings separate:

1. **Workspace** — what task/domain the user is currently working on.
2. **View** — how/where that work is presented and which coordinate space input
   is interpreted in.
3. **Tool** — the current interaction mode inside that workspace and view.

Conceptually:

```text
workspace / task intent
        +
view / coordinate presentation
        +
tool / interaction mode
        =
effective editor interaction
```

A workspace does not replace `EditorView`, and a view does not imply a workspace.

For example:

```text
FRAMING workspace
    Plan view
        select physical walls
        create/edit wall layout
        measurement / relevant dimensions

    Wall elevation view
        select framing/openings
        create/edit opening
        inspect generated framing

SLAB workspace
    Plan view
        select slab
        create slab
        penetration
        replacement region
        edge rebate
        slab geometry
        properties

ROOF workspace
    Plan/roof presentation initially
        select roof / portion
        source-edit roof intent
        composition / termination
        properties

DOCUMENTATION workspace
    Plan view initially
        notes
        dimensions
        symbols
        callouts
        revision markup
```

These labels and exact tool groupings are provisional. The architectural
separation is not.

## Workspace is transient editor state

Workspace is navigation/UI intent. It must never become building authority.

The eventual active workspace belongs with `SiteHelperEditor` (or a tightly
owned transient editor-context object), not with:

- `SiteHelperProject`;
- `Storey`;
- Wall/Slab/Roof definitions;
- persistence;
- command history; or
- saved construction/domain settings.

Switching workspace must not mutate the Project and must not consume an undo
record. Loading the same Project must not depend on which workspace was active
when it was saved.

## Workspace is not View

`EditorView` remains the presentation/coordinate-space boundary.

The initial compatibility is expected to look roughly like:

| Workspace concept | Plan | Wall elevation |
| --- | --- | --- |
| General / plan navigation | yes | no |
| Framing | yes | yes |
| Slab | yes | no |
| Roof | yes | no |
| Documentation | yes | no |

Future section/detail/3D views can therefore be added to an existing workspace
without inventing new construction domains merely to represent presentation.

Conversely, several workspaces may use the same Plan view while exposing very
different interactions.

## Tool availability has two layers

The current `sitehelper_editor_tool_available(EditorView, EditorTool)` combines
only the first of two policies that Priority 32 needs.

### 1. View capability

A tool must mechanically support the active view/coordinate space.

Examples:

- current wall placement consumes Plan X/Y, not wall-local U/Z;
- current opening placement consumes wall-elevation U/Z;
- slab geometry consumes Plan X/Y;
- current document tools consume Plan X/Y.

This is a low-level capability question and should remain independent of UX
workflow grouping.

### 2. Workspace exposure

A mechanically valid tool is exposed only when it is relevant to the active
workspace.

Examples:

- slab penetration is mechanically a Plan tool but should not appear in the
  framing workspace;
- wall placement is mechanically a Plan tool but should not appear in the slab
  workspace;
- revision-cloud creation is mechanically a Plan tool but belongs to a
  documentation workflow rather than every Plan workflow.

The effective availability rule is therefore:

```text
view supports tool
AND
workspace exposes tool in that view
```

32B should make those two predicates explicit rather than growing the current
single flat `switch`/boolean expression.

Shared tools are expected. `SELECT`, measurement and some dimension interactions
may legitimately be exposed by more than one workspace. Workspace membership
must not imply ownership of the underlying tool implementation.

## Selection and navigation remain separate

Workspace is also distinct from both selection and editor navigation.

Existing navigation IDs such as:

```text
current_storey_id
current_room_id
current_wall_id
```

identify editor focus/navigation. They should not become workspace-owned state.
A framing workspace may use `current_wall_id` for an elevation, while a later
workspace switch should not rewrite authoritative IDs or manufacture a new
selection.

The initial transition policy should be conservative:

- preserve Storey/Room/Wall navigation when it still resolves;
- cancel in-progress transient tool gestures/previews;
- clear the current selection when changing workspace;
- preserve the current view when the destination workspace supports it; and
- otherwise move through an explicit application-level view transition before
  entering the destination workspace.

Clearing selection avoids carrying an unrelated property target into a different
workflow. Preserving navigation avoids losing useful spatial focus merely because
the user changed task.

This policy can be revisited if concrete UX testing demonstrates a better
cross-workspace selection rule. Persistent per-workspace selections are not part
of the initial design.

## View/camera transition boundary

`SiteHelperEditor` currently owns `active_view`, but `AppViews` owns the saved
camera state for each `EditorView` and `app_views_set_active()` performs the
coordinated camera swap.

Therefore a future `sitehelper_editor_set_workspace()` must **not** silently
change `active_view` as a convenience when the destination workspace does not
support the current view. Doing that would bypass the application camera/view
transition contract.

Instead, the application should perform a transition equivalent to:

```text
requested workspace
      |
      +-- current view supported? -- yes --> change workspace
      |
      `-- no --> switch to a valid/preferred view through AppViews
                 then change workspace
```

The editor layer may provide pure queries such as "workspace supports view" and
"preferred initial view", but it should not acquire renderer/camera ownership.

## Rendering is contextual, not authoritative

Workspace can later influence how domains are presented:

- framing can emphasise physical walls/generated framing;
- slab can emphasise slab boundaries/features;
- roof can emphasise roof source/derived geometry;
- documentation can emphasise annotations and drawing objects.

That is rendering/UI policy only. Inactive-domain Project objects remain present
and authoritative. Switching workspace must never create/delete/hide data at the
model layer.

A future renderer may draw non-active domains in a subdued contextual style, but
that policy should live above the domain models and should not be encoded as a
Project property.

## Properties panel semantics

The properties panel should eventually follow the active workspace and selection,
not become a generic dump of every property family.

Workspace may determine:

- which selectable object families are prioritised;
- which property groups are presented;
- which edit actions are offered; and
- which empty-state/help content is useful.

Property values must still resolve fresh from authoritative objects by stable
identity. Workspace state is never a cache of construction properties.

## Roof integration

Priority 26G4 intentionally added
`sitehelper_editor_select_roof_at_position()` without inserting Roofs into normal
Plan SELECT precedence because Roof and Slab geometry commonly overlap.

Workspace context is the missing policy boundary for using that hook. A future
Roof workspace can choose roof/portion selection explicitly without making
ordinary Plan selection globally prefer Roof over Slab (or vice versa).

This is a concrete reason workspace belongs above domain hit-testing rather than
inside the Roof or Slab model.

## Toolbar / command-surface direction

The current SDL prototype keeps all tools in one fixed toolbar and disables the
ones invalid for the active view. That should be treated as transitional UI.

Once runtime workspace state exists, the visible command surface should be built
from the active workspace rather than presenting the global `EditorTool` enum and
asking the user to mentally filter it.

Do not make `EditorTool` order, toolbar order and workspace membership the same
thing. Workspace-to-tool presentation is UI policy and should be explicit.

## Initial invariants for 32B+

The implementation following this semantic step should preserve these invariants:

1. workspace is transient and never serialized;
2. workspace and view are independent state dimensions;
3. a workspace may support multiple views, and a view may be used by multiple
   workspaces;
4. effective tool availability requires both view capability and workspace
   exposure;
5. switching workspace never mutates `SiteHelperProject` or command history;
6. changing workspace cancels incompatible in-progress interaction state;
7. view changes that require camera restoration still pass through `AppViews`;
8. selection/hit-test precedence may be workspace-specific without changing
   domain identity or ownership rules; and
9. shared tools remain shared implementations rather than being duplicated per
   workspace.

## Recommended Priority 32 implementation sequence

### 32A — Semantic contract

This document. No enum or runtime behaviour is frozen yet.

### 32B — Transient workspace state and capability queries

Introduce the chosen workspace type in the editor layer, an active transient
workspace value, explicit workspace/view compatibility, and separate view-tool
capability from workspace-tool exposure. Add transition/invariant tests.

Do not redesign the toolbar in the same step.

### 32C — Application workspace transition + contextual command surface

Add an application-level workspace transition that coordinates `AppViews`, then
replace the global disabled-tool toolbar behaviour with a workspace-driven tool
surface. Make the active workspace visible and directly switchable.

### 32D — Workspace-aware selection/properties/render emphasis

Use workspace policy to resolve domain-overlap selection (especially Roof versus
Slab), tailor properties, and establish contextual rendering emphasis. Keep all
of this outside Project authority.

Roof authoring tools that do not yet exist should be introduced by the roof/editor
feature work that needs them; Priority 32 should provide the context boundary, not
invent fake tool implementations merely to fill a workspace.

## 32A review result

No Project-model or persistence change is required for workspace context. The
correct next implementation boundary is the editor layer, with `AppViews`
remaining responsible for coordinated presentation/camera transitions and the
GUI consuming workspace policy only after the runtime contract is tested.

## Priority 32B implementation

Runtime context now uses `EditorWorkspace` with:

- `GENERAL` — temporary compatibility workspace preserving the pre-32 toolbar
  surface until 32C provides explicit workspace switching;
- `FRAMING`;
- `SLAB`;
- `ROOF`; and
- `DOCUMENTATION`.

`editor_context.[ch]` owns the pure policy predicates for view capability,
workspace/view compatibility and workspace tool exposure. `SiteHelperEditor`
owns only the current transient workspace value and applies those predicates when
changing workspace/view/tool.

`GENERAL` supporting both current views is intentionally transitional. The named
domain workspaces already enforce their narrower semantics, so 32C can replace
the compatibility surface without changing the capability model.

## Priority 32C implementation

The application layer now owns the coordinated workspace transition in
`app/app_workspace.[ch]`.

`app_workspace_set_active()` preserves the editor/application ownership split:

```text
requested workspace
      |
      +-- current view supported --> editor workspace transition
      |
      `-- current view unsupported
              -> AppViews changes to the preferred view and restores its camera
              -> editor workspace transition
```

The transition never writes Project state or history. If no view change is
required, the renderer camera is untouched.

The same application policy module now defines an explicit ordered command
surface for each workspace. This list is intentionally independent of
`EditorTool` enum order. The SDL prototype builds its left toolbar from:

```text
user workspace choices
        +
active workspace command surface
```

Only `FRAMING`, `SLAB`, `ROOF`, and `DOCUMENTATION` are user-facing choices.
`GENERAL` remains the editor's compatibility/default state for callers that do
not own a workspace UI, but the SDL application enters `FRAMING` during startup.

Within a workspace, a tool may remain visible but disabled when the current
`EditorView` cannot mechanically run it. For example, Framing shows both Wall
and Opening: Wall is usable in Plan while Opening is usable in Wall Elevation.
That keeps workspace relevance separate from view capability.

No roof authoring tools are invented by 32C. The Roof command surface currently
contains only Select, Measure and Dimension until real roof source-edit tools
are introduced by roof/editor feature work.

## Priority 32D implementation

Workspace context now scopes three user-facing concerns without changing project
or domain authority:

1. **Selection routing.** Plan `SELECT` dispatches by workspace. Framing queries
   walls, Slab queries slab features, Roof queries authoritative roof support
   polygons, and Documentation queries document objects. `GENERAL` retains the
   pre-workspace mixed precedence for compatibility. A focused workspace does
   not discard remembered wall navigation merely because another domain object
   is selected.

2. **Properties context.** The properties panel only exposes a selection accepted
   by the active workspace. Existing wall/opening scalar commands are available
   through the numeric property-entry path, slab properties retain their prior
   editing behavior, Roof exposes a read-only source-intent snapshot, and
   Documentation reports the selected document-object kind. Roof source editing
   remains deliberately outside Priority 32D; real roof tools should own that
   interaction rather than turning the inspector into an ad-hoc authoring API.

3. **Rendering emphasis.** Walls, slabs and persistent documentation objects are
   visually de-emphasized when they are outside the active workspace. Shared
   dimensions remain fully visible. The Roof workspace draws authoritative
   portion support polygons as an editor/source overlay so roof selection has a
   visible target; these outlines are explicitly not derived roof planes,
   ridges, hips or valleys.

This is contextual emphasis, not a generic presentation architecture. It does
not introduce a scene graph, retained render tree, or general layer registry.
A later presentation-composition boundary may decide which model layers are
visible; workspace policy remains the statement of user intent that such a
boundary can consume.
