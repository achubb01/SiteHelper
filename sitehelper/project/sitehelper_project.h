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
    SITEHELPER_PROJECT_INVALID_OPENING_COLLECTION,
    SITEHELPER_PROJECT_INVALID_ROOM_ID,
    SITEHELPER_PROJECT_INVALID_WALL_ID,
    SITEHELPER_PROJECT_INVALID_OPENING_ID,
    SITEHELPER_PROJECT_DUPLICATE_ID,
    SITEHELPER_PROJECT_INVALID_ID_GENERATOR,
    SITEHELPER_PROJECT_INVALID_WALL_GEOMETRY,
    SITEHELPER_PROJECT_INVALID_OPENING,
    SITEHELPER_PROJECT_OVERLAPPING_OPENINGS,
    SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_COLLECTION,
    SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_ID,
    SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_GEOMETRY
} SiteHelperProjectValidationCode;

typedef struct
{
    SiteHelperProjectValidationCode code;
    /* Offending object ID; duplicate identity for DUPLICATE_ID; maximum live
     * identity for INVALID_ID_GENERATOR. Zero when unavailable/not applicable. */
    DomainId subject_id;
    /* Containing wall for invalid
     * openings; preceding conflicting opening for OVERLAPPING_OPENINGS.
     * Zero for all other categories. */
    DomainId related_id;
} SiteHelperProjectValidation;

/* Read-only, allocation-free validation of authoritative state only. Ignores
 * framing. Rooms and physical walls are independent domain objects. Unplaced
 * rooms and rooms placed at any representable plan point are valid; enclosure
 * and region association are not project-integrity invariants.
 * Returns the first failure in deterministic order: settings, all collection
 * metadata, identities/allocator, then wall geometry/openings and separators.
 * Within each pass, collections are visited in stored order (rooms before
 * walls, each wall before its openings, then separators). Metadata checks cannot establish the
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

/* Rooms start unplaced. These allocation-free mutations resolve a stable ID,
 * never inspect walls, and leave all state unchanged on failure. Clearing
 * discards the point; (0,0) is a valid placed point, not an absence sentinel. */
int sitehelper_project_set_room_location(SiteHelperProject *project,
    DomainId room_id, PlanPosition location);
int sitehelper_project_clear_room_location(SiteHelperProject *project,
    DomainId room_id);

/* Adds an authoritative global wall; no room or generated framing is required. */
DomainId sitehelper_project_add_wall(
    SiteHelperProject *project,
    WallPlanSegment segment
);

/* Independent virtual input. Failed mutations preserve project state and IDs.
 * Segment replacement is atomic and never inspects physical geometry. */
DomainId sitehelper_project_add_room_separator(SiteHelperProject *project, PlanSegment segment);
int sitehelper_project_remove_room_separator_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_set_room_separator_segment(SiteHelperProject *project,
    DomainId id, PlanSegment segment);

#endif
