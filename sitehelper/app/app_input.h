#ifndef APP_INPUT_H
#define APP_INPUT_H

#include "platform_event.h"
#include "text_edit.h"
#include "length_parse.h"
#include "sitehelper_editor.h"

typedef enum { APP_KEYBOARD_FOCUS_NONE, APP_KEYBOARD_FOCUS_TOOL_LENGTH } AppKeyboardFocus;
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
} AppInput;

/* Zero initialization is valid. Application owns lifetime and focus. */
void app_input_cancel(AppInput *input, SiteHelperEditor *editor);
/* Refresh after pointer/context changes; drops focus if placement disappeared. */
void app_input_refresh(AppInput *input, SiteHelperEditor *editor);
int app_input_wants_text(const AppInput *input, const SiteHelperEditor *editor);
/* Focus routing precedes global shortcut decoding. COMMAND returns an ordinary
 * EditorAction; caller executes history, completes/reconciles, then refreshes.
 * A failed command leaves text/focus available for correction and retry. */
AppInputResult app_input_route(AppInput *input, SiteHelperEditor *editor,
    const PlatformEvent *event, EditorAction *action);
int app_input_valid(const AppInput *input);
const char *app_input_feedback(const AppInput *input);

#endif
