# Priority 33D — Selection / Interaction Presentation

## Status

Implemented on top of Priority 33C.

Priority 33A/33B separated model-layer visibility/emphasis from domain rendering,
and 33C moved transient drawing bodies out of the composer. 33D gives persistent
editor interaction state a similarly explicit visual contract.

The important distinction is:

```text
layer emphasis                 interaction meaning
--------------                 -------------------
CONTEXT                        selected object
NORMAL                         selected object's owner
PRIMARY                        current/navigation context
```

These are independent axes. A selected object remains an interaction highlight;
it is not converted into a brighter form of PRIMARY or a darker form of CONTEXT.

## Interaction roles

`AppInteractionStyle` currently defines four roles:

```text
selected_colour          exact EditorSelection target
hovered_colour           transient pointer target
selection_owner_colour   persistent owner of a selected subordinate object
navigation_colour        current context that is not itself selected
```

Priority 34D introduced `hovered_colour` for Wall-elevation framing. Hover is
still transient editor state rather than selection authority; the shared colour
only keeps its interaction meaning reusable across future workspaces.

Default colours preserve the existing exact-selection yellow and slab-parent
highlight. Navigation/current context now has its own warm accent so a remembered
`current_wall_id` no longer visually claims to be selected.

## Selection authority

`EditorSelection` is the only authority for selection semantics.

Plan walls therefore no longer infer selection from `current_wall_id`. The wall
is yellow only when the Plan selection explicitly identifies that wall. If an
Opening or other subordinate wall target is selected, the wall may use the owner
highlight. A wall referenced only by `current_wall_id` uses the navigation colour
when wall presentation is active.

This matters because Priority 32 intentionally preserves navigation across
workspace changes while clearing incompatible selections.

## Domain mapping

- **Walls** — exact Plan wall selection uses `selected_colour`; subordinate
  selection uses `selection_owner_colour`; `current_wall_id` uses
  `navigation_colour` only while walls are NORMAL/PRIMARY. Wall-elevation member
  selection uses the exact selected colour.
- **Slabs** — exact slab/feature selection continues to use `selected_colour`;
  the owning slab of a selected subordinate feature uses
  `selection_owner_colour`.
- **Roofs** — selecting a whole roof highlights all of its authoritative portion
  boundaries with `selected_colour`. Selecting one portion highlights that
  portion exactly and its sibling portions with `selection_owner_colour`.
- **Documentation** — dimensions, notes, symbols, callouts and revision clouds
  all use the same exact selection colour instead of hard-coding their own copy.

Transient authoring previews, snap feedback and geometry handles remain overlay
state from Priority 33C. They are not persistent selection and are intentionally
not folded into this style.

## Ownership

`SiteHelperApp` owns `AppInteractionStyle` beside `AppPresentationStyle` and
passes both through `AppPresentationRenderContext`.

`presentation_composition` may suppress navigation-only emphasis when a model
layer is merely CONTEXT, but it does not suppress an explicit compatible
selection. Workspace selection reconciliation from Priority 32 remains the source
of truth for whether a selection is valid in the active workspace.

No interaction colours are persisted into Project data.

## Invariants

1. `EditorSelection` means selection; navigation IDs do not.
2. Layer emphasis and interaction highlighting are independent concerns.
3. Exact selections share one application-level visual role across domains.
4. Subordinate selections may identify their owning persistent object without
   making the owner look exactly selected.
5. Navigation/current context must remain visually distinguishable from
   selection.
6. Transient authoring overlays remain separate from persistent selection style.
7. Hover may use the shared interaction colour while remaining transient and
   subordinate to exact selection.
8. Renderer2D remains unaware of workspace, selection and domain semantics.
