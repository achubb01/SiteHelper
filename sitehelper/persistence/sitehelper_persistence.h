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
    SITEHELPER_PERSISTENCE_ALLOCATION_FAILED,
    /* Legacy opening cannot regenerate with faithfully preserved framing and
     * a positive canonical clear rectangle. Destination remains unchanged. */
    SITEHELPER_PERSISTENCE_OPENING_MIGRATION_FAILED
} SiteHelperPersistenceResult;

/*
 * Persisted authoritative physical measurements are integer millimetres,
 * without suffixes/unit markers (../model/MEASUREMENTS.md). IDs, counts, flags
 * and modes are not measurements. Supported versions 1-14 all use millimetres;
 * legacy geometry-reference migrations below do not change dimensional units.
 * Version 14 adds slab-owned edge-rebate records after regions. Each stores an
 * outer edge index, integer-mm U interval, inward width and local-top-relative
 * depth. Versions 1-13 have zero edge rebates. Derived geometry is not saved.
 * Version 13 adds slab-owned replacement regions after penetrations: each stores
 * Storey-relative top offset, positive thickness and ordered outline vertices.
 * Versions 1-12 have zero regions. Bottom levels, areas and volumes are not saved.
 * Version 12 adds slab-owned penetration outlines, without IDs, between the
 * exterior vertices and end_slab. Versions 1-11 have zero penetrations.
 * Version 11 adds Storey-owned slabs (ID, relative top level, thickness and
 * ordered integer-mm vertices), after room_separators and before end_storey.
 * Version 10 stores canonical clear-opening geometry. Otherwise it retains
 * version 9's ordered Storeys, each as "storey ID elevation MM", followed
 * by "stud_height inherit" or "stud_height override N", then
 * by its walls/openings, rooms/placements and room_separators, then end_storey.
 * Settings and the DomainId watermark remain project-wide. No derived state
 * is saved. Save validates all authoritative state before opening the file.
 */
SiteHelperPersistenceResult sitehelper_project_save_file(
    const SiteHelperProject *project,
    const char *path
);

/*
 * Loads versions 1-14 transactionally into an initialized destination. Parse,
 * authoritative validation and framing regeneration must all succeed before
 * replacing it. On failure destination remains unchanged.
 * Versions 1-10 load with zero slabs. Slab quantities are never serialized.
 * Versions 1-7 migrate into one elevation-zero Storey. Existing entity IDs
 * and ordered geometry are preserved. The old watermark supplies the fresh
 * Storey ID and advances once; collision/exhaustion fails without wrapping.
 * Versions 1-2 promote nested walls into Storey-global storage; 3-4 validate
 * and discard Room wall references. Versions 1-5 leave Rooms unplaced, and
 * versions 1-6 have no virtual separators. No spatial relationship is inferred.
 * Versions 1-8 have no Storey overrides; every Storey inherits Project defaults.
 * Versions 1-9 openings migrate with the owning Storey's effective settings:
 * window bottom becomes legacy sill top; clear top remains header underside.
 * Door clear top becomes the legacy trimmer top, retaining the lower reference
 * used by legacy noggin exclusion. No legacy semantics survive in model state.
 * With legacy effective height H, bottom B and member width W: windows become
 * (bottom B+W, effective height H-W); doors become (bottom B, height H-B).
 * A non-positive resulting height fails explicitly, rather than moving members.
 */
SiteHelperPersistenceResult sitehelper_project_load_file(
    SiteHelperProject *destination,
    const char *path
);

#endif
