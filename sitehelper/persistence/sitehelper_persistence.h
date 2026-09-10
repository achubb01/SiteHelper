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
 * Version 8 stores ordered Storeys, each as "storey ID elevation MM", followed
 * by its walls/openings, rooms/placements and room_separators, then end_storey.
 * Settings and the DomainId watermark remain project-wide. No derived state
 * is saved. Save validates all authoritative state before opening the file.
 */
SiteHelperPersistenceResult sitehelper_project_save_file(
    const SiteHelperProject *project,
    const char *path
);

/*
 * Loads versions 1-8 transactionally into an initialized destination. Parse,
 * authoritative validation and framing regeneration must all succeed before
 * replacing it. On failure destination remains unchanged.
 * Versions 1-7 migrate into one elevation-zero Storey. Existing entity IDs
 * and ordered geometry are preserved. The old watermark supplies the fresh
 * Storey ID and advances once; collision/exhaustion fails without wrapping.
 * Versions 1-2 promote nested walls into Storey-global storage; 3-4 validate
 * and discard Room wall references. Versions 1-5 leave Rooms unplaced, and
 * versions 1-6 have no virtual separators. No spatial relationship is inferred.
 */
SiteHelperPersistenceResult sitehelper_project_load_file(
    SiteHelperProject *destination,
    const char *path
);

#endif
