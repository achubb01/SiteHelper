#ifndef SITEHELPER_PROJECT_H
#define SITEHELPER_PROJECT_H

#include "build_settings.h"
#include "storey.h"
#include "domain_id.h"

/* Physical coordinates, dimensions, elevations, settings and mutation arguments
 * use integer millimetres (../model/MEASUREMENTS.md). Callers convert external
 * units before these APIs; Project never interprets unit strings or magnitudes. */
typedef struct
{
    /* Stored defaults. Live changes must use the transactional setters below;
     * direct writes are reserved for initialization/loading before generation. */
    BuildSettings settings;
    Storey *storeys;
    size_t storey_count, storey_capacity;
    DomainIdGenerator domain_ids;
} SiteHelperProject;

typedef enum
{
    SITEHELPER_PROJECT_VALID = 0,
    SITEHELPER_PROJECT_INVALID_ARGUMENT,
    SITEHELPER_PROJECT_INVALID_STOREY_COLLECTION,
    SITEHELPER_PROJECT_INVALID_STOREY_ID,
    SITEHELPER_PROJECT_INVALID_SETTINGS,
    SITEHELPER_PROJECT_INVALID_STOREY_SETTINGS,
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
    SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_GEOMETRY,
    SITEHELPER_PROJECT_INVALID_SLAB_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_OUTLINE_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_ID,
    SITEHELPER_PROJECT_INVALID_SLAB_GEOMETRY,
    SITEHELPER_PROJECT_INVALID_SLAB_THICKNESS,
    SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW,
    SITEHELPER_PROJECT_INVALID_SLAB_PENETRATION_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_PENETRATION_OUTLINE_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_PENETRATION_OUTLINE,
    SITEHELPER_PROJECT_SLAB_PENETRATION_OUTSIDE,
    SITEHELPER_PROJECT_SLAB_PENETRATION_OVERLAP
} SiteHelperProjectValidationCode;

typedef struct
{
    SiteHelperProjectValidationCode code;
    /* Offending object ID; duplicate identity for DUPLICATE_ID; maximum live
     * identity for INVALID_ID_GENERATOR. Zero when unavailable/not applicable. */
    DomainId subject_id;
    /* Containing wall for invalid
     * openings; preceding conflicting opening for OVERLAPPING_OPENINGS.
     * For slab errors, owning Storey ID. Zero for other categories. */
    DomainId related_id;
} SiteHelperProjectValidation;

/* Read-only, allocation-free validation of authoritative state only. Ignores
 * framing. Rooms and physical walls are independent domain objects. Unplaced
 * rooms and rooms placed at any representable plan point are valid; enclosure
 * and region association are not project-integrity invariants.
 * Returns the first failure in deterministic order: settings, all collection
 * metadata, Storey settings, identities/allocator, then wall geometry/openings
 * and separators (using each Storey's resolved settings), then slab geometry.
 * Within each pass, Storeys are visited in stored order. Identity order is
 * Storey, Rooms, Walls (each before its openings), separators, then slabs. All nested
 * collection metadata is checked before any global identity traversal.
 * Metadata checks cannot establish the
 * actual allocation size or validity of arbitrary non-null C pointers. */
SiteHelperProjectValidation sitehelper_project_validate(const SiteHelperProject *project);

/* Allocation-free, read-only resolution; writes a complete transient output
 * only on success. Output must be independent of stored project/Storey settings.
 * Existing-object callers resolve the owning Storey by stable ID first. */
int sitehelper_project_resolve_storey_build_settings(const SiteHelperProject *project,
    DomainId storey_id, BuildSettings *output);

/* Live settings mutations, outside command history. Validate current state and
 * stage all affected framing before committing settings and framing together.
 * Failure preserves definitions, IDs, allocator, settings and framing/pointers.
 * Effective configurations that do not change leave their Walls untouched.
 * Success may invalidate framing selections/previews: reconcile editor state. */
int sitehelper_project_set_build_settings(SiteHelperProject *project, const BuildSettings *defaults);
int sitehelper_project_set_stud_height(SiteHelperProject *project, int stud_height);
int sitehelper_project_set_storey_stud_height(SiteHelperProject *project,
    DomainId storey_id, int stud_height);
int sitehelper_project_clear_storey_stud_height(SiteHelperProject *project, DomainId storey_id);

void sitehelper_project_init(
    SiteHelperProject *project
);

void sitehelper_project_destroy(
    SiteHelperProject *project
);

DomainId sitehelper_project_add_room(
    SiteHelperProject *project, DomainId storey_id
);

/* Rooms start unplaced. These allocation-free mutations resolve a stable ID,
 * never inspect walls, and leave all state unchanged on failure. Clearing
 * discards the point; (0,0) is a valid placed point, not an absence sentinel. */
int sitehelper_project_set_room_location(SiteHelperProject *project,
    DomainId room_id, PlanPosition location);
int sitehelper_project_clear_room_location(SiteHelperProject *project,
    DomainId room_id);

/* Adds an authoritative Storey-local wall; no room or generated framing is required. */
DomainId sitehelper_project_add_wall(
    SiteHelperProject *project, DomainId storey_id,
    WallPlanSegment segment
);

/* Independent virtual input. Failed mutations preserve project state and IDs.
 * Segment replacement is atomic and never inspects physical geometry. */
DomainId sitehelper_project_add_room_separator(SiteHelperProject *project, DomainId storey_id, PlanSegment segment);
int sitehelper_project_remove_room_separator_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_set_room_separator_segment(SiteHelperProject *project,
    DomainId id, PlanSegment segment);

/* Init allocates nothing; an empty project is valid. Storeys own their arrays
 * exclusively. All borrowed lookup pointers must be reacquired after mutation
 * or project replacement. IDs survive collection reallocation and persistence.
 * Core creation never chooses a Storey implicitly. Failure preserves state/IDs. */
DomainId sitehelper_project_add_storey(SiteHelperProject *project, int elevation_mm);
Storey *sitehelper_project_find_storey_by_id(SiteHelperProject *project, DomainId id);
const Storey *sitehelper_project_find_storey_by_id_const(const SiteHelperProject *project, DomainId id);
/* Append an empty Storey with an existing ID for loading/restoration. Does not
 * allocate an ID or advance the watermark; caller must establish it. */
int sitehelper_project_insert_storey(SiteHelperProject *project, DomainId id, int elevation_mm);

/* Queries assume coherent authoritative collection metadata. The owner query
 * accepts any nested entity ID, including openings, or the Storey's own ID. */
int sitehelper_project_contains_domain_id(const SiteHelperProject *project, DomainId id);
Storey *sitehelper_project_find_owning_storey(SiteHelperProject *project, DomainId id);
const Storey *sitehelper_project_find_owning_storey_const(const SiteHelperProject *project, DomainId id);
Room *sitehelper_project_find_room_by_id(SiteHelperProject *project, DomainId id);
const Room *sitehelper_project_find_room_by_id_const(const SiteHelperProject *project, DomainId id);
/* Room-only lookup with optional borrowed owner output (cleared on miss).
 * Does not inspect Walls/Openings/separators, so Room queries remain independent
 * of physical geometry. Collection metadata must be coherent. */
const Room *sitehelper_project_find_room_with_owner_const(const SiteHelperProject *project,
    DomainId id, const Storey **owner);
Wall *sitehelper_project_find_wall_by_id(SiteHelperProject *project, DomainId id);
const Wall *sitehelper_project_find_wall_by_id_const(const SiteHelperProject *project, DomainId id);
RoomSeparator *sitehelper_project_find_room_separator_by_id(SiteHelperProject *project, DomainId id);
const RoomSeparator *sitehelper_project_find_room_separator_by_id_const(const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_wall_by_id(SiteHelperProject *project, DomainId id);
/* Existing-identity insertion checks the entire project namespace. */
int sitehelper_project_insert_room_separator(SiteHelperProject *project, DomainId storey_id,
    const RoomSeparator *separator, size_t index);

/* Storey-owned slabs, independent of BuildStructure. Deep-copies the ordered
 * outline; failure preserves project allocations and ID watermark. A successful
 * add/insert may invalidate borrowed slab pointers in that Storey. */
DomainId sitehelper_project_add_slab(SiteHelperProject *project, DomainId storey_id,
    const PlanPosition *vertices, size_t vertex_count, int thickness_mm, int top_level_offset_mm);
Slab *sitehelper_project_find_slab_by_id(SiteHelperProject *project, DomainId id);
const Slab *sitehelper_project_find_slab_by_id_const(const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_slab_by_id(SiteHelperProject *project, DomainId id);
/* Restoration: appends an independent copy with its existing identity; checks
 * the global namespace but does not advance the watermark. Caller establishes it. */
int sitehelper_project_insert_slab(SiteHelperProject *project, DomainId storey_id, const Slab *slab);

#endif
