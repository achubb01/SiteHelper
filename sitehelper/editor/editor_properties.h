#ifndef EDITOR_PROPERTIES_H
#define EDITOR_PROPERTIES_H

#include "sitehelper_editor.h"

/* Physical snapshot values (including resolved settings) are integer
 * millimetres. Display/unit conversion belongs to the GUI/application. */
typedef struct
{
    DomainId wall_id;                 /* Read-only identity. */
    WallPlanSegment segment;          /* Editable ordered plan endpoints, mm. */
    int length_mm;                    /* Read-only, derived from segment. */
    int resolved_stud_height;         /* Read-only, owned by Project/Storey. */
    int resolved_stud_spacing;        /* Read-only, owned by Project. */
} EditorWallProperties;

typedef struct
{
    DomainId wall_id;                 /* Read-only owner identity. */
    Opening definition;              /* Value copy; id is read-only. All other
                                      * fields are editable authoritative data. */
} EditorOpeningProperties;

typedef struct
{
    DomainId slab_id;
    int thickness_mm;
    int top_level_offset_mm;
    size_t vertex_count;              /* Read-only geometry summary. */
} EditorSlabProperties;

typedef struct
{
    DomainId slab_id;
    size_t feature_index;
    size_t vertex_count;              /* Geometry editing is deferred. */
} EditorSlabPenetrationProperties;

typedef struct
{
    DomainId slab_id;
    size_t feature_index;
    int top_level_offset_mm;
    int thickness_mm;
    size_t vertex_count;              /* Geometry editing is deferred. */
} EditorSlabRegionProperties;

typedef struct
{
    DomainId slab_id;
    size_t feature_index;
    SlabEdgeRebate definition;        /* edge_index is read-only in 25E4. */
    int length_mm;                    /* Read-only, derived from U interval. */
} EditorSlabEdgeRebateProperties;

typedef struct
{
    DomainId roof_id;
    size_t portion_count;
    size_t composition_count;
    size_t termination_count;
} EditorRoofProperties;

typedef struct
{
    DomainId roof_id;
    DomainId portion_id;
    size_t support_vertex_count;
    RoofPortionGeneration generation;
    int64_t slope_ppm;
    int reference_z_mm;
    RoofDirection direction;
    RoofSingleSlopeReference single_slope_reference;
} EditorRoofPortionProperties;

typedef struct
{
    EditorSelectionKind kind;
    union
    {
        EditorWallProperties wall;
        EditorOpeningProperties opening;
        EditorSlabProperties slab;
        EditorSlabPenetrationProperties slab_penetration;
        EditorSlabRegionProperties slab_region;
        EditorSlabEdgeRebateProperties slab_edge_rebate;
        EditorRoofProperties roof;
        EditorRoofPortionProperties roof_portion;
    } data;
} EditorProperties;

/* Transient value projection, scoped to the current Storey, independent of
 * current_wall_id navigation. No property values are cached in editor state.
 * NONE, members, stale IDs or invalid context return 0 and zero the output.
 * Output must be caller-owned, not aliased into editor/project state. */
int sitehelper_editor_inspect_properties(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperties *properties);

/* Only editable authoritative fields are represented. No length/height/spacing
 * intent exists for Walls. GUI parsing precedes this typed boundary. */
typedef enum
{
    EDITOR_PROPERTY_WALL_START_X,
    EDITOR_PROPERTY_WALL_START_Y,
    EDITOR_PROPERTY_WALL_END_X,
    EDITOR_PROPERTY_WALL_END_Y,
    EDITOR_PROPERTY_OPENING_TYPE,
    EDITOR_PROPERTY_OPENING_FRAME_POSITION,
    EDITOR_PROPERTY_OPENING_FRAME_BOTTOM,
    EDITOR_PROPERTY_OPENING_WIDTH,
    EDITOR_PROPERTY_OPENING_HEIGHT,
    EDITOR_PROPERTY_OPENING_CUSTOM_ALLOWANCE,
    EDITOR_PROPERTY_OPENING_WIDTH_ALLOWANCE,
    EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE,
    EDITOR_PROPERTY_SLAB_THICKNESS,
    EDITOR_PROPERTY_SLAB_TOP_LEVEL,
    EDITOR_PROPERTY_SLAB_REGION_TOP_LEVEL,
    EDITOR_PROPERTY_SLAB_REGION_THICKNESS,
    EDITOR_PROPERTY_SLAB_REBATE_START,
    EDITOR_PROPERTY_SLAB_REBATE_END,
    EDITOR_PROPERTY_SLAB_REBATE_WIDTH,
    EDITOR_PROPERTY_SLAB_REBATE_DEPTH
} EditorProperty;

typedef struct
{
    EditorProperty property;
    union
    {
        int millimetres;              /* Coordinates, sizes and allowances. */
        OpeningType opening_type;     /* OPENING_TYPE only. */
        bool custom_allowance;        /* OPENING_CUSTOM_ALLOWANCE only. */
    } value;
} EditorPropertyEdit;

/* Return the current integer-mm value for a property that belongs to the active
 * selected object. Useful to seed numeric editing without caching model state. */
int sitehelper_editor_property_millimetres(const SiteHelperEditor *editor,
    const SiteHelperProject *project, EditorProperty property, int *millimetres);

/* Copy the current definition and replace one typed component. Does not mutate
 * or validate geometry: execute through command history, then reconcile editor
 * state after success. Failure zeroes the output command. */
int sitehelper_editor_create_property_command(const SiteHelperEditor *editor,
    const SiteHelperProject *project, const EditorPropertyEdit *edit,
    SiteHelperCommand *command);

#endif
