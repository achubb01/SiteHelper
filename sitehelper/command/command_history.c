#include <stdlib.h>

#include "command_history.h"


static int
sitehelper_command_history_reserve(
    SiteHelperCommandHistory *history,
    size_t required_capacity
)
{
    if (history == NULL) {
        return 0;
    }

    if (
        required_capacity
        <= history->capacity
    ) {
        return 1;
    }

    size_t new_capacity =
        history->capacity == 0
        ? 8
        : history->capacity * 2;

    while (
        new_capacity
        < required_capacity
    ) {
        new_capacity *= 2;
    }

    SiteHelperCommandHistoryEntry *entries =
        realloc(
            history->entries,
            new_capacity
                * sizeof *entries
        );

    if (entries == NULL) {
        return 0;
    }

    history->entries =
        entries;

    history->capacity =
        new_capacity;

    return 1;
}


void
sitehelper_command_history_init(
    SiteHelperCommandHistory *history
)
{
    if (history == NULL) {
        return;
    }

    *history =
        (SiteHelperCommandHistory){0};
}


void
sitehelper_command_history_destroy(
    SiteHelperCommandHistory *history
)
{
    if (history == NULL) {
        return;
    }

    free(
        history->entries
    );

    *history =
        (SiteHelperCommandHistory){0};
}


int
sitehelper_command_history_execute(
    SiteHelperCommandHistory *history,
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
)
{
    if (
        history == NULL
        || project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    /*
     * A failed history execution must never
     * leave stale successful result data.
     */
    *result =
        (SiteHelperCommandResult){
            .type =
                SITEHELPER_COMMAND_NONE
        };

    /*
     * Reserve history storage before executing
     * the command.
     *
     * Once the command commits successfully,
     * recording it must not be capable of
     * failing due to allocation.
     */
    if (!sitehelper_command_history_reserve(
            history,
            history->count + 1)) {
        return 0;
    }

    if (!sitehelper_command_execute(
            project,
            command,
            result)) {
        return 0;
    }

    history->entries[
        history->count
    ] =
        (SiteHelperCommandHistoryEntry){
            .command =
                *command,

            .result =
                *result
        };

    history->count++;

    return 1;
}


int
sitehelper_command_history_undo(
    SiteHelperCommandHistory *history,
    SiteHelperProject *project
)
{
    if (
        history == NULL
        || project == NULL
        || history->count == 0
    ) {
        return 0;
    }

    SiteHelperCommandHistoryEntry *entry =
        &history->entries[
            history->count - 1
        ];

    /*
     * Do not remove the history entry until
     * the domain mutation has been successfully
     * reversed.
     */
    if (!sitehelper_command_undo(
            project,
            &entry->command,
            &entry->result)) {
        return 0;
    }

    history->count--;

    history->entries[
        history->count
    ] =
        (SiteHelperCommandHistoryEntry){0};

    return 1;
}