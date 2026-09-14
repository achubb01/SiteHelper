#include "editor_action.h"

void editor_action_destroy(EditorAction *action)
{
    if (action == NULL) { return; }
    sitehelper_command_destroy(&action->command);
    *action=(EditorAction){0};
}
