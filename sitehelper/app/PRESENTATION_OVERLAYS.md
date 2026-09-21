# Priority 33C — Presentation Overlay Rendering

Transient authoring feedback is not project/model presentation, but it still
needs a stable composition boundary.

## Ownership

`presentation_composition.c` owns:

- whether an overlay group is visible for the active workspace/view;
- its ordering relative to persistent model layers and other overlays; and
- the viewport clip surrounding the complete presentation pass.

Overlay adapters own:

- querying the editor's transient preview state;
- converting that state into Renderer2D primitives; and
- the overlay's established interaction colours and fixed marker sizes.

Overlay adapters do not own workspace policy, clipping, project authority,
selection policy, tool activation, or persistence.

## Current adapters

```text
framing_overlay_render.c
    opening placement preview
    wall placement preview

slab_overlay_render.c
    slab polygon sketch
    penetration/region polygon sketch
    edge-rebate preview
    slab geometry-edit outline/handles

snap_overlay_render.c
    endpoint/grid/centreline/intersection marker
```

Existing dimension, callout, revision-cloud and measurement rendering already
lives outside the composer in `app_view.c`; those functions remain valid overlay
adapters for now.

`roof_overlays` is intentionally still an empty policy slot. 33C does not invent
roof edit handles before the roof editor owns meaningful transient edit state.

## Invariants

1. The composer decides **when and in what order** an overlay is drawn.
2. The adapter decides **how its editor state becomes drawing primitives**.
3. Overlay adapters never inspect `EditorWorkspace`.
4. Overlay adapters never begin/end the viewport clip.
5. Overlay adapters never mutate `SiteHelperEditor` or `SiteHelperProject`.
6. Transient interaction feedback remains full-strength and is not accidentally
   run through the persistent-layer `AppRenderTone` hierarchy.
7. Renderer2D remains unaware of SiteHelper tools, workspaces and domains.
8. No generic overlay registry, scene graph or retained render tree is added.
