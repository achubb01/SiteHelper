#ifndef APP_INPUT_H
#define APP_INPUT_H

#include "platform_event.h"
#include "text_edit.h"
#include "length_parse.h"
#include "editor_properties.h"

typedef enum {
    APP_KEYBOARD_FOCUS_NONE,
    APP_KEYBOARD_FOCUS_TOOL_LENGTH,
    APP_KEYBOARD_FOCUS_PROPERTY_MM
} AppKeyboardFocus;
typedef enum {
    APP_INPUT_UNHANDLED, APP_INPUT_CONSUMED, APP_INPUT_COMMAND,
    APP_INPUT_UNDO, APP_INPUT_REDO, APP_INPUT_SWITCH_VIEW,
    APP_INPUT_PAN_LEFT, APP_INPUT_PAN_RIGHT, APP_INPUT_PAN_UP, APP_INPUT_PAN_DOWN
} AppInputResult;

typedef struct {
    AppKeyboardFocus focus;
    TextEdit text;
    LengthParseStatus parse_status;
    WallLengthStatus length_status;
    TextEditResult edit_status;
    int millimetres;
    int command_failed;

    /* PROPERTY_MM captures value identity, never a project pointer. Current
     * selection must continue to match this target until Enter commits. */
    EditorProperty property;
    EditorSelectionKind property_target_kind;
    DomainId property_slab_id;
    size_t property_feature_index;
    /* A property field is seeded with its current value. The first committed
     * text-input event replaces that seed; cursor/editing keys opt into editing
     * the seeded text in place instead. */
    int replace_on_next_text_input;
} AppInput;

/* Zero initialization is valid. Application owns lifetime and focus. */
void app_input_cancel(AppInput *input, SiteHelperEditor *editor);
/* Start numeric editing from the selected object's current authoritative value. */
int app_input_begin_property(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperty property);
/* Refresh after pointer/context changes; drops focus if placement/target vanished. */
void app_input_refresh(AppInput *input, SiteHelperEditor *editor);
void app_input_refresh_in_project(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project);
int app_input_wants_text(const AppInput *input, const SiteHelperEditor *editor);
/* Focus routing precedes global shortcut decoding. COMMAND returns an ordinary
 * EditorAction; caller executes history, completes/reconciles, then refreshes.
 * A failed command leaves text/focus available for correction and retry. */
AppInputResult app_input_route(AppInput *input, SiteHelperEditor *editor,
    const PlatformEvent *event, EditorAction *action);
AppInputResult app_input_route_in_project(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, const PlatformEvent *event, EditorAction *action);
int app_input_valid(const AppInput *input);
const char *app_input_feedback(const AppInput *input);

#endif
