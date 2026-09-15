# Slab lifecycle and drawing

The plan-only Slab Tool owns an unfinished ordered polygon in editor memory.
Clicks accept snapped integer-millimetre `PlanPosition` vertices; no unfinished
geometry enters `SiteHelperProject` or persistence. Consecutive duplicate clicks
are ignored. Clicking the first point within the normal object tolerance after
three vertices, or pressing Enter, attempts implicit closure without duplicating
the first vertex.

Unsnapped fractional millimetres use the same conversion as the Wall Tool:
finite in-range coordinates truncate toward zero. Snapped integral coordinates
remain exact, and out-of-range values are rejected before any integer cast.

Commit validates through the slab domain, then emits one owned `CREATE_SLAB`
command. History executes that command against the sketch's Storey using UI
defaults of 100 mm base thickness and 0 mm Storey-relative top offset. Success
clears the sketch and selects the new slab by its stable `DomainId`; failure keeps
the sketch for correction. Escape, tool/view/Storey changes, and project
replacement cancel it. Pointer leave only hides the live preview.

`CREATE_SLAB` owns a deep outline copy. Undo removes the slab and redo restores
the original ID. Whole-slab Delete emits `DELETE_SLAB`; its history state owns a
deep snapshot including penetrations, replacement regions, and edge rebates, and
undo restores the original Storey and collection position. Commands contain no
editor/view state, and persistence remains v14.

## Slab feature creation

The plan-only Penetration and Region tools share an editor-owned dynamic polygon
interaction. The first vertex uses the existing deterministic slab hit query to
lock a parent slab in the current Storey; later vertices cannot retarget it.
They use the same snapping and finite, in-range, truncation-toward-zero
`PlanPoint` conversion as the Slab Tool. Click-first and Enter closure are
implicit. A failed command leaves the parent and sketch intact; success clears
the sketch and selects the returned parent slab plus ephemeral collection index.

A Region begins with neutral creation properties copied from its parent slab's
base Storey-relative top offset and thickness. These become independent region
values when the command succeeds; there is no continuing link to base values.

The Rebate tool has a separate edge-local interaction. Its first click chooses a
current-Storey slab exterior edge and integer U; its second click must project to
that same edge. The interval is normalized before `ADD_SLAB_EDGE_REBATE` runs.
Projection clamps to the finite edge, maps endpoints exactly to 0 and the
authoritative rounded edge length, and rounds an interior `t * length` to the
nearest integer millimetre with half values upward. Preview shows only the
unambiguous longitudinal interval. The editor-only defaults are 100 mm inward
width and 20 mm depth below local slab top; they are neither domain constraints
nor compliance rules.

All three feature tools emit the owning ADD commands through history. They never
allocate subordinate DomainIds. Escape, tool/view/Storey changes, project
replacement, and loss of the locked parent cancel transient state. Unfinished
polygons, rebate endpoints, and UI defaults are never persisted. Movement and
reshape remain later work. Priority 25E4 now edits the existing scalar slab,
region and rebate properties through the separate properties/history path.
