#ifndef SITEHELPER_COMMAND_INTERNAL_H
#define SITEHELPER_COMMAND_INTERNAL_H

#include "sitehelper_command.h"

typedef struct SiteHelperCommandUndoState SiteHelperCommandUndoState;

/* Capture before mutation. Success may return NULL for compact add commands. */
int sitehelper_command_capture_undo_state(
    const SiteHelperProject *project, const SiteHelperCommand *command,
    SiteHelperCommandUndoState **state);
void sitehelper_command_destroy_undo_state(SiteHelperCommandUndoState *state);
int sitehelper_command_undo_with_state(
    SiteHelperProject *project, const SiteHelperCommand *command,
    const SiteHelperCommandResult *result, const SiteHelperCommandUndoState *state);

#endif
