#include "app_input.h"

void app_input_cancel(AppInput *input, SiteHelperEditor *editor)
{
    if (input == NULL) { return; }
    if (input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH) {
        sitehelper_editor_clear_wall_length(editor);
    }
    *input = (AppInput){0};
}

int app_input_wants_text(const AppInput *input, const SiteHelperEditor *editor)
{
    return input != NULL && (input->focus != APP_KEYBOARD_FOCUS_NONE ||
        sitehelper_editor_has_wall_preview(editor));
}

void app_input_refresh(AppInput *input, SiteHelperEditor *editor)
{
    if (input == NULL || input->focus != APP_KEYBOARD_FOCUS_TOOL_LENGTH) { return; }
    if (!sitehelper_editor_has_wall_preview(editor)) { app_input_cancel(input, editor); return; }
    input->parse_status = length_parse_mm(input->text.text, &input->millimetres);
    sitehelper_editor_clear_wall_length(editor);
    input->length_status = WALL_LENGTH_NONPOSITIVE;
    if (input->edit_status == TEXT_EDIT_OK && input->parse_status == LENGTH_PARSE_OK) {
        input->length_status = sitehelper_editor_set_wall_length(editor, input->millimetres);
    }
}

int app_input_valid(const AppInput *input)
{
    return input != NULL && input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH &&
        input->edit_status == TEXT_EDIT_OK && input->parse_status == LENGTH_PARSE_OK &&
        input->length_status == WALL_LENGTH_OK && !input->command_failed;
}

const char *app_input_feedback(const AppInput *input)
{
    if (input->edit_status == TEXT_EDIT_FULL) { return "Input full; edit to continue"; }
    if (input->edit_status != TEXT_EDIT_OK) { return "Unsupported text; edit to continue"; }
    if (input->command_failed) { return "Wall creation failed; edit or retry"; }
    switch (input->parse_status) {
        case LENGTH_PARSE_EMPTY: return "Enter a length (mm or m)";
        case LENGTH_PARSE_INVALID: return "Invalid length (mm or m)";
        case LENGTH_PARSE_OVERFLOW: return "Length exceeds integer mm range";
        case LENGTH_PARSE_FRACTIONAL_MM: return "Use whole millimetres";
        default: break;
    }
    switch (input->length_status) {
        case WALL_LENGTH_NONPOSITIVE: return "Length must be positive";
        case WALL_LENGTH_DIRECTIONLESS: return "Move pointer to establish direction";
        case WALL_LENGTH_OUT_OF_RANGE: return "Endpoint outside coordinate range";
        case WALL_LENGTH_INACTIVE: return "No active Wall start";
        default: return "Enter: create wall   Esc: cancel entry";
    }
}

AppInputResult app_input_route(AppInput *input, SiteHelperEditor *editor,
    const PlatformEvent *event, EditorAction *action)
{
    if (input == NULL || editor == NULL || event == NULL || action == NULL) { return APP_INPUT_UNHANDLED; }
    *action = (EditorAction){0};
    app_input_refresh(input, editor);
    if (event->type == PLATFORM_EVENT_TEXT_INPUT) {
        if (input->focus == APP_KEYBOARD_FOCUS_NONE) {
            if (!sitehelper_editor_has_wall_preview(editor)) { return APP_INPUT_UNHANDLED; }
            input->focus = APP_KEYBOARD_FOCUS_TOOL_LENGTH;
            text_edit_begin(&input->text);
        }
        input->command_failed = 0;
        input->edit_status = event->data.text_input.overflow ? TEXT_EDIT_FULL :
            text_edit_insert(&input->text, event->data.text_input.text);
        app_input_refresh(input, editor);
        return APP_INPUT_CONSUMED;
    }
    if (event->type != PLATFORM_EVENT_KEY_DOWN) { return APP_INPUT_UNHANDLED; }
    PlatformKey key = event->data.key_down.key;
    int modifiers = event->data.key_down.modifiers;
    int repeat = event->data.key_down.repeat;
    if (input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH) {
        if (key == PLATFORM_KEY_ESCAPE && !repeat) {
            app_input_cancel(input, editor);
        } else if (key == PLATFORM_KEY_ENTER && !repeat) {
            input->command_failed = 0;
            if (app_input_valid(input) &&
                sitehelper_editor_create_wall_length_action(editor, input->millimetres, action) == WALL_LENGTH_OK) {
                return APP_INPUT_COMMAND;
            }
        } else if (!(modifiers & PLATFORM_MODIFIER_CTRL)) {
            TextEditOperation operation;
            switch (key) {
                case PLATFORM_KEY_LEFT: operation = TEXT_EDIT_LEFT; break;
                case PLATFORM_KEY_RIGHT: operation = TEXT_EDIT_RIGHT; break;
                case PLATFORM_KEY_HOME: operation = TEXT_EDIT_HOME; break;
                case PLATFORM_KEY_END: operation = TEXT_EDIT_END; break;
                case PLATFORM_KEY_BACKSPACE: operation = TEXT_EDIT_BACKSPACE; break;
                case PLATFORM_KEY_DELETE: operation = TEXT_EDIT_DELETE; break;
                default: return APP_INPUT_CONSUMED;
            }
            text_edit_apply(&input->text, operation);
            input->edit_status = TEXT_EDIT_OK;
            input->command_failed = 0;
            app_input_refresh(input, editor);
        }
        /* All keys belong to the focus owner, including Ctrl+Z/Y and Tab. */
        return APP_INPUT_CONSUMED;
    }
    if (key == PLATFORM_KEY_ESCAPE && !repeat && sitehelper_editor_cancel_tool_interaction(editor)) {
        return APP_INPUT_CONSUMED;
    }
    if (key == PLATFORM_KEY_ENTER && !repeat && modifiers == PLATFORM_MODIFIER_NONE &&
        sitehelper_editor_create_slab_action(editor,action)) {
        return APP_INPUT_COMMAND;
    }
    if (key == PLATFORM_KEY_DELETE && !repeat && modifiers == PLATFORM_MODIFIER_NONE &&
        sitehelper_editor_create_delete_selection_action(editor,action)) {
        return APP_INPUT_COMMAND;
    }
    if (!repeat && (modifiers & PLATFORM_MODIFIER_CTRL)) {
        if (key == PLATFORM_KEY_Y || (key == PLATFORM_KEY_Z && (modifiers & PLATFORM_MODIFIER_SHIFT))) { return APP_INPUT_REDO; }
        if (key == PLATFORM_KEY_Z) { return APP_INPUT_UNDO; }
    }
    if (key == PLATFORM_KEY_TAB && !repeat && modifiers == PLATFORM_MODIFIER_NONE) { return APP_INPUT_SWITCH_VIEW; }
    switch (key) {
        case PLATFORM_KEY_LEFT: return APP_INPUT_PAN_LEFT;
        case PLATFORM_KEY_RIGHT: return APP_INPUT_PAN_RIGHT;
        case PLATFORM_KEY_UP: return APP_INPUT_PAN_UP;
        case PLATFORM_KEY_DOWN_ARROW: return APP_INPUT_PAN_DOWN;
        default: return APP_INPUT_UNHANDLED;
    }
}
