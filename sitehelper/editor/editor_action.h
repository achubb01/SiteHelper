#ifndef EDITOR_ACTION_H
#define EDITOR_ACTION_H

#include "opening_command.h"
#include "sitehelper_command.h"

typedef enum
{
    EDITOR_ACTION_NONE = 0,
    EDITOR_ACTION_COMMAND,

    EDITOR_ACTION_COUNT
} EditorActionKind;

typedef struct
{
    EditorActionKind kind;

    SiteHelperCommand command;
} EditorAction;

/* An action returned with COMMAND owns its command payload. Execute/clone it,
 * then destroy exactly once. Output arguments must be fresh/zero or previously
 * destroyed; action-producing APIs do not replace a live owning action. */
void editor_action_destroy(EditorAction *action);

#endif
