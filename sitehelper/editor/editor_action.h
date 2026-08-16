#ifndef EDITOR_ACTION_H
#define EDITOR_ACTION_H

#include "opening_command.h"

typedef enum
{
    EDITOR_ACTION_NONE = 0,
    EDITOR_ACTION_OPENING_COMMAND
} EditorActionKind;

typedef struct
{
    EditorActionKind kind;

    OpeningCommand opening_command;
} EditorAction;

#endif