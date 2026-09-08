#ifndef SITEHELPER_PROJECT_H
#define SITEHELPER_PROJECT_H

#include "build_settings.h"
#include "build_structure.h"
#include "domain_id.h"

typedef struct
{
    BuildSettings settings;
    BuildStructure structure;
    DomainIdGenerator domain_ids;
} SiteHelperProject;

typedef enum
{
    SITEHELPER_PROJECT_VALID = 0,
    SITEHELPER_PROJECT_INVALID_ARGUMENT,
    SITEHELPER_PROJECT_INVALID_SETTINGS,
    SITEHELPER_PROJECT_INVALID_ROOM_COLLECTION,
    SITEHELPER_PROJECT_INVALID_WALL_COLLECTION,
    SITEHELPER_PROJECT_INVALID_ROOM_REFERENCE_COLLECTION,
    SITEHELPER_PROJECT_INVALID_OPENING_COLLECTION,
    SITEHELPER_PROJECT_INVALID_ROOM_ID,
    SITEHELPER_PROJECT_INVALID_WALL_ID,
    SITEHELPER_PROJECT_INVALID_OPENING_ID,
    SITEHELPER_PROJECT_DUPLICATE_ID,
    SITEHELPER_PROJECT_INVALID_ID_GENERATOR,
    SITEHELPER_PROJECT_UNRESOLVED_WALL_REFERENCE,
    SITEHELPER_PROJECT_DUPLICATE_WALL_REFERENCE,
    SITEHELPER_PROJECT_INVALID_WALL_GEOMETRY,
    SITEHELPER_PROJECT_INVALID_OPENING,
    SITEHELPER_PROJECT_OVERLAPPING_OPENINGS
} SiteHelperProjectValidationCode;

typedef struct
{
    SiteHelperProjectValidationCode code;
    /* Offending object ID; duplicate identity for DUPLICATE_ID; maximum live
     * identity for INVALID_ID_GENERATOR. Zero when unavailable/not applicable. */
    DomainId subject_id;
    /* Referenced wall for reference failures; containing wall for invalid
     * openings; preceding conflicting opening for OVERLAPPING_OPENINGS.
     * Zero for all other categories. */
    DomainId related_id;
} SiteHelperProjectValidation;

/* Read-only, allocation-free validation of authoritative state only. Ignores
 * framing. Shared and unreferenced physical walls are valid.
 * Returns the first failure in deterministic order: settings, all collection
 * metadata, identities/allocator, room references, then wall geometry/openings.
 * Within each pass, collections are visited in stored order (rooms before
 * walls, each wall before its openings). Metadata checks cannot establish the
 * actual allocation size or validity of arbitrary non-null C pointers. */
SiteHelperProjectValidation sitehelper_project_validate(const SiteHelperProject *project);

void sitehelper_project_init(
    SiteHelperProject *project
);

void sitehelper_project_destroy(
    SiteHelperProject *project
);

DomainId sitehelper_project_add_room(
    SiteHelperProject *project
);

DomainId sitehelper_project_add_wall(
    SiteHelperProject *project,
    DomainId room_id,
    WallPlanSegment segment
);

#endif
