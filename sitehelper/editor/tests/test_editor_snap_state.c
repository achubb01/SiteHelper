#include <assert.h>
#include <stdio.h>

#include "editor_snap_state.h"

static void test_snap_state_initialises_without_snap(void)
{
    EditorSnapState state;

    editor_snap_state_init(
        &state
    );

    assert(
        !editor_snap_state_has_snap(
            &state
        )
    );

    const SnapResult *result =
        editor_snap_state_get_result(
            &state
        );

    assert(result != NULL);
    assert(result->type == SNAP_NONE);
}

static void test_snap_state_initialises_default_settings(void)
{
    EditorSnapState state;

    editor_snap_state_init(
        &state
    );

    const SnapSettings *settings =
        editor_snap_state_get_settings(
            &state
        );

    assert(settings != NULL);

    assert(settings->grid_enabled == 1);
    assert(settings->grid_spacing == 100.0);

    assert(settings->endpoint_enabled == 1);
    assert(settings->intersection_enabled == 1);

    assert(
        settings->object_snap_tolerance
        == 80.0
    );
}

static void test_snap_state_can_store_result(void)
{
    EditorSnapState state;

    editor_snap_state_init(
        &state
    );

    SnapResult result = {
        .position = {
            .x = 300.0,
            .y = 500.0
        },

        .type = SNAP_ENDPOINT
    };

    editor_snap_state_set_result(
        &state,
        result
    );

    const SnapResult *stored =
        editor_snap_state_get_result(
            &state
        );

    assert(stored != NULL);

    assert(stored->position.x == 300.0);
    assert(stored->position.y == 500.0);
    assert(stored->type == SNAP_ENDPOINT);

    assert(
        editor_snap_state_has_snap(
            &state
        )
    );
}

static void test_snap_state_clear_removes_result(void)
{
    EditorSnapState state;

    editor_snap_state_init(
        &state
    );

    editor_snap_state_set_result(
        &state,
        (SnapResult){
            .position = {
                .x = 300.0,
                .y = 500.0
            },

            .type = SNAP_ENDPOINT
        }
    );

    editor_snap_state_clear(
        &state
    );

    assert(
        !editor_snap_state_has_snap(
            &state
        )
    );

    const SnapResult *result =
        editor_snap_state_get_result(
            &state
        );

    assert(result != NULL);
    assert(result->type == SNAP_NONE);
}

static void test_snap_state_clear_preserves_settings(void)
{
    EditorSnapState state;

    editor_snap_state_init(
        &state
    );

    editor_snap_state_set_result(
        &state,
        (SnapResult){
            .position = {
                .x = 300.0,
                .y = 500.0
            },

            .type = SNAP_ENDPOINT
        }
    );

    editor_snap_state_clear(
        &state
    );

    const SnapSettings *settings =
        editor_snap_state_get_settings(
            &state
        );

    assert(settings != NULL);

    assert(settings->grid_enabled == 1);
    assert(settings->grid_spacing == 100.0);

    assert(settings->endpoint_enabled == 1);
    assert(settings->intersection_enabled == 1);

    assert(
        settings->object_snap_tolerance
        == 80.0
    );
}

static void test_snap_state_accepts_null(void)
{
    editor_snap_state_init(NULL);
    editor_snap_state_clear(NULL);

    editor_snap_state_set_result(
        NULL,
        (SnapResult){
            .type = SNAP_ENDPOINT
        }
    );

    assert(
        editor_snap_state_get_result(NULL)
        == NULL
    );

    assert(
        editor_snap_state_get_settings(NULL)
        == NULL
    );

    assert(
        !editor_snap_state_has_snap(NULL)
    );
}

int main(void)
{
    test_snap_state_initialises_without_snap();
    test_snap_state_initialises_default_settings();
    test_snap_state_can_store_result();
    test_snap_state_clear_removes_result();
    test_snap_state_clear_preserves_settings();
    test_snap_state_accepts_null();

    printf(
        "All editor snap state tests passed.\n"
    );

    return 0;
}