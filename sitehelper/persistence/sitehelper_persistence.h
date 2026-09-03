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
 * Saving observes the authoritative project only. It does not generate
 * framing or allocate domain IDs.
 */
SiteHelperPersistenceResult sitehelper_project_save_file(
    const SiteHelperProject *project,
    const char *path
);

/*
 * destination must have been initialized with sitehelper_project_init().
 * On failure, destination remains unchanged.
 */
SiteHelperPersistenceResult sitehelper_project_load_file(
    SiteHelperProject *destination,
    const char *path
);

#endif
