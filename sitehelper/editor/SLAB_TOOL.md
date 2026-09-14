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
undo restores the original Storey and collection position. Subfeature deletion
is deferred. Commands contain no editor/view state, and persistence remains v14.
