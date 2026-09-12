Measurement query = transient editor state.
Persistent dimension annotation = intentionally deferred.

`EDITOR_TOOL_MEASURE` is available only in Plan view. The fourth toolbar button
(`Meas.`) maps through its own `GuiButtonId`; Select, Opening and Wall retain their
existing IDs and positions. Elevation disables Measure and uses the existing
Select fallback when leaving Plan with Measure active.

`MeasurementTool` owns an active flag, a has-start flag and a
`PlanMeasurementQuery` value: `PlanPoint start/end`, `double distance_mm`, and
`completed`. It allocates nothing and has no identity, geometry references or
Project ownership. The application reads a copied value through
`sitehelper_editor_get_measurement`; absence clears the output. It does not
inspect measurement-tool internals. Tool lifecycle lives in
`measurement_tool_init/activate/cancel/update/click/get_query`.

The first click establishes A and a live zero-length query. Pointer movement
updates its endpoint and Euclidean distance. The second click establishes B and
marks the query complete; subsequent motion does not move B. The next click
replaces the completed query with a new A. A → A is valid and reports zero.
Nonfinite points or a distance that overflows `double` are rejected without
replacing the previous valid query.

Points and calculated distance are physical plan millimetres, potentially
fractional, consistent with [MEASUREMENTS.md](../model/MEASUREMENTS.md). There is
no Wall-derived length or `INT_MAX` restriction: a query can span the diagonal of
the full integer coordinate range. Application presentation rounds to the nearest
whole millimetre (half upward) and prints `N mm` without integer narrowing. The
retained query is not rounded. No units parser, preferences or formatting
framework was added.

Motion and each click call the existing editor snap resolver at that event's
position. A snap result supplies the point when present; otherwise the raw Plan
pointer is used. This avoids stale motion/snap coordinates on clicks. Current
Plan snapping supplies grid points; it does not yet gather Wall endpoint or
intersection candidates. Measure shares the resolver so it can use richer Plan
candidates later. No topology dependency or snap-system redesign was introduced.

`sitehelper_editor_cancel_tool_interaction` lets application Escape routing
dispatch cancellation to the editor tool. Measure clears its query and snap while
remaining active. Wall numeric-entry Escape still gets first refusal, preserving
the two-stage entry/placement cancellation. Tool/view/Storey changes, explicit
transient invalidation and reconciliation clear the query. Pointer leaving the
viewport also clears the **entire live or completed query**, following the
existing editor pointer-leave contract; returning requires a fresh first click.
The existing resize/tool-refresh path likewise invalidates transient state.

`app_render_measurement` draws one world-space line and a screen-text label just
above the camera-transformed world midpoint, under the normal viewport clip.
Live queries use blue; completed queries use yellow. Midpoint calculation avoids
sum overflow, nonfinite screen projections are skipped, and existing temporary
backend text rendering is reused. No dimension annotation rendering is implied.

Measurement clicks return `EDITOR_ACTION_NONE`. They allocate no DomainIds,
produce no commands or history entries, and alter no Project geometry or save
representation. Undo/redo still operate only on Project commands; successful
reconciliation clears transient queries and never restores them. Persistence
remains version 10. No model, command or persistence implementation was changed.

Verification adds standalone tool tests, editor/query/presentation integration
tests, and measurement coverage in the existing headless SDL application test.
Tests compare serialization before/after two clicks, check ID and history state,
exercise a retained redo branch, and verify current snapping, display rounding,
large distances, cancellation, invalid geometry and view/tool/Storey lifecycle.
Existing Wall numeric-input tests remain intact.
