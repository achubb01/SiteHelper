#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./commandmenu.h"
#include "./commandmenu_internal.h"

#define COMMAND_INPUT_LENGTH 256

typedef enum {
    COMMAND_VALID,
    COMMAND_INVALID_NULL,
    COMMAND_INVALID_NAME,
    COMMAND_INVALID_TYPE,
    COMMAND_INVALID_ACTION,
    COMMAND_INVALID_CHILDREN,
    COMMAND_DUPLICATE_NAME,
    COMMAND_RESERVED_NAME
} CommandValidationStatus;

typedef struct {
    CommandValidationStatus type;
    const Command *command;
} CommandValidation;

typedef enum {
    COMMAND_INPUT_OK,
    COMMAND_INPUT_TOO_LONG,
    COMMAND_INPUT_END
} CommandInputResult;

static int command_handle_result(
    Command **current,
    CommandResult result,
    void *context
);

static int command_name_equal(
    const char *first,
    const char *second
);

static CommandValidation command_validate(
    const Command *command,
    int is_root
);

static CommandValidation command_validate_child_names(
    const Command *command,
    int is_root
);

static void command_print(const Command *current);

static void command_link_children(Command *menu);

int command_prefix_match(
    const char *input,
    const char *command
);

static CommandInputResult command_read_input(
    char *buffer,
    size_t buffer_size
);

CommandResult command_select(
    Command *current,
    const char *input
);

static CommandValidation command_validation(
    CommandValidationStatus type,
    const Command *command
);

static const char *command_validation_message(
    CommandValidationStatus result
);

static void command_execute(
    const Command *command,
    void *context
);

void command_run(Command *root, void *context)
{
    if (root == NULL || root->type != COMMAND_MENU) {
        fprintf(
            stderr,
            "Invalid command tree: "
            "root must be a menu\n"
        );

        return;
    }

    CommandValidation validation =
        command_validate(root, 1);

    if (validation.type != COMMAND_VALID) {
        const char *command_name = "(unknown)";

        if (validation.command != NULL &&
            validation.command->name != NULL) {
            command_name = validation.command->name;
        }

        fprintf(
            stderr,
            "Invalid command \"%s\": %s\n",
            command_name,
            command_validation_message(
                validation.type
            )
        );

        return;
    }

    command_link_children(root);

    Command *current = root;
    int running = 1;

    while (running) {
        command_print(current);

        char input[COMMAND_INPUT_LENGTH];

        printf("> ");

        CommandInputResult input_result =
            command_read_input(input, sizeof input);

        if (input_result == COMMAND_INPUT_END) {
            break;
        }

        if (input_result == COMMAND_INPUT_TOO_LONG) {
            printf("Input is too long\n");
            continue;
        }

        CommandResult result =
            command_select(current, input);

        running =
            command_handle_result(&current, result, context);
    }
}

static CommandValidation command_validate(
    const Command *command,
    int is_root
)
{
    if (command == NULL) {
        return command_validation(
            COMMAND_INVALID_NULL,
            NULL
        );
    }

    if (command->name == NULL ||
        command->name[0] == '\0') {
        return command_validation(
            COMMAND_INVALID_NAME,
            command
        );
    }

    if (command->type != COMMAND_MENU &&
        command->type != COMMAND_ACTION) {
        return command_validation(
            COMMAND_INVALID_TYPE,
            command
        );
    }

    if (command->type == COMMAND_ACTION) {
        if (command->target.function == NULL) {
            return command_validation(
                COMMAND_INVALID_ACTION,
                command
            );
        }

        return command_validation(
            COMMAND_VALID,
            NULL
        );
    }

    if (command->target.menu.child_count > 0 &&
        command->target.menu.children == NULL) {
        return command_validation(
            COMMAND_INVALID_CHILDREN,
            command
        );
    }

    CommandValidation name_validation =
        command_validate_child_names(
            command,
            is_root
        );

    if (name_validation.type != COMMAND_VALID) {
        return name_validation;
    }

    for (size_t i = 0;
         i < command->target.menu.child_count;
         i++) {

        const Command *child =
            &command->target.menu.children[i];

        CommandValidation result =
            command_validate(
                child,
                0
            );

        if (result.type != COMMAND_VALID) {
            return result;
        }
    }

    return command_validation(
        COMMAND_VALID,
        NULL
    );
}

static const char *command_validation_message(
    CommandValidationStatus result
)
{
    switch (result) {
        case COMMAND_VALID:
            return "valid command tree";

        case COMMAND_INVALID_NULL:
            return "null command pointer";

        case COMMAND_INVALID_NAME:
            return "command has no name";

        case COMMAND_INVALID_TYPE:
            return "command has an invalid type";

        case COMMAND_INVALID_ACTION:
            return "action has no function";

        case COMMAND_INVALID_CHILDREN:
            return "menu has a child count but no child array";

        case COMMAND_DUPLICATE_NAME:
            return "duplicate command name";

        case COMMAND_RESERVED_NAME:
            return "command uses a reserved navigation name";
    }

    return "unknown validation error";
}

static CommandValidation command_validation(
    CommandValidationStatus type,
    const Command *command
)
{
    return (CommandValidation) {
        .type = type,
        .command = command
    };
}

static int command_handle_result(
    Command **current,
    CommandResult result,
    void *context
)
{
    switch (result.type) {
        case COMMAND_RESULT_MATCH:
            if (result.command->type == COMMAND_MENU) {
                *current = result.command;
            } else {
                command_execute(
                    result.command,
                    context
                );
            }

            return 1;

        case COMMAND_RESULT_BACK:
            *current = (*current)->previous;
            return 1;

        case COMMAND_RESULT_QUIT:
            return 0;

        case COMMAND_RESULT_UNKNOWN:
            printf("Unknown option\n");
            return 1;

        case COMMAND_RESULT_AMBIGUOUS:
            printf("Ambiguous option\n");
            return 1;
    }

    return 0;
}

static int command_name_equal(
    const char *first,
    const char *second
)
{
    if (first == NULL || second == NULL) {
        return 0;
    }

    size_t first_length = strlen(first);
    size_t second_length = strlen(second);

    if (first_length != second_length) {
        return 0;
    }

    for (size_t i = 0; i < first_length; i++) {
        if (tolower((unsigned char)first[i]) !=
            tolower((unsigned char)second[i])) {
            return 0;
        }
    }

    return 1;
}

static CommandValidation command_validate_child_names(
    const Command *menu,
    int is_root
)
{
    size_t child_count =
        menu->target.menu.child_count;

    const Command *children =
        menu->target.menu.children;

    const char *reserved_name =
        is_root ? "Quit" : "Back";

    for (size_t i = 0; i < child_count; i++) {
        if (command_name_equal(
                children[i].name,
                reserved_name
            )) {
            return command_validation(
                COMMAND_RESERVED_NAME,
                &children[i]
            );
        }

        for (size_t j = i + 1;
             j < child_count;
             j++) {

            if (command_name_equal(
                    children[i].name,
                    children[j].name
                )) {
                return command_validation(
                    COMMAND_DUPLICATE_NAME,
                    &children[j]
                );
            }
        }
    }

    return command_validation(
        COMMAND_VALID,
        NULL
    );
}

static void command_execute(
    const Command *command,
    void *context
)
{
    if (command == NULL ||
        command->type != COMMAND_ACTION ||
        command->target.function == NULL) {
        return;
    }

    command->target.function(context);
}

static void command_print(const Command *current)
{
    printf("\n%s\n\n", current->name);

    for (size_t i = 0;
         i < current->target.menu.child_count;
         i++) {
        printf(
            "%zu. %s\n",
            i + 1,
            current->target.menu.children[i].name
        );
    }

    size_t final_option =
        current->target.menu.child_count + 1;

    if (current->previous == NULL) {
        printf("%zu. Quit\n", final_option);
    } else {
        printf("%zu. Back\n", final_option);
    }
}

static void command_link_children(Command *menu) {
    if (menu == NULL || menu->type != COMMAND_MENU) {
        return;
    }

    for (size_t i = 0; i < menu->target.menu.child_count; i++) {
        Command *child = &menu->target.menu.children[i];

        child->previous = menu;

        if (child->type == COMMAND_MENU) {
            command_link_children(child);
        }
    }
}

static CommandInputResult command_read_input(
    char *buffer,
    size_t buffer_size
)
{
    if (buffer == NULL || buffer_size < 2) {
        return COMMAND_INPUT_END;
    }

    if (fgets(buffer, buffer_size, stdin) == NULL) {
        return COMMAND_INPUT_END;
    }

    size_t newline_position =
        strcspn(buffer, "\n");

    if (buffer[newline_position] == '\n') {
        buffer[newline_position] = '\0';
        return COMMAND_INPUT_OK;
    }

    /*
     * If EOF has already been reached, this may simply be the final
     * line of input without a newline.
     */
    if (feof(stdin)) {
        return COMMAND_INPUT_OK;
    }

    int character;

    while ((character = getchar()) != '\n' &&
           character != EOF) {
    }

    buffer[0] = '\0';

    return COMMAND_INPUT_TOO_LONG;
}

int command_prefix_match(const char *input, const char *command)
{
    /* 1. Gather information */
    size_t input_length = strlen(input);
    size_t command_length = strlen(command);

    /* 2. Reject impossible cases */
    if (input_length > command_length) {
        return 0;
    }

    /* 3. Compare characters */
    for (size_t i = 0; i < input_length; i++) {

        if (tolower((unsigned char)input[i]) !=
            tolower((unsigned char)command[i])) {

            return 0;
        }
    }

    /* 4. Everything matched */
    return 1;
}

CommandResult command_select(
    Command *current,
    const char *input
)
{
    size_t child_count =
        current->target.menu.child_count;
        
    /*
     * Try interpreting the input as a number.
     */
    char *end;
    long selection = strtol(input, &end, 10);

    if (*input != '\0' && *end == '\0') {
        if (selection >= 1 &&
            selection <= (long)child_count) {

            return (CommandResult) {
                .type = COMMAND_RESULT_MATCH,
                .command =
                    &current->target.menu.children[
                        selection - 1
                    ]
            };
        }

        if (selection == (long)child_count + 1) {
            if (current->previous == NULL) {
                return (CommandResult) {
                    .type = COMMAND_RESULT_QUIT,
                    .command = NULL
                };
            }

            return (CommandResult) {
                .type = COMMAND_RESULT_BACK,
                .command = NULL
            };
        }

        return (CommandResult) {
            .type = COMMAND_RESULT_UNKNOWN,
            .command = NULL
        };
    }

    /*
    * Input was not numeric, so try prefix matching.
    */
    Command *match = NULL;
    size_t input_length = strlen(input);

    if (input_length == 0) {
        return (CommandResult) {
            .type = COMMAND_RESULT_UNKNOWN,
            .command = NULL
        };
    }

    for (size_t i = 0; i < child_count; i++) {
        Command *child =
            &current->target.menu.children[i];

        if (command_prefix_match(input, child->name)) {
            if (match != NULL) {
                return (CommandResult) {
                    .type = COMMAND_RESULT_AMBIGUOUS,
                    .command = NULL
                };
            }

            match = child;
        }
    }

    /*
    * Back exists only inside a submenu.
    */
    if (current->previous != NULL &&
        command_prefix_match(input, "back")) {

        if (match != NULL) {
            return (CommandResult) {
                .type = COMMAND_RESULT_AMBIGUOUS,
                .command = NULL
            };
        }

        return (CommandResult) {
            .type = COMMAND_RESULT_BACK,
            .command = NULL
        };
    }

    /*
    * Quit exists only at the root.
    */
    if (current->previous == NULL &&
        command_prefix_match(input, "quit")) {

        if (match != NULL) {
            return (CommandResult) {
                .type = COMMAND_RESULT_AMBIGUOUS,
                .command = NULL
            };
        }

        return (CommandResult) {
            .type = COMMAND_RESULT_QUIT,
            .command = NULL
        };
    }

    if (match != NULL) {
        return (CommandResult) {
            .type = COMMAND_RESULT_MATCH,
            .command = match
        };
    }

    return (CommandResult) {
        .type = COMMAND_RESULT_UNKNOWN,
        .command = NULL
    };
}

