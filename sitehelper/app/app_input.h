#ifndef APP_INPUT_H
#define APP_INPUT_H

#include "platform_event.h"
#include "text_edit.h"
#include "length_parse.h"
#include "editor_properties.h"

typedef enum {
    APP_KEYBOARD_FOCUS_NONE,
    APP_KEYBOARD_FOCUS_TOOL_LENGTH,
    APP_KEYBOARD_FOCUS_PROPERTY_MM,
    APP_KEYBOARD_FOCUS_PLAN_NOTE,
    APP_KEYBOARD_FOCUS_PLAN_CALLOUT
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
    EditorSelectionScope property_target_scope;
    DomainId property_wall_id;
    DomainId property_opening_id;
    DomainId property_slab_id;
    size_t property_feature_index;
    /* A property field is seeded with its current value. The first committed
     * text-input event replaces that seed; cursor/editing keys opt into editing
     * the seeded text in place instead. */
    int replace_on_next_text_input;

    /* Plan-note authoring captures stable identity/context, never pointers. A
     * zero annotation_id means creation; otherwise the existing note is edited
     * in place while preserving its weak physical target association. */
    DomainId note_storey_id;
    DomainId note_annotation_id;
    DomainId note_target_id;
    PlanPosition note_position;

    /* Plan-callout text focus captures authored geometry and optional existing
     * identity, never a borrowed document pointer. */
    DomainId callout_storey_id;
    DomainId callout_id;
    PlanPosition callout_target;
    PlanPosition callout_label_anchor;
} AppInput;

/* Zero initialization is valid. Application owns lifetime and focus. */
void app_input_cancel(AppInput *input, SiteHelperEditor *editor);
/* Start numeric editing from the selected object's current authoritative value. */
int app_input_begin_property(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperty property);
/* Begin Note-tool authoring at the click position. Clicking an existing note
 * edits it; otherwise a new note begins at the editor-resolved snapped point. */
int app_input_begin_plan_note(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 view_position);
/* Begin text entry after the Callout tool has authored target + label points. */
int app_input_begin_plan_callout(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project);
/* Begin text editing for the currently selected callout without relocating it. */
int app_input_begin_selected_plan_callout(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project);
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
