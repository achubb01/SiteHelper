# Wall Elevation Presentation

Priority 34A introduces a renderer-neutral, read-only presentation/query boundary
for generated wall framing.

The dependency direction is now:

```
Wall definition + generated WallFraming
                 |
                 v
wall_elevation_presentation
                 |
        +--------+--------+
        |                 |
        v                 v
 wall elevation       elevation hit
    renderer             queries
        |
        v
    Renderer2D
```

This is deliberately **not** a generic render scene. It is a narrow adapter for
one established view of one domain.

## Why this boundary exists

`WallFraming` is generation storage. Its arrays are useful implementation data,
but a professional framing view needs semantic meaning such as "king stud" or
"header", not knowledge that a member happened to live in `studs[7]` or
`members[1]`.

The presentation adapter interprets generated `Timber` into a deterministic
semantic stream with:

- bottom and top plates;
- common, king, trimmer and cripple studs;
- noggins;
- generated header and sill roles;
- wall-local U/Z rectangles;
- the existing `WallMemberKind` needed by transient selection;
- effective opening clear bounds with stable Opening IDs; and
- authoritative elevation extents when effective BuildSettings are supplied.

The existing order is retained intentionally: plates, studs, noggins, generated
opening members. That preserves current draw order and member hit precedence.

## Ownership and identity

The adapter owns nothing and allocates nothing. It does not add persistent
member IDs and it does not become project authority.

A `WallElevationMember` contains a borrowed `const Timber *source` only so an
immediate renderer/query can bridge to the existing generated-member selection
mechanism. That pointer must never be stored in editor/project state. SiteHelper
continues to store wall-member selection by value and reconcile it after framing
regeneration.

Openings are different: they already have stable DomainIds, so elevation opening
presentation returns those IDs rather than pointers.

## Priority 34A scope

34A is intentionally a no-visual-change architecture step:

- `wall_elevation_render` consumes the semantic member stream instead of walking
  `WallFraming` arrays itself;
- member and opening hit queries consume the same presentation/query geometry;
- semantic member roles become available for later role-specific styling;
- opening bounds and wall extents become available for later context rendering.

It does **not** yet introduce member labels, role colours, opening outlines,
dimensions, axes, edit handles, or new member identity.

Those belong in later Priority 34 increments on top of this boundary.

## Priority 34B visual language

34B consumes the 34A semantics without adding new domain authority. The
framing renderer can now assign independent presentation colours to every
semantic member role while retaining `timber_colour` as the fallback. A role
colour with alpha `0` means "inherit the base timber colour", so callers that
only provide the older base style keep the previous appearance.

The elevation context layer now draws, in order:

1. the full Wall elevation extent, when enabled and effective BuildSettings are
   available;
2. authoritative Opening clear rectangles, when enabled;
3. generated framing members using their semantic roles.

Context outlines therefore sit behind the framing rather than obscuring it.
The renderer accepts a value-only `WallElevationRenderSelection` bridge:

- a resolved ephemeral generated-member pointer for the current synchronous
  render only;
- a stable Opening DomainId; and
- a whole-Wall selected flag.

Any active selection overrides semantic styling with `selected_colour`. The app
continues to own interaction state and resolves Storey BuildSettings before
calling the renderer; the wall renderer neither resolves project ownership nor
stores editor state.

The default SDL framing workspace enables wall/opening context and assigns a
restrained distinct palette to bottom/top plates, common/king/trimmer/cripple
studs, noggins, headers and sills. This is presentation policy only: framing
construction, persistence, command history and generated-member identity are
unchanged.

### Still deferred after 34B

34B intentionally does not add labels, dimensions, fixed-pixel member markers,
U/Z debug axes, framing edit handles or a legend. Those can build on the same
semantic stream without expanding `WallFraming` or creating a generic render
scene.

## Priority 34C context and identification

34C makes the semantic view self-identifying without adding object names or
annotation records to project authority.

The elevation can now show:

- the current Wall's stable DomainId above the elevation;
- each Opening's type and stable DomainId inside its clear rectangle;
- the selected generated member's semantic role, timber section and length;
- overall Wall width and elevation height as derived dimension linework; and
- the selected Opening's effective clear width/height as transient detail text.

Only the overall dimensions are rendered as world-space graphical linework;
all labels are transient screen-space annotations derived synchronously from the
34A presentation stream. Nothing is written back to `Wall`, `WallFraming`, the
Editor selection, persistence, or command history.

SiteHelper still does not have a production typography/layout system. 34C uses
Renderer2D's replaceable screen-space debug-text callback only for these compact
labels, just as numeric/documentation HUD code already does. The wall renderer
owns no font metrics and makes no typography contract. Replacing debug text with
production typography later therefore does not change framing semantics.

To avoid label noise, member identification is selection-driven rather than
printing a role on every stud. Opening labels remain compact (`Window <id>` /
`Door <id>`), with clear size shown only for the selected Opening. Overall wall
dimensions use fixed-pixel annotation offsets converted through the current
camera scale while the measured values remain integer millimetres.

Still deferred after 34C: production typography, a persistent/user-authored Wall
name, a framing legend, U/Z debug axes, editable framing handles and any new
member identity model.
