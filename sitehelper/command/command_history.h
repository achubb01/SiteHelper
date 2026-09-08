#ifndef COMMAND_HISTORY_H
#define COMMAND_HISTORY_H

#include <stddef.h>

#include "sitehelper_command.h"

typedef struct SiteHelperCommandUndoState SiteHelperCommandUndoState;

/* Entries own undo_state exclusively. They may be moved, never shallow-copied
 * into another owning entry. Commands and public results remain plain values. */
typedef struct
{
    SiteHelperCommand command;
    SiteHelperCommandResult result;
    SiteHelperCommandUndoState *undo_state;

} SiteHelperCommandHistoryEntry;


typedef struct
{
    SiteHelperCommandHistoryEntry *entries;

    size_t count;
    size_t cursor;
    size_t capacity;

} SiteHelperCommandHistory;


void sitehelper_command_history_init(
    SiteHelperCommandHistory *history
);

void sitehelper_command_history_destroy(
    SiteHelperCommandHistory *history
);

int sitehelper_command_history_execute(
    SiteHelperCommandHistory *history,
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
);

int sitehelper_command_history_undo(
    SiteHelperCommandHistory *history,
    SiteHelperProject *project
);

int sitehelper_command_history_redo(
    SiteHelperCommandHistory *history,
    SiteHelperProject *project
);


#endif
