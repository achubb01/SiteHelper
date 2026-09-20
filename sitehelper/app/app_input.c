#include "app_input.h"
#include <stdio.h>
#include <string.h>

static int property_target_matches(const AppInput *input,
    const SiteHelperEditor *editor)
{
    if (input == NULL || editor == NULL ||
        input->focus != APP_KEYBOARD_FOCUS_PROPERTY_MM) { return 0; }
    const EditorSelection *s=&editor->selection;
    if (s->kind != input->property_target_kind ||
        s->scope != EDITOR_SELECTION_SCOPE_PLAN ||
        s->slab_id != input->property_slab_id) { return 0; }
    if (s->kind == EDITOR_SELECTION_SLAB) { return 1; }
    return (s->kind == EDITOR_SELECTION_SLAB_REGION ||
        s->kind == EDITOR_SELECTION_SLAB_EDGE_REBATE) &&
        s->slab_feature_index == input->property_feature_index;
}

void app_input_cancel(AppInput *input, SiteHelperEditor *editor)
{
    if (input == NULL) { return; }
    if (input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH) {
        sitehelper_editor_clear_wall_length(editor);
    }
    if (input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) {
        sitehelper_editor_reset_plan_callout_tool(editor);
    }
    *input = (AppInput){0};
}

int app_input_begin_property(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperty property)
{
    if (input == NULL || editor == NULL || project == NULL) { return 0; }
    int value;
    if (!sitehelper_editor_property_millimetres(editor,project,property,&value)) { return 0; }
    EditorSelection selection=editor->selection;
    if (selection.kind != EDITOR_SELECTION_SLAB &&
        selection.kind != EDITOR_SELECTION_SLAB_REGION &&
        selection.kind != EDITOR_SELECTION_SLAB_EDGE_REBATE) { return 0; }
    app_input_cancel(input,editor);
    input->focus=APP_KEYBOARD_FOCUS_PROPERTY_MM;
    input->property=property;
    input->property_target_kind=selection.kind;
    input->property_slab_id=selection.slab_id;
    input->property_feature_index=selection.kind == EDITOR_SELECTION_SLAB ? SIZE_MAX :
        selection.slab_feature_index;
    input->replace_on_next_text_input=1;
    text_edit_begin(&input->text);
    char text[32];
    snprintf(text,sizeof text,"%d",value);
    input->edit_status=text_edit_insert(&input->text,text);
    input->parse_status=length_parse_mm(input->text.text,&input->millimetres);
    return input->edit_status == TEXT_EDIT_OK && input->parse_status == LENGTH_PARSE_OK;
}

int app_input_begin_plan_note(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, Vec2 view_position)
{
    if (input == NULL || editor == NULL || project == NULL) { return 0; }
    DomainId annotation_id=DOMAIN_ID_INVALID;
    PlanPosition position;
    if (!sitehelper_editor_prepare_plan_note_authoring(editor,project,view_position,
            &annotation_id,&position)) { return 0; }
    const DocumentAnnotation *note=NULL;
    if (annotation_id != DOMAIN_ID_INVALID) {
        note=sitehelper_project_find_annotation_by_id_const(project,annotation_id);
        if (note == NULL || note->kind != DOCUMENT_ANNOTATION_NOTE) { return 0; }
    }
    app_input_cancel(input,editor);
    input->focus=APP_KEYBOARD_FOCUS_PLAN_NOTE;
    input->note_storey_id=editor->current_storey_id;
    input->note_annotation_id=annotation_id;
    input->note_position=position;
    input->note_target_id=note != NULL ? note->target_id : DOMAIN_ID_INVALID;
    input->edit_status=TEXT_EDIT_OK;
    text_edit_begin(&input->text);
    if (note != NULL) {
        input->edit_status=text_edit_insert_multiline(&input->text,note->text);
    }
    return input->edit_status == TEXT_EDIT_OK;
}

int app_input_begin_plan_callout(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project)
{
    if (input == NULL || editor == NULL || project == NULL ||
        sitehelper_project_find_storey_by_id_const(project,editor->current_storey_id) == NULL) {
        return 0;
    }
    PlanPosition target,label;
    if (!sitehelper_editor_get_plan_callout_ready(editor,&target,&label)) { return 0; }
    app_input_cancel(input,editor);
    input->focus=APP_KEYBOARD_FOCUS_PLAN_CALLOUT;
    input->callout_storey_id=editor->current_storey_id;
    input->callout_id=DOMAIN_ID_INVALID;
    input->callout_target=target;
    input->callout_label_anchor=label;
    input->edit_status=TEXT_EDIT_OK;
    text_edit_begin(&input->text);
    return 1;
}

int app_input_begin_selected_plan_callout(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project)
{
    if (input == NULL || editor == NULL || project == NULL ||
        !editor_selection_is_document_kind(&editor->selection,DOCUMENT_OBJECT_CALLOUT)) { return 0; }
    const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(
        project,editor->selection.document.id);
    if (callout == NULL || callout->storey_id != editor->current_storey_id) { return 0; }
    app_input_cancel(input,editor);
    input->focus=APP_KEYBOARD_FOCUS_PLAN_CALLOUT;
    input->callout_storey_id=callout->storey_id;
    input->callout_id=callout->id;
    input->callout_target=callout->target;
    input->callout_label_anchor=callout->label_anchor;
    text_edit_begin(&input->text);
    input->edit_status=text_edit_insert_multiline(&input->text,callout->text);
    return input->edit_status == TEXT_EDIT_OK;
}

int app_input_wants_text(const AppInput *input, const SiteHelperEditor *editor)
{
    return input != NULL && (input->focus != APP_KEYBOARD_FOCUS_NONE ||
        sitehelper_editor_has_wall_preview(editor));
}

void app_input_refresh_in_project(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project)
{
    if (input == NULL) { return; }
    if (input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) {
        if (project == NULL || editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
            editor->active_tool != EDITOR_TOOL_CALLOUT ||
            editor->current_storey_id != input->callout_storey_id) {
            app_input_cancel(input,editor);
            return;
        }
        if (input->callout_id != DOMAIN_ID_INVALID) {
            const DocumentPlanCallout *callout=sitehelper_project_find_callout_by_id_const(
                project,input->callout_id);
            if (callout == NULL || callout->storey_id != input->callout_storey_id) {
                app_input_cancel(input,editor);
                return;
            }
        }
        return;
    }
    if (input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE) {
        if (project == NULL || editor == NULL || editor->active_view != EDITOR_VIEW_PLAN ||
            editor->active_tool != EDITOR_TOOL_NOTE ||
            editor->current_storey_id != input->note_storey_id) {
            app_input_cancel(input,editor);
            return;
        }
        if (input->note_annotation_id != DOMAIN_ID_INVALID) {
            const DocumentAnnotation *note=sitehelper_project_find_annotation_by_id_const(
                project,input->note_annotation_id);
            if (note == NULL || note->kind != DOCUMENT_ANNOTATION_NOTE ||
                note->anchor.storey_id != input->note_storey_id) {
                app_input_cancel(input,editor);
                return;
            }
        }
        return;
    }
    if (input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM) {
        int current;
        if (project == NULL || !property_target_matches(input,editor) ||
            !sitehelper_editor_property_millimetres(editor,project,input->property,&current)) {
            app_input_cancel(input,editor);
            return;
        }
        input->parse_status=length_parse_mm(input->text.text,&input->millimetres);
        return;
    }
    if (input->focus != APP_KEYBOARD_FOCUS_TOOL_LENGTH) { return; }
    if (!sitehelper_editor_has_wall_preview(editor)) { app_input_cancel(input, editor); return; }
    input->parse_status = length_parse_mm(input->text.text, &input->millimetres);
    sitehelper_editor_clear_wall_length(editor);
    input->length_status = WALL_LENGTH_NONPOSITIVE;
    if (input->edit_status == TEXT_EDIT_OK && input->parse_status == LENGTH_PARSE_OK) {
        input->length_status = sitehelper_editor_set_wall_length(editor, input->millimetres);
    }
}

void app_input_refresh(AppInput *input, SiteHelperEditor *editor)
{
    app_input_refresh_in_project(input,editor,NULL);
}

int app_input_valid(const AppInput *input)
{
    if (input == NULL || input->edit_status != TEXT_EDIT_OK || input->command_failed) { return 0; }
    if (input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE ||
        input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) { return input->text.length != 0; }
    if (input->parse_status != LENGTH_PARSE_OK) { return 0; }
    if (input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM) { return 1; }
    return input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH &&
        input->length_status == WALL_LENGTH_OK;
}

const char *app_input_feedback(const AppInput *input)
{
    if (input->edit_status == TEXT_EDIT_FULL) { return "Input full; edit to continue"; }
    if (input->edit_status != TEXT_EDIT_OK) { return "Unsupported text; edit to continue"; }
    if (input->command_failed) {
        if (input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM) {
            return "Property rejected; edit or retry";
        }
        if (input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE) {
            return "Note rejected; edit or retry";
        }
        if (input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) {
            return "Callout rejected; edit or retry";
        }
        return "Wall creation failed; edit or retry";
    }
    if (input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE) {
        return input->text.length == 0 ? "Type note text   Esc: cancel" :
            "Enter: save   Shift+Enter: new line   Esc: cancel";
    }
    if (input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) {
        return input->text.length == 0 ? "Type callout text   Esc: cancel" :
            "Enter: save callout   Shift+Enter: new line   Esc: cancel";
    }
    switch (input->parse_status) {
        case LENGTH_PARSE_EMPTY: return "Enter a value (mm or m)";
        case LENGTH_PARSE_INVALID: return "Invalid value (mm or m)";
        case LENGTH_PARSE_OVERFLOW: return "Value exceeds integer mm range";
        case LENGTH_PARSE_FRACTIONAL_MM: return "Use whole millimetres";
        default: break;
    }
    if (input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM) {
        return "Enter: apply   Esc: cancel";
    }
    switch (input->length_status) {
        case WALL_LENGTH_NONPOSITIVE: return "Length must be positive";
        case WALL_LENGTH_DIRECTIONLESS: return "Move pointer to establish direction";
        case WALL_LENGTH_OUT_OF_RANGE: return "Endpoint outside coordinate range";
        case WALL_LENGTH_INACTIVE: return "No active Wall start";
        default: return "Enter: create wall   Esc: cancel entry";
    }
}

AppInputResult app_input_route_in_project(AppInput *input, SiteHelperEditor *editor,
    const SiteHelperProject *project, const PlatformEvent *event, EditorAction *action)
{
    if (input == NULL || editor == NULL || event == NULL || action == NULL) { return APP_INPUT_UNHANDLED; }
    *action = (EditorAction){0};
    app_input_refresh_in_project(input,editor,project);
    if (event->type == PLATFORM_EVENT_TEXT_INPUT) {
        if (input->focus == APP_KEYBOARD_FOCUS_NONE) {
            if (!sitehelper_editor_has_wall_preview(editor)) { return APP_INPUT_UNHANDLED; }
            input->focus = APP_KEYBOARD_FOCUS_TOOL_LENGTH;
            text_edit_begin(&input->text);
        }
        input->command_failed = 0;
        if (event->data.text_input.overflow) {
            input->edit_status=TEXT_EDIT_FULL;
        } else if (input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM &&
            input->replace_on_next_text_input) {
            /* Property fields display their authoritative value on focus, but CAD
             * entry should replace that seed when the user simply starts typing.
             * Stage the replacement so an invalid text event cannot destroy the
             * existing editable value. */
            TextEdit candidate=input->text;
            text_edit_clear(&candidate);
            TextEditResult status=text_edit_insert(&candidate,event->data.text_input.text);
            input->edit_status=status;
            if (status == TEXT_EDIT_OK) {
                input->text=candidate;
                input->replace_on_next_text_input=0;
            }
        } else {
            input->edit_status=text_edit_insert(&input->text,event->data.text_input.text);
        }
        app_input_refresh_in_project(input,editor,project);
        return APP_INPUT_CONSUMED;
    }
    if (event->type != PLATFORM_EVENT_KEY_DOWN) { return APP_INPUT_UNHANDLED; }
    PlatformKey key = event->data.key_down.key;
    int modifiers = event->data.key_down.modifiers;
    int repeat = event->data.key_down.repeat;
    if (input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH ||
        input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM ||
        input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE ||
        input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) {
        if (key == PLATFORM_KEY_ESCAPE && !repeat) {
            app_input_cancel(input, editor);
        } else if (key == PLATFORM_KEY_ENTER && !repeat) {
            input->command_failed = 0;
            if ((input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE ||
                 input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT) &&
                (modifiers & PLATFORM_MODIFIER_SHIFT)) {
                input->edit_status=text_edit_insert_newline(&input->text);
                return APP_INPUT_CONSUMED;
            }
            if (app_input_valid(input)) {
                if (input->focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH) {
                    if (sitehelper_editor_create_wall_length_action(editor,input->millimetres,action) == WALL_LENGTH_OK) {
                        return APP_INPUT_COMMAND;
                    }
                } else if (input->focus == APP_KEYBOARD_FOCUS_PLAN_NOTE && project != NULL) {
                    if (input->note_annotation_id != DOMAIN_ID_INVALID) {
                        const DocumentAnnotation *current=sitehelper_project_find_annotation_by_id_const(
                            project,input->note_annotation_id);
                        if (current != NULL && current->anchor.position.x == input->note_position.x &&
                            current->anchor.position.y == input->note_position.y &&
                            current->target_id == input->note_target_id &&
                            strcmp(current->text,input->text.text) == 0) {
                            app_input_cancel(input,editor);
                            return APP_INPUT_CONSUMED;
                        }
                        if (sitehelper_editor_create_edit_plan_note_action(editor,
                                input->note_position,input->note_target_id,input->text.text,action)) {
                            return APP_INPUT_COMMAND;
                        }
                    } else if (sitehelper_editor_create_plan_note_action(editor,
                            input->note_position,input->note_target_id,input->text.text,action)) {
                        return APP_INPUT_COMMAND;
                    }
                } else if (input->focus == APP_KEYBOARD_FOCUS_PLAN_CALLOUT && project != NULL) {
                    if (input->callout_id != DOMAIN_ID_INVALID) {
                        const DocumentPlanCallout *current=sitehelper_project_find_callout_by_id_const(
                            project,input->callout_id);
                        if (current != NULL && current->target.x == input->callout_target.x &&
                            current->target.y == input->callout_target.y &&
                            current->label_anchor.x == input->callout_label_anchor.x &&
                            current->label_anchor.y == input->callout_label_anchor.y &&
                            strcmp(current->text,input->text.text) == 0) {
                            app_input_cancel(input,editor);
                            return APP_INPUT_CONSUMED;
                        }
                        if (sitehelper_editor_create_edit_plan_callout_action(editor,
                                input->callout_target,input->callout_label_anchor,
                                input->text.text,action)) { return APP_INPUT_COMMAND; }
                    } else if (sitehelper_editor_create_plan_callout_action(editor,
                            input->callout_target,input->callout_label_anchor,
                            input->text.text,action)) { return APP_INPUT_COMMAND; }
                } else if (project != NULL) {
                    int current;
                    if (sitehelper_editor_property_millimetres(editor,project,input->property,&current) &&
                        current == input->millimetres) {
                        /* Applying the displayed value would create a meaningless
                         * history entry. Treat Enter as a successful close instead. */
                        app_input_cancel(input,editor);
                        return APP_INPUT_CONSUMED;
                    }
                    EditorPropertyEdit edit={.property=input->property,
                        .value.millimetres=input->millimetres};
                    if (sitehelper_editor_create_property_command(editor,project,&edit,&action->command)) {
                        action->kind=EDITOR_ACTION_COMMAND;
                        return APP_INPUT_COMMAND;
                    }
                }
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
            if (input->focus == APP_KEYBOARD_FOCUS_PROPERTY_MM) {
                input->replace_on_next_text_input=0;
            }
            input->edit_status = TEXT_EDIT_OK;
            input->command_failed = 0;
            app_input_refresh_in_project(input,editor,project);
        }
        /* All keys belong to the focus owner, including Ctrl+Z/Y and Tab. */
        return APP_INPUT_CONSUMED;
    }
    if (key == PLATFORM_KEY_ESCAPE && !repeat && sitehelper_editor_cancel_tool_interaction(editor)) {
        return APP_INPUT_CONSUMED;
    }
    if (key == PLATFORM_KEY_ENTER && !repeat && modifiers == PLATFORM_MODIFIER_NONE &&
        sitehelper_editor_create_active_polygon_action(editor,action)) {
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

AppInputResult app_input_route(AppInput *input, SiteHelperEditor *editor,
    const PlatformEvent *event, EditorAction *action)
{
    return app_input_route_in_project(input,editor,NULL,event,action);
}
