#include "editor_snap_state.h"

void editor_snap_state_init(
    EditorSnapState *state
)
{
    if (state == NULL) {
        return;
    }

    *state = (EditorSnapState){
        .settings = {
            .grid_enabled = 1,
            .grid_spacing = 100.0,

            .endpoint_enabled = 1,
            .intersection_enabled = 1,

            .object_snap_tolerance = 80.0
        },

        .result = {
            .position = {0.0, 0.0},
            .type = SNAP_NONE
        }
    };
}

void editor_snap_state_clear(
    EditorSnapState *state
)
{
    if (state == NULL) {
        return;
    }

    state->result = (SnapResult){
        .position = {0.0, 0.0},
        .type = SNAP_NONE
    };
}

void editor_snap_state_set_result(
    EditorSnapState *state,
    SnapResult result
)
{
    if (state == NULL) {
        return;
    }

    state->result = result;
}

const SnapResult *
editor_snap_state_get_result(
    const EditorSnapState *state
)
{
    if (state == NULL) {
        return NULL;
    }

    return &state->result;
}

const SnapSettings *
editor_snap_state_get_settings(
    const EditorSnapState *state
)
{
    if (state == NULL) {
        return NULL;
    }

    return &state->settings;
}

int editor_snap_state_has_snap(
    const EditorSnapState *state
)
{
    if (state == NULL) {
        return 0;
    }

    return state->result.type != SNAP_NONE;
}