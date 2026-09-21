# Priority 33B — Presentation Emphasis Styling

## Status

Implemented on top of Priority 33A.

Priority 33A established the semantic presentation policy:

```text
HIDDEN
CONTEXT
NORMAL
PRIMARY
```

33B gives those visible emphasis values an explicit rendering contract instead
of leaving them as composer-local colour arithmetic.

The boundary is:

```text
AppPresentationPolicy
        |
        | emphasis for each persistent layer
        v
AppPresentationStyle
        |
        | AppRenderTone
        v
workspace-neutral render adapter
        |
        v
Renderer2D
```

This remains deliberately smaller than a general UI theme system. It does not
own GUI colours, fonts, line types, object palettes or domain geometry.

## Default emphasis style

The default application style is:

```text
CONTEXT  45% RGB intensity
NORMAL   unchanged base colour
PRIMARY  unchanged base colour + 12% blend toward white
```

This preserves the contextual treatment established by Priority 32D and keeps
legacy/GENERAL `NORMAL` presentation unchanged, while making the active
workspace's `PRIMARY` model layer visibly distinct from `NORMAL` layers.

Examples:

```text
FRAMING
    wall geometry       PRIMARY
    dimensions          NORMAL
    slab/support context CONTEXT

SLAB
    slab geometry       PRIMARY
    dimensions          NORMAL
    walls               CONTEXT

ROOF
    roof source intent  PRIMARY
    dimensions          NORMAL
    walls/slabs          CONTEXT

DOCUMENTATION
    dimensions/markup   PRIMARY
    walls/slabs          CONTEXT
```

## Ownership

`AppPresentationStyle` is application presentation state. `SiteHelperApp` owns
the current style and passes it into `AppPresentationRenderContext`.

`presentation_composition` maps an `AppPresentationEmphasis` to the relevant
`AppRenderTone`. Domain/document render adapters still own their base colours and
geometry. They receive only the derived tone needed to render that base palette
at the requested prominence.

The colour transform is centralized in `app_render_tone_apply()` so walls,
slabs, roof source intent, dimensions and persistent documentation do not each
invent their own definition of contextual/primary colour treatment.

## Selection and interaction feedback

Selection highlight is intentionally not treated as ordinary layer colour.

Priority 33D now owns this concern through `AppInteractionStyle`. Exact selection,
selection-owner context and navigation/current context are separate visual roles.
A remembered `current_wall_id` is suppressed when walls are only context, while
`EditorSelection` remains the authority for actual selection. Workspace transition
rules from Priority 32 clear incompatible semantic selections.

Transient authoring previews, edit handles, measurement feedback and snap
markers also remain at their established full-strength colours in 33B. They are
interaction feedback and need to remain legible even when the persistent model
layer underneath them is subdued.

Priority 33C extracted the remaining tool-specific overlay drawing bodies. Their
interaction colours are still intentionally local to the overlay adapters; a
separate overlay theme is unnecessary until those visuals need shared semantics.

## Invariants

1. `AppPresentationPolicy` decides visibility and semantic emphasis.
2. `AppPresentationStyle` decides how semantic emphasis looks.
3. Domain/document render adapters own geometry and base palettes.
4. Renderer2D remains unaware of SiteHelper workspaces or emphasis semantics.
5. Presentation style is transient and is never persisted into the project.
6. Selection/authoring feedback remains distinct from model-layer emphasis.
7. No generic scene graph, retained render tree or theme engine is introduced.

## Next boundary

After 33B, the remaining composition pressure is mostly in transient authoring
and edit overlays. A sensible 33C is therefore an explicit **overlay composition
contract**: keep overlay ordering/visibility in the composer, but move the
remaining tool-specific drawing bodies out of `presentation_composition.c` into
small workspace/domain overlay adapters. That would stop the composer from
becoming the next monolithic render loop while preserving the narrow 33A/33B
architecture.
