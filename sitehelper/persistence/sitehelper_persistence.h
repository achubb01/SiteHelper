#ifndef SITEHELPER_PERSISTENCE_H
#define SITEHELPER_PERSISTENCE_H

#include "sitehelper_project.h"

typedef enum
{
    SITEHELPER_PERSISTENCE_SUCCESS = 0,
    SITEHELPER_PERSISTENCE_INVALID_ARGUMENT,
    SITEHELPER_PERSISTENCE_IO_ERROR,
    SITEHELPER_PERSISTENCE_UNSUPPORTED_VERSION,
    SITEHELPER_PERSISTENCE_MALFORMED_DATA,
    SITEHELPER_PERSISTENCE_INVALID_PROJECT,
    SITEHELPER_PERSISTENCE_REGENERATION_FAILED,
    SITEHELPER_PERSISTENCE_ALLOCATION_FAILED
} SiteHelperPersistenceResult;

/*
 * Version 7 stores each room's optional semantic plan placement explicitly:
 * "room ID placement unplaced" or "room ID placement placed X Y", followed by
 * "end_room". Physical walls remain global. After rooms, "room_separators N"
 * introduces N "room_separator ID segment X1 Y1 X2 Y2" records in stored order.
 * Saving observes the authoritative project only. It does not generate
 * framing or allocate domain IDs.
 */
SiteHelperPersistenceResult sitehelper_project_save_file(
    const SiteHelperProject *project,
    const char *path
);

/*
 * Loads versions 1-7. Versions 1-2 promote nested wall definitions to global
 * walls; versions 3-4 validate then discard legacy room wall references.
 * IDs, ordered geometry, openings and the allocator watermark are preserved.
 * No topology is inferred from legacy membership.
 * Rooms loaded from versions 1-5 are unplaced; no location is synthesized.
 * Versions 1-6 load with zero room separators; none are inferred from geometry.
 * destination must have been initialized with sitehelper_project_init().
 * On failure, destination remains unchanged.
 */
SiteHelperPersistenceResult sitehelper_project_load_file(
    SiteHelperProject *destination,
    const char *path
);

#endif
