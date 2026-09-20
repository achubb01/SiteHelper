#ifndef SITEHELPER_PROJECT_H
#define SITEHELPER_PROJECT_H

#include "build_settings.h"
#include "storey.h"
#include "domain_id.h"
#include "document.h"

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
    /* Authored non-physical project information; not owned by any Storey. */
    DocumentModel document;
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
    SITEHELPER_PROJECT_SLAB_PENETRATION_OVERLAP,
    SITEHELPER_PROJECT_INVALID_SLAB_REGION_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE,
    SITEHELPER_PROJECT_INVALID_SLAB_REGION_THICKNESS,
    SITEHELPER_PROJECT_SLAB_REGION_OUTSIDE,
    SITEHELPER_PROJECT_SLAB_REGION_OVERLAP,
    SITEHELPER_PROJECT_SLAB_REGION_PENETRATION_INTERSECTION,
    SITEHELPER_PROJECT_INVALID_SLAB_EDGE_REBATE_COLLECTION,
    SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_EDGE,
    SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_INTERVAL,
    SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_DIMENSIONS,
    SITEHELPER_PROJECT_SLAB_EDGE_REBATE_OVERLAP,
    SITEHELPER_PROJECT_INVALID_ROOF_COLLECTION,
    SITEHELPER_PROJECT_INVALID_ROOF_ID,
    SITEHELPER_PROJECT_INVALID_ROOF_PORTION_ID,
    SITEHELPER_PROJECT_INVALID_ROOF,
    SITEHELPER_PROJECT_INVALID_ROOF_GEOMETRY,
    SITEHELPER_PROJECT_INVALID_ROOF_COMPOSITION,
    SITEHELPER_PROJECT_INVALID_ROOF_TERMINATION,
    SITEHELPER_PROJECT_INVALID_ANNOTATION_COLLECTION,
    SITEHELPER_PROJECT_INVALID_ANNOTATION_ID,
    SITEHELPER_PROJECT_INVALID_ANNOTATION,
    SITEHELPER_PROJECT_INVALID_ANNOTATION_REFERENCE,
    SITEHELPER_PROJECT_INVALID_DIMENSION_COLLECTION,
    SITEHELPER_PROJECT_INVALID_DIMENSION_ID,
    SITEHELPER_PROJECT_INVALID_DIMENSION,
    SITEHELPER_PROJECT_INVALID_DIMENSION_REFERENCE,
    SITEHELPER_PROJECT_INVALID_SYMBOL_COLLECTION,
    SITEHELPER_PROJECT_INVALID_SYMBOL_ID,
    SITEHELPER_PROJECT_INVALID_SYMBOL,
    SITEHELPER_PROJECT_INVALID_CALLOUT_COLLECTION,
    SITEHELPER_PROJECT_INVALID_CALLOUT_ID,
    SITEHELPER_PROJECT_INVALID_CALLOUT,
    SITEHELPER_PROJECT_INVALID_REVISION_COLLECTION,
    SITEHELPER_PROJECT_INVALID_REVISION_ID,
    SITEHELPER_PROJECT_INVALID_REVISION,
    SITEHELPER_PROJECT_INVALID_REVISION_CLOUD_COLLECTION,
    SITEHELPER_PROJECT_INVALID_REVISION_CLOUD_ID,
    SITEHELPER_PROJECT_INVALID_REVISION_CLOUD
} SiteHelperProjectValidationCode;

typedef struct
{
    SiteHelperProjectValidationCode code;
    /* Offending object ID; duplicate identity for DUPLICATE_ID; maximum live
     * identity for INVALID_ID_GENERATOR. Zero when unavailable/not applicable. */
    DomainId subject_id;
    /* Containing wall for invalid
     * openings; preceding conflicting opening for OVERLAPPING_OPENINGS.
     * For slab/roof errors, owning Storey ID unless the roof relationship uses
     * its Roof ID as the more useful parent. Annotation/dimension/callout errors use their Storey scope or invalid target ID where useful. Zero for other categories. */
    DomainId related_id;
} SiteHelperProjectValidation;

/* Read-only, allocation-free validation of authoritative state only. Ignores
 * framing. Rooms and physical walls are independent domain objects. Unplaced
 * rooms and rooms placed at any representable plan point are valid; enclosure
 * and region association are not project-integrity invariants.
 * Returns the first failure in deterministic order: settings, all collection
 * metadata, Storey settings, identities/allocator, then wall geometry/openings
 * and separators (using each Storey's resolved settings), then roof source
 * authority and slab geometry. Within each pass, Storeys are visited in stored
 * order. Identity order is Storey, Rooms, Walls (each before its openings),
 * separators, Slabs, Roofs (each before its portions), annotations, dimensions, symbols, callouts, revisions, then revision clouds. All nested
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

/* Project-scope queries are deliberately limited to global identity/ownership.
 * Feature/spatial/report queries remain in their owning subsystems; see
 * QUERY_LAYER.md. Do not turn these typed lookups into a generic repository.
 * Queries assume coherent authoritative collection metadata. The owner query
 * accepts physical nested entity IDs, including openings, or the Storey's own
 * ID. Project-owned annotations have global IDs but deliberately no owning
 * Storey; their explicit plan anchor supplies presentation scope. */
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

/* Priority 26G1 authoritative roof ownership. Creation is transactional: a Roof
 * enters a Storey only together with one valid source portion, and all globally
 * identifiable Roof/portion objects use the Project DomainId allocator.
 * Compositions and terminations are roof-owned value relationships keyed by
 * stable portion IDs; generated planes/edges/layout snapshots have no IDs. */
DomainId sitehelper_project_add_roof(SiteHelperProject *project, DomainId storey_id,
    const RoofPortionSpec *initial_portion, DomainId *portion_id);
/* Adds a new portion and its explicit relationship as one transaction so a
 * successful mutation never leaves an uncomposed multi-portion Roof. */
DomainId sitehelper_project_add_roof_portion_composed(SiteHelperProject *project, DomainId roof_id,
    const RoofPortionSpec *spec, DomainId existing_portion_id, RoofCompositionKind kind);
/* History redo variant: inserts a new portion using an existing identity below
 * the allocator watermark. The ID must be globally free. */
int sitehelper_project_restore_roof_portion_composed(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id, const RoofPortionSpec *spec, DomainId existing_portion_id,
    RoofCompositionKind kind);
int sitehelper_project_set_roof_portion(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id, const RoofPortionSpec *spec);
int sitehelper_project_remove_roof_portion(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id);
int sitehelper_project_set_roof_composition(SiteHelperProject *project, DomainId roof_id,
    RoofComposition composition);
int sitehelper_project_remove_roof_composition(SiteHelperProject *project, DomainId roof_id,
    DomainId first_portion_id, DomainId second_portion_id);
int sitehelper_project_set_roof_termination(SiteHelperProject *project, DomainId roof_id,
    RoofTermination termination);
int sitehelper_project_remove_roof_termination(SiteHelperProject *project, DomainId roof_id,
    DomainId portion_id, RoofEnd end);
/* History restoration primitive for source edits. Replacement must retain the
 * same Roof identity. New-to-current portion IDs must be globally free and
 * below the existing allocator watermark. */
int sitehelper_project_replace_roof(SiteHelperProject *project, const Roof *replacement);
/* Legacy additive aliases retained for current callers. */
int sitehelper_project_add_roof_composition(SiteHelperProject *project, DomainId roof_id,
    RoofComposition composition);
int sitehelper_project_add_roof_termination(SiteHelperProject *project, DomainId roof_id,
    RoofTermination termination);
Roof *sitehelper_project_find_roof_by_id(SiteHelperProject *project, DomainId id);
const Roof *sitehelper_project_find_roof_by_id_const(const SiteHelperProject *project, DomainId id);
RoofPortionDefinition *sitehelper_project_find_roof_portion_by_id(SiteHelperProject *project, DomainId id);
const RoofPortionDefinition *sitehelper_project_find_roof_portion_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_roof_by_id(SiteHelperProject *project, DomainId id);
/* Existing-identity restoration/redo. Deep-copies the Roof and preserves the
 * requested Storey collection order. Does not advance the global ID watermark;
 * callers restoring history must ensure all identities are below it. */
int sitehelper_project_insert_roof_at(SiteHelperProject *project, DomainId storey_id,
    const Roof *roof, size_t index);


/* Project-owned document authority. Plan notes are the first supported
 * annotation payload. They are non-physical, use global stable IDs, and may
 * optionally associate with a physical object on the same Storey. Live add
 * requires that target to exist; the association becomes weak if the target is
 * later deleted. ID allocation is staged and text is deep-copied; failure
 * preserves project state. */
DomainId sitehelper_project_add_plan_note(SiteHelperProject *project, DomainId storey_id,
    PlanPosition position, DomainId target_id, const char *text);
DocumentAnnotation *sitehelper_project_find_annotation_by_id(
    SiteHelperProject *project, DomainId id);
const DocumentAnnotation *sitehelper_project_find_annotation_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_annotation_by_id(SiteHelperProject *project, DomainId id);
/* Transactional note edit. Identity remains stable and text is deep-copied before
 * the old payload is released. An unchanged unresolved weak target may remain; a
 * newly selected target must currently resolve on the destination Storey. */
int sitehelper_project_update_plan_note(SiteHelperProject *project, DomainId id,
    DomainId storey_id, PlanPosition position, DomainId target_id, const char *text);
/* Loading/restoration primitive. Deep-copies text, preserves the existing ID and
 * does not advance the allocator watermark. */
int sitehelper_project_insert_annotation(SiteHelperProject *project,
    const DocumentAnnotation *annotation);
/* History restoration primitive for an existing annotation identity. Deep-copy
 * replacement; unresolved weak physical targets are allowed. */
int sitehelper_project_replace_annotation(SiteHelperProject *project,
    const DocumentAnnotation *annotation);

/* Persistent plan dimensions. Measurements are derived from two references; the
 * numeric value is never stored as authority. The first associative reference
 * contract deliberately supports only semantically stable wall start/end points
 * plus free fixed points. Missing wall targets are valid weak references after
 * loading/history, but live creation requires resolvable same-Storey targets. */
DomainId sitehelper_project_add_plan_dimension(SiteHelperProject *project, DomainId storey_id,
    DocumentDimensionReference first, DocumentDimensionReference second, int offset_mm);
DocumentPlanDimension *sitehelper_project_find_dimension_by_id(SiteHelperProject *project, DomainId id);
const DocumentPlanDimension *sitehelper_project_find_dimension_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_dimension_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_insert_dimension(SiteHelperProject *project,
    const DocumentPlanDimension *dimension);
int sitehelper_project_update_plan_dimension(SiteHelperProject *project, DomainId id,
    DomainId storey_id, DocumentDimensionReference first,
    DocumentDimensionReference second, int offset_mm);
int sitehelper_project_replace_dimension(SiteHelperProject *project,
    const DocumentPlanDimension *dimension);
/* Returns 0 if either weak association is unresolved or if the resolved points
 * coincide. On success writes both authoritative plan points and the canonical
 * nearest-whole-millimetre distance. */
int sitehelper_project_resolve_plan_dimension(const SiteHelperProject *project, DomainId id,
    PlanPosition *first, PlanPosition *second, int *distance_mm);

/* Persistent Plan symbols. Symbols are project-owned document authority scoped
 * to a Storey for presentation; they are not physical Storey children. Concrete
 * kinds currently include fixed point markers and oriented view-direction marks. */
DomainId sitehelper_project_add_plan_symbol(SiteHelperProject *project, DomainId storey_id,
    DocumentPlanSymbolKind kind, PlanPosition anchor, DocumentPlanDirection direction);
DocumentPlanSymbol *sitehelper_project_find_symbol_by_id(SiteHelperProject *project, DomainId id);
const DocumentPlanSymbol *sitehelper_project_find_symbol_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_symbol_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_insert_symbol(SiteHelperProject *project, const DocumentPlanSymbol *symbol);
int sitehelper_project_update_plan_symbol(SiteHelperProject *project, DomainId id,
    DomainId storey_id, DocumentPlanSymbolKind kind, PlanPosition anchor,
    DocumentPlanDirection direction);
int sitehelper_project_replace_symbol(SiteHelperProject *project, const DocumentPlanSymbol *symbol);

/* Persistent Plan leader/callouts. Target and label anchor are fixed integer-mm
 * authored points; text is deep-owned by the document layer. */
DomainId sitehelper_project_add_plan_callout(SiteHelperProject *project, DomainId storey_id,
    PlanPosition target, PlanPosition label_anchor, const char *text);
DocumentPlanCallout *sitehelper_project_find_callout_by_id(SiteHelperProject *project, DomainId id);
const DocumentPlanCallout *sitehelper_project_find_callout_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_callout_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_insert_callout(SiteHelperProject *project, const DocumentPlanCallout *callout);
int sitehelper_project_update_plan_callout(SiteHelperProject *project, DomainId id,
    DomainId storey_id, PlanPosition target, PlanPosition label_anchor, const char *text);
int sitehelper_project_replace_callout(SiteHelperProject *project,
    const DocumentPlanCallout *callout);


/* Project-level revision records group review markup. They intentionally carry
 * only identifier + optional description until sheets/issue workflow/user identity
 * establish stronger lifecycle semantics. */
DomainId sitehelper_project_add_revision(SiteHelperProject *project,
    const char *identifier, const char *description);
DocumentRevision *sitehelper_project_find_revision_by_id(
    SiteHelperProject *project, DomainId id);
const DocumentRevision *sitehelper_project_find_revision_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_revision_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_insert_revision(SiteHelperProject *project,
    const DocumentRevision *revision);
int sitehelper_project_update_revision(SiteHelperProject *project, DomainId id,
    const char *identifier, const char *description);
int sitehelper_project_replace_revision(SiteHelperProject *project,
    const DocumentRevision *revision);

/* Project-owned revision markup. The authored closed boundary is deep-owned and
 * Storey-scoped; no paper-space scallop/style authority is persisted yet. */
DomainId sitehelper_project_add_plan_revision_cloud(SiteHelperProject *project,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count);
DocumentPlanRevisionCloud *sitehelper_project_find_revision_cloud_by_id(
    SiteHelperProject *project, DomainId id);
const DocumentPlanRevisionCloud *sitehelper_project_find_revision_cloud_by_id_const(
    const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_revision_cloud_by_id(SiteHelperProject *project, DomainId id);
int sitehelper_project_insert_revision_cloud(SiteHelperProject *project,
    const DocumentPlanRevisionCloud *cloud);
int sitehelper_project_update_plan_revision_cloud(SiteHelperProject *project, DomainId id,
    DomainId storey_id, const PlanPosition *vertices, size_t vertex_count);
int sitehelper_project_replace_revision_cloud(SiteHelperProject *project,
    const DocumentPlanRevisionCloud *cloud);
/* Explicit assignment requires a live revision; DOMAIN_ID_INVALID clears it.
 * The stored link remains weak so deleting a revision record leaves cloud
 * authority intact and undo can restore the same revision ID. */
int sitehelper_project_set_revision_cloud_revision(SiteHelperProject *project,
    DomainId revision_cloud_id, DomainId revision_id);

DomainId sitehelper_project_add_slab(SiteHelperProject *project, DomainId storey_id,
    const PlanPosition *vertices, size_t vertex_count, int thickness_mm, int top_level_offset_mm);
Slab *sitehelper_project_find_slab_by_id(SiteHelperProject *project, DomainId id);
const Slab *sitehelper_project_find_slab_by_id_const(const SiteHelperProject *project, DomainId id);
int sitehelper_project_remove_slab_by_id(SiteHelperProject *project, DomainId id);
/* Restoration: appends an independent copy with its existing identity; checks
 * the global namespace but does not advance the watermark. Caller establishes it. */
int sitehelper_project_insert_slab(SiteHelperProject *project, DomainId storey_id, const Slab *slab);
/* Indexed restoration preserves Storey collection order. Like insert_slab it
 * does not advance/check the allocator watermark because transactional loading
 * establishes that watermark after inserting all persisted identities. */
int sitehelper_project_insert_slab_at(SiteHelperProject *project, DomainId storey_id,
    const Slab *slab, size_t index);

#endif
