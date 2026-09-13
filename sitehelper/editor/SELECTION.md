# Selection context

`SiteHelperEditor` owns one transient `EditorSelection`. Navigation/focus is
separate: `current_storey_id`, `current_room_id` and `current_wall_id` identify
the editing context, not necessarily a selected object. An elevation remains
focused on its Wall after selection is cleared. Plan Select still updates both
Wall navigation and selection. Plan rendering continues to highlight the current
Wall; distinguishing focus from selection visually is deferred.

Selection **kind** identifies what was selected (`WALL`, `OPENING`,
`WALL_MEMBER`); selection **scope** identifies its context:

| Scope | Coordinate context | Current creation path |
| --- | --- | --- |
| `PLAN` | Plan X/Y millimetres | Physical Wall selection |
| `WALL_ELEVATION` | Wall-local U/Z millimetres | Generated member or Opening selection |
| `NONE` | No selection | Initialization, clear or failed setter |

`EditorSelectionScope` belongs to the selection layer and does not depend on
`EditorView`. Kind and scope remain orthogonal: the container permits any valid
non-NONE scope with any supported kind. Current editor hit-test paths explicitly
pass Plan scope for Walls and Elevation scope for members/Openings. Wall-local
member hit testing requires the Elevation view.

All setters accept a scope. Invalid scopes, IDs or member arguments clear the
entire selection. `NONE` kind always pairs with `NONE` scope; clear resets both
IDs and the copied member payload. A Wall selection identifies the physical Wall
by `wall_id`; an Opening/member selection uses that ID as its explicit owner.
Scope and ownership are never inferred from `current_wall_id`.

`sitehelper_editor_selection_matches_view` maps the active view to its expected
scope. Both editor reconciliation paths clear mismatches, including manually
constructed stale state. Storey reconciliation still resolves identities only
inside the active Storey. Switching views clears the single selection; switching
back does not restore it. Storey switching also retains its existing clearing
behavior. Clearing selection alone never clears navigation.

Elevation rendering asks `editor_selection_get_wall_member` for Elevation scope
and the currently viewed physical Wall's ID. Both must match before a highlight
is resolved. Member identity remains the existing copied `WallSelection`/Timber
value, with no persistent pointers or generated IDs. Regeneration preserves the
selection and scope if the member still resolves; a missing member clears the
complete editor selection.

Property inspection rejects mismatched view/scope even before reconciliation,
then resolves authoritative Wall/Opening values by stable IDs and current Storey,
independently of Wall navigation. It has no cache. Generated members remain
unsupported by the authoritative property snapshot.

Selection is editor-only: no Project ownership, persistence, DomainId allocation,
command or undo/redo entry. Multi-selection, remembered per-view selections and
selection history are not implemented.
