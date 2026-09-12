#ifndef EDITOR_SNAP_STATE_H
#define EDITOR_SNAP_STATE_H

#include "snap.h"

/* SiteHelper binds generic SnapSettings/results to physical millimetres in the
 * active plan X/Y or elevation U/Z space. Grid spacing and object tolerance are
 * millimetres, not fixed screen-pixel radii; they do not scale with camera zoom. */
typedef struct
{
    SnapSettings settings;
    SnapResult result;
} EditorSnapState;

void editor_snap_state_init(
    EditorSnapState *state
);

void editor_snap_state_clear(
    EditorSnapState *state
);

void editor_snap_state_set_result(
    EditorSnapState *state,
    SnapResult result
);

const SnapResult *
editor_snap_state_get_result(
    const EditorSnapState *state
);

const SnapSettings *
editor_snap_state_get_settings(
    const EditorSnapState *state
);

int editor_snap_state_has_snap(
    const EditorSnapState *state
);

#endif