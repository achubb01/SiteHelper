#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "app_input.h"
#include "command_history.h"

static AppInputResult key(AppInput *input, SiteHelperEditor *editor, PlatformKey keycode, int modifiers, EditorAction *action)
{
    PlatformEvent event = {.type = PLATFORM_EVENT_KEY_DOWN,
        .data.key_down = {.key = keycode, .modifiers = modifiers}};
    return app_input_route(input, editor, &event, action);
}
static void text(AppInput *input, SiteHelperEditor *editor, const char *text)
{
    PlatformEvent event = {.type = PLATFORM_EVENT_TEXT_INPUT};
    snprintf(event.data.text_input.text, sizeof event.data.text_input.text, "%s", text);
    EditorAction action;
    assert(app_input_route(input, editor, &event, &action) == APP_INPUT_CONSUMED);
    assert(action.kind == EDITOR_ACTION_NONE);
}
static void begin(SiteHelperEditor *editor, SiteHelperProject *project)
{
    EditorAction action;
    assert(sitehelper_editor_set_active_tool(editor, EDITOR_TOOL_WALL));
    sitehelper_editor_pointer_move_in_project(editor, project, (Vec2){0,0});
    assert(sitehelper_editor_primary_action_in_project(editor, project, (Vec2){0,0}, &action));
    assert(action.kind == EDITOR_ACTION_NONE);
}
int main(void)
{
    SiteHelperProject project;
    SiteHelperEditor editor;
    SiteHelperCommandHistory history;
    AppInput input = {0};
    sitehelper_project_init(&project);
    sitehelper_editor_init(&editor);
    sitehelper_command_history_init(&history);
    DomainId storey = sitehelper_project_add_storey(&project, 0);
    assert(sitehelper_editor_set_current_storey(&editor, &project, storey));
    EditorAction action;
    assert(!app_input_wants_text(&input, &editor));
    begin(&editor, &project);
    assert(app_input_wants_text(&input, &editor));
    DomainId next_id = project.domain_ids.next;
    text(&input, &editor, "4200");
    assert(input.focus == APP_KEYBOARD_FOCUS_TOOL_LENGTH);
    assert(input.length_status == WALL_LENGTH_DIRECTIONLESS);
    assert(key(&input, &editor, PLATFORM_KEY_ENTER, 0, &action) == APP_INPUT_CONSUMED);
    assert(project.storeys[0].structure.wall_count == 0 && history.count == 0);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){333,777});
    app_input_refresh(&input, &editor);
    assert(app_input_valid(&input));
    WallPlanSegment preview;
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(wall_plan_segment_length_mm(preview) == 4200);
    assert(hypot(preview.end.x - 333.0 / hypot(333, 777) * 4200,
        preview.end.y - 777.0 / hypot(333, 777) * 4200) <= 1.415);
    const PlatformKey suppressed[] = {PLATFORM_KEY_TAB, PLATFORM_KEY_UP, PLATFORM_KEY_DOWN_ARROW, PLATFORM_KEY_Z, PLATFORM_KEY_Y};
    for (size_t i = 0; i < sizeof suppressed / sizeof *suppressed; i++) {
        assert(key(&input, &editor, suppressed[i], PLATFORM_MODIFIER_CTRL, &action) == APP_INPUT_CONSUMED);
    }
    assert(key(&input, &editor, PLATFORM_KEY_LEFT, 0, &action) == APP_INPUT_CONSUMED && input.text.cursor == 3);
    assert(key(&input, &editor, PLATFORM_KEY_RIGHT, 0, &action) == APP_INPUT_CONSUMED && input.text.cursor == 4);
    assert(key(&input, &editor, PLATFORM_KEY_ESCAPE, 0, &action) == APP_INPUT_CONSUMED);
    assert(input.focus == APP_KEYBOARD_FOCUS_NONE && sitehelper_editor_has_wall_preview(&editor));
    assert(sitehelper_editor_get_wall_preview_segment(&editor, &preview));
    assert(preview.end.x == 300 && preview.end.y == 800); /* Normal grid preview restored. */
    assert(project.domain_ids.next == next_id && history.count == 0);
    assert(key(&input, &editor, PLATFORM_KEY_ESCAPE, 0, &action) == APP_INPUT_CONSUMED);
    assert(!sitehelper_editor_has_wall_preview(&editor) && editor.active_tool == EDITOR_TOOL_WALL);
    assert(!app_input_wants_text(&input, &editor));
    assert(key(&input, &editor, PLATFORM_KEY_LEFT, 0, &action) == APP_INPUT_PAN_LEFT);
    assert(key(&input, &editor, PLATFORM_KEY_RIGHT, 0, &action) == APP_INPUT_PAN_RIGHT);
    assert(key(&input, &editor, PLATFORM_KEY_UP, 0, &action) == APP_INPUT_PAN_UP);
    assert(key(&input, &editor, PLATFORM_KEY_DOWN_ARROW, 0, &action) == APP_INPUT_PAN_DOWN);
    assert(key(&input, &editor, PLATFORM_KEY_TAB, 0, &action) == APP_INPUT_SWITCH_VIEW);
    assert(key(&input, &editor, PLATFORM_KEY_Z, PLATFORM_MODIFIER_CTRL, &action) == APP_INPUT_UNDO);
    assert(key(&input, &editor, PLATFORM_KEY_Y, PLATFORM_MODIFIER_CTRL, &action) == APP_INPUT_REDO);
    assert(key(&input, &editor, PLATFORM_KEY_Z, PLATFORM_MODIFIER_CTRL|PLATFORM_MODIFIER_SHIFT, &action) == APP_INPUT_REDO);

    begin(&editor, &project);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){333,777});
    const char *invalid[] = {"0", "-1", "0.2mm", "nan", "4.2mx", "999999999999999999999"};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        text(&input, &editor, invalid[i]);
        assert(!app_input_valid(&input));
        assert(key(&input, &editor, PLATFORM_KEY_ENTER, 0, &action) == APP_INPUT_CONSUMED);
        assert(action.kind == EDITOR_ACTION_NONE && history.count == 0 && project.domain_ids.next == next_id);
        app_input_cancel(&input, &editor);
    }
    text(&input, &editor, "4200");
    PlatformEvent overflow = {.type = PLATFORM_EVENT_TEXT_INPUT, .data.text_input = {.overflow = 1}};
    assert(app_input_route(&input, &editor, &overflow, &action) == APP_INPUT_CONSUMED);
    assert(!app_input_valid(&input) && input.edit_status == TEXT_EDIT_FULL);
    assert(key(&input, &editor, PLATFORM_KEY_ENTER, 0, &action) == APP_INPUT_CONSUMED);
    assert(strcmp(input.text.text, "4200") == 0 && history.count == 0);
    app_input_cancel(&input, &editor);
    text(&input, &editor, "4.2mx");
    assert(key(&input, &editor, PLATFORM_KEY_BACKSPACE, 0, &action) == APP_INPUT_CONSUMED);
    assert(strcmp(input.text.text, "4.2m") == 0 && app_input_valid(&input));
    assert(key(&input, &editor, PLATFORM_KEY_ENTER, 0, &action) == APP_INPUT_COMMAND);
    assert(action.kind == EDITOR_ACTION_COMMAND && action.command.type == SITEHELPER_COMMAND_ADD_WALL);
    WallPlanSegment segment = action.command.data.wall.segment;
    assert(wall_plan_segment_length_mm(segment) == 4200);
    assert(project.domain_ids.next == next_id && project.storeys[0].structure.wall_count == 0);
    SiteHelperCommandResult result;
    assert(sitehelper_command_history_execute(&history, &project, &action.command, &result));
    sitehelper_editor_complete_action(&editor, &action, &result);
    sitehelper_editor_reconcile(&editor, &project);
    app_input_refresh(&input, &editor);
    assert(!app_input_wants_text(&input, &editor) && input.focus == APP_KEYBOARD_FOCUS_NONE);
    assert(history.count == 1 && history.entries[0].command.type == SITEHELPER_COMMAND_ADD_WALL);
    assert(project.storeys[0].structure.wall_count == 1);
    assert(wall_plan_segment_length_mm(project.storeys[0].structure.walls[0].definition.segment) == 4200);
    assert(sitehelper_command_history_undo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(project.storeys[0].structure.wall_count == 0);
    assert(sitehelper_command_history_redo(&history, &project));
    sitehelper_editor_reconcile(&editor, &project);
    assert(project.storeys[0].structure.wall_count == 1);
    WallPlanSegment restored = project.storeys[0].structure.walls[0].definition.segment;
    assert(restored.start.x == segment.start.x && restored.start.y == segment.start.y &&
        restored.end.x == segment.end.x && restored.end.y == segment.end.y);

    begin(&editor, &project);
    sitehelper_editor_pointer_move_in_project(&editor, &project, (Vec2){1234,567});
    assert(sitehelper_editor_primary_action_in_project(&editor, &project, (Vec2){1234,567}, &action));
    assert(action.kind == EDITOR_ACTION_COMMAND && action.command.type == SITEHELPER_COMMAND_ADD_WALL);
    assert(action.command.data.wall.segment.end.x == 1200 && action.command.data.wall.segment.end.y == 600);
    /* External context transitions relinquish focus, too. */
    text(&input, &editor, "4200");
    sitehelper_editor_pointer_leave(&editor);
    app_input_refresh(&input, &editor);
    assert(input.focus == APP_KEYBOARD_FOCUS_NONE);
    begin(&editor, &project); text(&input, &editor, "4200");
    assert(sitehelper_editor_set_active_view(&editor, EDITOR_VIEW_WALL_ELEVATION));
    app_input_refresh(&input, &editor);
    assert(input.focus == APP_KEYBOARD_FOCUS_NONE);
    sitehelper_command_history_destroy(&history);
    sitehelper_project_destroy(&project);
    puts("application input tests passed");
}
