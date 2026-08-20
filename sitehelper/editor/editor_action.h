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

#endif