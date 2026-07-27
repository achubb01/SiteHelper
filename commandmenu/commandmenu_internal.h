#ifndef COMMANDMENU_INTERNAL_H
#define COMMANDMENU_INTERNAL_H

#include "commandmenu.h"

typedef enum {
    COMMAND_RESULT_UNKNOWN,
    COMMAND_RESULT_AMBIGUOUS,
    COMMAND_RESULT_MATCH,
    COMMAND_RESULT_BACK,
    COMMAND_RESULT_QUIT
} CommandResultType;

typedef struct {
    CommandResultType type;
    Command *command;
} CommandResult;

int command_prefix_match(
    const char *input,
    const char *command
);

CommandResult command_select(
    Command *current,
    const char *input
);

#endif