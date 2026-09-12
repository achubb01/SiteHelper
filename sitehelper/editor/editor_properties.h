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
    EditorSelectionKind kind;
    union
    {
        EditorWallProperties wall;
        EditorOpeningProperties opening;
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
    EDITOR_PROPERTY_OPENING_HEIGHT_ALLOWANCE
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

/* Copy the current definition and replace one typed component. Does not mutate
 * or validate geometry: execute through command history, then reconcile editor
 * state after success. Failure zeroes the output command. */
int sitehelper_editor_create_property_command(const SiteHelperEditor *editor,
    const SiteHelperProject *project, const EditorPropertyEdit *edit,
    SiteHelperCommand *command);

#endif
