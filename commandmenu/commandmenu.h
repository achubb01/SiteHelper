#ifndef COMMANDMENU_H
#define COMMANDMENU_H

#include <stddef.h>

#define ARRAY_COUNT(array) \
    (sizeof(array) / sizeof((array)[0]))

#define ACTION_COMMAND(command_name, command_function) \
    {                                                  \
        .name = (command_name),                        \
        .type = COMMAND_ACTION,                        \
        .target.function = (command_function)          \
    }

#define MENU_COMMAND(command_name, command_children)      \
    {                                                     \
        .name = (command_name),                           \
        .type = COMMAND_MENU,                             \
        .target.menu = {                                  \
            .children = (command_children),               \
            .child_count = ARRAY_COUNT(command_children)  \
        }                                                 \
    }

typedef struct Command Command;
typedef void (*CommandFunction)(void *context);

typedef enum {
    COMMAND_MENU,
    COMMAND_ACTION
} CommandType;

struct Command {
    const char *name;
    CommandType type;
    Command *previous;

    union {
        struct {
            Command *children;
            size_t child_count;
        } menu;

        CommandFunction function;
    } target;
};

void command_run(Command *root, void *context);

#endif