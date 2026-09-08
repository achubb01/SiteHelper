#include <stdlib.h>

#include "command_history.h"
#include "sitehelper_command_internal.h"

static void history_entry_destroy(SiteHelperCommandHistoryEntry *entry)
{
    sitehelper_command_destroy_undo_state(entry->undo_state);
    *entry = (SiteHelperCommandHistoryEntry){0};
}

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

    for (size_t i = 0; i < history->count; i++) {
        history_entry_destroy(&history->entries[i]);
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
    /* Copy before realloc: callers may pass a command from an existing entry. */
    SiteHelperCommandHistoryEntry candidate = { .command = *command };
    if (!sitehelper_command_history_reserve(
            history,
            history->cursor + 1)) {
        return 0;
    }

    if (!sitehelper_command_capture_undo_state(
            project, &candidate.command, &candidate.undo_state) ||
        !sitehelper_command_execute(
            project,
            &candidate.command,
            &candidate.result)) {
        history_entry_destroy(&candidate);
        return 0;
    }

    for (size_t i = history->cursor; i < history->count; i++) {
        history_entry_destroy(&history->entries[i]);
    }
    *result = candidate.result;
    /* Transfer exclusive ownership into the reserved slot. */
    history->entries[history->cursor] = candidate;

    /*
    * The new command replaces any redoable
    * branch beyond the current cursor.
    */
    history->cursor++;

    history->count =
        history->cursor;

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
        || history->cursor == 0
    ) {
        return 0;
    }

    SiteHelperCommandHistoryEntry *entry =
        &history->entries[
            history->cursor - 1
        ];

    /*
     * Do not move the history cursor until
     * the domain mutation has been successfully
     * reversed.
     */
    if (!sitehelper_command_undo_with_state(
            project,
            &entry->command,
            &entry->result,
            entry->undo_state)) {
        return 0;
    }

    history->cursor--;

    return 1;
}

int
sitehelper_command_history_redo(
    SiteHelperCommandHistory *history,
    SiteHelperProject *project
)
{
    if (
        history == NULL
        || project == NULL
        || history->cursor >= history->count
    ) {
        return 0;
    }

    SiteHelperCommandHistoryEntry *entry =
        &history->entries[
            history->cursor
        ];

    /*
     * Do not move the history cursor until
     * the domain mutation has been successfully
     * reapplied.
     */
    if (!sitehelper_command_redo(
            project,
            &entry->command,
            &entry->result)) {
        return 0;
    }

    history->cursor++;

    return 1;
}
