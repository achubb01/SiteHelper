# Priority 33A — Presentation Composition Boundary

## Status

Implemented against the completed Priority 32 workspace architecture.

Priority 32 now owns authoritative transient `EditorWorkspace` state in
`SiteHelperEditor`. Priority 33A consumes that state directly; it does **not**
introduce a parallel presentation-profile state or infer context from the active
tool.

The viewport path is now:

```text
EditorWorkspace + EditorView
            |
            v
AppPresentationPolicy
  - persistent layer visibility/emphasis
  - transient overlay visibility/emphasis
            |
            v
app_presentation_render_viewport()
            |
            +-- existing app/domain render adapters
            +-- transient editor overlay adapters
            |
            v
Renderer2D
```

`sitehelper_app_render()` clears the frame, asks for the policy for the active
workspace/view, delegates viewport composition, then renders GUI chrome,
toolbar, properties and HUD.

This is deliberately **not** a scene graph, ECS, retained render tree, generic
layer registry, or new source of Project/editor authority.

## Relationship to Priority 32D

Priority 32D intentionally introduced a small amount of workspace-aware rendering
inside `app_view.c` so workspace selection/properties/render emphasis could be
validated before a general composition boundary existed.

Priority 33A removes that temporary ownership:

- `app_render_walls()` no longer decides whether Framing is active;
- `app_render_slabs()` no longer decides whether Slab is active;
- `app_render_roofs()` no longer hides itself outside Roof workspace; and
- persistent documentation renderers no longer inspect `EditorWorkspace` to mute
  themselves.

Those adapters render what the caller asks them to render. Workspace visibility
and emphasis now come from `AppPresentationPolicy`.

The 45% contextual colour treatment introduced by Priority 32D is retained by
the composer so this architectural move does not discard the established visual
behaviour.

## Policy vocabulary

Every layer/overlay has one of four semantic emphasis values:

```text
HIDDEN
CONTEXT
NORMAL
PRIMARY
```

`HIDDEN` prevents dispatch. `CONTEXT` currently subdues supported layer styles.
`NORMAL` and `PRIMARY` are visually equivalent for several adapters today, but
remain distinct so the active domain can become more prominent later without
changing the policy contract.

Current persistent layers:

```text
grid
slabs
walls
roof source intent
dimensions
symbols
callouts
notes
revision clouds
```

Current transient groups:

```text
framing overlays
slab overlays
roof overlays          (reserved; no edit-handle adapter yet)
dimension overlay      (shared across workspaces)
documentation overlays
measurement overlay
snap overlay
```

The Dimension preview is intentionally separate from documentation-only overlays.
Priority 32 exposes Dimension in Framing, Slab, Roof and Documentation, so
placing its preview inside a Documentation group would create an interaction that
works mechanically but appears invisible in three workspaces.

## Workspace mapping

`app_presentation_policy_for_workspace()` is a pure mapping from the authoritative
Priority 32 workspace plus view into render policy.

### GENERAL / Plan

Compatibility surface:

```text
grid                 NORMAL
slabs                NORMAL
walls                NORMAL
roof source intent   HIDDEN
document objects     NORMAL
framing overlays     NORMAL
slab overlays        NORMAL
dimension overlay    NORMAL
document overlays    NORMAL
measurement          NORMAL
snap                 NORMAL
```

`GENERAL` remains a compatibility/editor state rather than a user-facing
workspace.

### FRAMING / Plan

```text
grid                 NORMAL
walls                PRIMARY
slabs                CONTEXT
roof source intent   HIDDEN
dimensions           NORMAL
other document objs  CONTEXT
framing overlays     PRIMARY
dimension overlay    NORMAL
measurement          NORMAL
snap                 PRIMARY
```

FRAMING / Wall Elevation keeps only the grid, wall presentation, framing
overlays and snap overlay.

### SLAB / Plan

```text
grid                 NORMAL
slabs                PRIMARY
walls                CONTEXT
roof source intent   HIDDEN
dimensions           NORMAL
other document objs  CONTEXT
slab overlays        PRIMARY
dimension overlay    NORMAL
measurement          NORMAL
snap                 PRIMARY
```

### ROOF / Plan

```text
grid                 NORMAL
roof source intent   PRIMARY
walls                CONTEXT
slabs                CONTEXT
dimensions           NORMAL
other document objs  CONTEXT
roof overlays        PRIMARY (reserved slot)
dimension overlay    NORMAL
measurement          NORMAL
snap                 PRIMARY
```

The current roof renderer still draws authoritative portion support polygons only.
It does not claim that those outlines are derived roof planes, ridges, hips or
valleys.

### DOCUMENTATION / Plan

```text
grid                 NORMAL
walls                CONTEXT
slabs                CONTEXT
roof source intent   HIDDEN
dimensions           PRIMARY
symbols              PRIMARY
callouts             PRIMARY
notes                 PRIMARY
revision clouds      PRIMARY
dimension overlay    PRIMARY
document overlays    PRIMARY
measurement          NORMAL
snap                 NORMAL
```

## Render-adapter neutrality

`app_view.c` remains responsible for translating a specific domain/document type
into Renderer2D primitives. It no longer owns workspace decisions.

Walls and slabs already take render styles, so the composer derives contextual
copies of those styles. Persistent documentation layers now accept a small
`AppAnnotationRenderStyle` containing only a colour-scale percentage. That keeps
the adapters workspace-neutral while preserving their domain-specific colours.

A remembered `current_wall_id` is navigation state. When walls are contextual,
the composer deliberately makes the contextual selected/current colour equal to
the contextual normal wall colour so a wall remembered from Framing does not
look selected while the user is working in Slab, Roof or Documentation.

## Roof source layer

Priority 32D added a real production adapter for authoritative roof support
polygons. Priority 33A therefore dispatches that adapter through the `roofs`
policy slot. The previous 33A assumption that no roof adapter existed is no
longer true.

Future derived roof geometry and roof edit handles should plug into the existing
composer boundary rather than adding branches back to `sitehelper_app_render()`.

## Ownership invariants

1. `EditorWorkspace` remains owned by the editor/application context from
   Priority 32.
2. Presentation policy is derived transient state and is never persisted.
3. The composer never mutates `SiteHelperProject` or editor interaction state.
4. `EditorView` remains the coordinate/presentation-space capability boundary.
5. Renderer2D remains generic and knows nothing about construction workspaces.
6. Domain/document render adapters remain responsible for translation into
   drawing primitives, not deciding workspace relevance.
7. The composer owns viewport layer ordering, visibility, emphasis and overlay
   dispatch.
8. GUI chrome, workspace/tool toolbar, properties and HUD remain outside the
   viewport composer.

## Priority sequencing consequence

The originally proposed separate "33B workspace-to-presentation mapping" is no
longer necessary: completed Priority 32 makes that mapping available now, and it
is part of 33A.

The next Priority 33 step should build on this boundary rather than add another
context abstraction. Likely follow-ons are explicit presentation/theme styling
where `PRIMARY` needs to differ from `NORMAL`, and then new domain presentation
adapters (roof derived geometry/edit handles, framing overlays, etc.) as their
underlying authoritative/derived geometry contracts become ready.
