#ifndef SITEHELPER_COMMAND_H
#define SITEHELPER_COMMAND_H

#include "opening_command.h"
#include "edit_opening_command.h"
#include "wall_command.h"
#include "delete_wall_command.h"
#include "move_wall_endpoint_command.h"
#include "room_location_command.h"
#include "room_separator_command.h"
#include "create_slab_command.h"
#include "delete_slab_command.h"
#include "slab_feature_command.h"
#include "edit_slab_command.h"
#include "move_slab_vertex_command.h"
#include "create_roof_command.h"
#include "delete_roof_command.h"
#include "roof_source_edit_command.h"
#include "plan_note_command.h"
#include "plan_dimension_command.h"
#include "plan_symbol_command.h"
#include "plan_callout_command.h"
#include "plan_revision_cloud_command.h"
#include "document_revision_command.h"
#include "sitehelper_project.h"

typedef enum
{
    SITEHELPER_COMMAND_NONE = 0,

    SITEHELPER_COMMAND_ADD_OPENING,
    SITEHELPER_COMMAND_ADD_WALL,
    SITEHELPER_COMMAND_DELETE_WALL,
    SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
    SITEHELPER_COMMAND_SET_ROOM_LOCATION,
    SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR,
    SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR,
    SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT,
    SITEHELPER_COMMAND_EDIT_OPENING,
    SITEHELPER_COMMAND_CREATE_SLAB,
    SITEHELPER_COMMAND_DELETE_SLAB,
    SITEHELPER_COMMAND_ADD_SLAB_PENETRATION,
    SITEHELPER_COMMAND_DELETE_SLAB_PENETRATION,
    SITEHELPER_COMMAND_ADD_SLAB_REGION,
    SITEHELPER_COMMAND_DELETE_SLAB_REGION,
    SITEHELPER_COMMAND_ADD_SLAB_EDGE_REBATE,
    SITEHELPER_COMMAND_DELETE_SLAB_EDGE_REBATE,
    SITEHELPER_COMMAND_EDIT_SLAB,
    SITEHELPER_COMMAND_EDIT_SLAB_REGION,
    SITEHELPER_COMMAND_EDIT_SLAB_EDGE_REBATE,
    SITEHELPER_COMMAND_MOVE_SLAB_VERTEX,
    SITEHELPER_COMMAND_CREATE_ROOF,
    SITEHELPER_COMMAND_DELETE_ROOF,
    SITEHELPER_COMMAND_EDIT_ROOF_SOURCE,
    SITEHELPER_COMMAND_CREATE_PLAN_NOTE,
    SITEHELPER_COMMAND_EDIT_PLAN_NOTE,
    SITEHELPER_COMMAND_DELETE_PLAN_NOTE,
    SITEHELPER_COMMAND_CREATE_PLAN_DIMENSION,
    SITEHELPER_COMMAND_EDIT_PLAN_DIMENSION,
    SITEHELPER_COMMAND_DELETE_PLAN_DIMENSION,
    SITEHELPER_COMMAND_CREATE_PLAN_SYMBOL,
    SITEHELPER_COMMAND_EDIT_PLAN_SYMBOL,
    SITEHELPER_COMMAND_DELETE_PLAN_SYMBOL,
    SITEHELPER_COMMAND_CREATE_PLAN_CALLOUT,
    SITEHELPER_COMMAND_EDIT_PLAN_CALLOUT,
    SITEHELPER_COMMAND_DELETE_PLAN_CALLOUT,
    SITEHELPER_COMMAND_CREATE_DOCUMENT_REVISION,
    SITEHELPER_COMMAND_EDIT_DOCUMENT_REVISION,
    SITEHELPER_COMMAND_DELETE_DOCUMENT_REVISION,
    SITEHELPER_COMMAND_SET_PLAN_REVISION_CLOUD_REVISION,
    SITEHELPER_COMMAND_CREATE_PLAN_REVISION_CLOUD,
    SITEHELPER_COMMAND_EDIT_PLAN_REVISION_CLOUD,
    SITEHELPER_COMMAND_DELETE_PLAN_REVISION_CLOUD,

    SITEHELPER_COMMAND_COUNT
} SiteHelperCommandType;

typedef struct
{
    SiteHelperCommandType type;

    union
    {
        OpeningCommand opening;
        EditOpeningCommand edit_opening;
        WallCommand wall;
        DeleteWallCommand delete_wall;
        MoveWallEndpointCommand move_wall_endpoint;
        RoomLocationCommand room_location;
        AddRoomSeparatorCommand add_room_separator;
        DeleteRoomSeparatorCommand delete_room_separator;
        MoveRoomSeparatorEndpointCommand move_room_separator_endpoint;
        CreateSlabCommand create_slab;
        DeleteSlabCommand delete_slab;
        AddSlabPenetrationCommand add_slab_penetration;
        DeleteSlabPenetrationCommand delete_slab_penetration;
        AddSlabRegionCommand add_slab_region;
        DeleteSlabRegionCommand delete_slab_region;
        AddSlabEdgeRebateCommand add_slab_edge_rebate;
        DeleteSlabEdgeRebateCommand delete_slab_edge_rebate;
        EditSlabCommand edit_slab;
        EditSlabRegionCommand edit_slab_region;
        EditSlabEdgeRebateCommand edit_slab_edge_rebate;
        MoveSlabVertexCommand move_slab_vertex;
        CreateRoofCommand create_roof;
        DeleteRoofCommand delete_roof;
        RoofSourceEditCommand edit_roof_source;
        CreatePlanNoteCommand create_plan_note;
        EditPlanNoteCommand edit_plan_note;
        DeletePlanNoteCommand delete_plan_note;
        CreatePlanDimensionCommand create_plan_dimension;
        EditPlanDimensionCommand edit_plan_dimension;
        DeletePlanDimensionCommand delete_plan_dimension;
        CreatePlanSymbolCommand create_plan_symbol;
        EditPlanSymbolCommand edit_plan_symbol;
        DeletePlanSymbolCommand delete_plan_symbol;
        CreatePlanCalloutCommand create_plan_callout;
        EditPlanCalloutCommand edit_plan_callout;
        DeletePlanCalloutCommand delete_plan_callout;
        CreateDocumentRevisionCommand create_document_revision;
        EditDocumentRevisionCommand edit_document_revision;
        DeleteDocumentRevisionCommand delete_document_revision;
        SetPlanRevisionCloudRevisionCommand set_plan_revision_cloud_revision;
        CreatePlanRevisionCloudCommand create_plan_revision_cloud;
        EditPlanRevisionCloudCommand edit_plan_revision_cloud;
        DeletePlanRevisionCloudCommand delete_plan_revision_cloud;
    } data;
} SiteHelperCommand;

typedef struct
{
    SiteHelperCommandType type;

    union
    {
        struct
        {

            DomainId wall_id;
            DomainId opening_id;
        } add_opening;

        struct
        {

            DomainId wall_id;
        } add_wall;

        struct
        {
            DomainId wall_id;
        } delete_wall;

        struct
        {
            DomainId wall_id;
        } move_wall_endpoint;

        struct
        {
            DomainId room_id;
        } room_location;

        struct { DomainId separator_id; } room_separator;
        struct { DomainId wall_id, opening_id; } edit_opening;
        struct { DomainId slab_id; } slab;
        struct { DomainId roof_id, portion_id; } roof;
        struct { DomainId annotation_id; } annotation;
        struct { DomainId dimension_id; } dimension;
        struct { DomainId symbol_id; } symbol;
        struct { DomainId callout_id; } callout;
        struct { DomainId revision_id; } revision;
        struct { DomainId revision_cloud_id; } revision_cloud;
        struct { DomainId slab_id; size_t feature_index; } slab_feature;
        struct {
            DomainId slab_id;
            size_t feature_index;
            size_t vertex_index;
        } slab_vertex;

    } data;

} SiteHelperCommandResult;

int sitehelper_command_from_opening(
    const OpeningCommand *opening,
    SiteHelperCommand *command
);
int sitehelper_command_from_edit_opening(const EditOpeningCommand *edit,
    SiteHelperCommand *command);

int sitehelper_command_from_wall(
    const WallCommand *wall,
    SiteHelperCommand *command
);

int sitehelper_command_execute(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
);

int sitehelper_command_from_delete_wall(
    const DeleteWallCommand *deletion,
    SiteHelperCommand *command
);

int sitehelper_command_from_move_wall_endpoint(
    const MoveWallEndpointCommand *move, SiteHelperCommand *command);

int sitehelper_command_from_room_location(
    const RoomLocationCommand *placement, SiteHelperCommand *command);

int sitehelper_command_from_add_room_separator(const AddRoomSeparatorCommand *add, SiteHelperCommand *command);
int sitehelper_command_from_delete_room_separator(const DeleteRoomSeparatorCommand *deletion, SiteHelperCommand *command);
int sitehelper_command_from_move_room_separator_endpoint(const MoveRoomSeparatorEndpointCommand *move, SiteHelperCommand *command);
int sitehelper_command_from_create_slab(const CreateSlabCommand *create, SiteHelperCommand *command);
int sitehelper_command_from_delete_slab(const DeleteSlabCommand *deletion, SiteHelperCommand *command);
int sitehelper_command_from_add_slab_penetration(
    const AddSlabPenetrationCommand *add, SiteHelperCommand *command);
int sitehelper_command_from_delete_slab_penetration(
    const DeleteSlabPenetrationCommand *deletion, SiteHelperCommand *command);
int sitehelper_command_from_add_slab_region(
    const AddSlabRegionCommand *add, SiteHelperCommand *command);
int sitehelper_command_from_delete_slab_region(
    const DeleteSlabRegionCommand *deletion, SiteHelperCommand *command);
int sitehelper_command_from_add_slab_edge_rebate(
    const AddSlabEdgeRebateCommand *add, SiteHelperCommand *command);
int sitehelper_command_from_delete_slab_edge_rebate(
    const DeleteSlabEdgeRebateCommand *deletion, SiteHelperCommand *command);
int sitehelper_command_from_edit_slab(const EditSlabCommand *edit, SiteHelperCommand *command);
int sitehelper_command_from_edit_slab_region(const EditSlabRegionCommand *edit, SiteHelperCommand *command);
int sitehelper_command_from_edit_slab_edge_rebate(const EditSlabEdgeRebateCommand *edit, SiteHelperCommand *command);
int sitehelper_command_from_move_slab_vertex(const MoveSlabVertexCommand *move,
    SiteHelperCommand *command);
int sitehelper_command_from_create_roof(const CreateRoofCommand *create,
    SiteHelperCommand *command);
int sitehelper_command_from_delete_roof(const DeleteRoofCommand *deletion,
    SiteHelperCommand *command);
int sitehelper_command_from_roof_source_edit(const RoofSourceEditCommand *edit,
    SiteHelperCommand *command);
int sitehelper_command_from_create_plan_note(const CreatePlanNoteCommand *create,
    SiteHelperCommand *command);
int sitehelper_command_from_edit_plan_note(const EditPlanNoteCommand *edit,
    SiteHelperCommand *command);
int sitehelper_command_from_delete_plan_note(const DeletePlanNoteCommand *deletion,
    SiteHelperCommand *command);
int sitehelper_command_from_create_plan_dimension(const CreatePlanDimensionCommand *create,
    SiteHelperCommand *command);
int sitehelper_command_from_edit_plan_dimension(const EditPlanDimensionCommand *edit,
    SiteHelperCommand *command);
int sitehelper_command_from_delete_plan_dimension(const DeletePlanDimensionCommand *deletion,
    SiteHelperCommand *command);
int sitehelper_command_from_create_plan_symbol(const CreatePlanSymbolCommand *create,
    SiteHelperCommand *command);
int sitehelper_command_from_edit_plan_symbol(const EditPlanSymbolCommand *edit,
    SiteHelperCommand *command);
int sitehelper_command_from_delete_plan_symbol(const DeletePlanSymbolCommand *deletion,
    SiteHelperCommand *command);
int sitehelper_command_from_create_plan_callout(const CreatePlanCalloutCommand *create,
    SiteHelperCommand *command);
int sitehelper_command_from_edit_plan_callout(const EditPlanCalloutCommand *edit,
    SiteHelperCommand *command);
int sitehelper_command_from_delete_plan_callout(const DeletePlanCalloutCommand *deletion,
    SiteHelperCommand *command);
int sitehelper_command_from_create_document_revision(
    const CreateDocumentRevisionCommand *create, SiteHelperCommand *command);
int sitehelper_command_from_edit_document_revision(
    const EditDocumentRevisionCommand *edit, SiteHelperCommand *command);
int sitehelper_command_from_delete_document_revision(
    const DeleteDocumentRevisionCommand *deletion, SiteHelperCommand *command);
int sitehelper_command_from_set_plan_revision_cloud_revision(
    const SetPlanRevisionCloudRevisionCommand *set, SiteHelperCommand *command);
int sitehelper_command_from_create_plan_revision_cloud(
    const CreatePlanRevisionCloudCommand *create, SiteHelperCommand *command);
int sitehelper_command_from_edit_plan_revision_cloud(
    const EditPlanRevisionCloudCommand *edit, SiteHelperCommand *command);
int sitehelper_command_from_delete_plan_revision_cloud(
    const DeletePlanRevisionCloudCommand *deletion, SiteHelperCommand *command);

/* Commands are owned values. CREATE_SLAB, CREATE_ROOF, EDIT_ROOF_SOURCE where applicable,
 * and polygon feature ADD commands own outlines/source geometry. Do not shallow-copy them;
 * clone explicitly and destroy each owner
 * exactly once. History clones, so caller lifetime remains independent. */
int sitehelper_command_clone(const SiteHelperCommand *source, SiteHelperCommand *output);
void sitehelper_command_destroy(SiteHelperCommand *command);

/* Compact add-command undo. Deletion and mutation commands require state
 * owned by command history; use history execute/undo for reversible edits. */
int sitehelper_command_undo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
);

int sitehelper_command_redo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
);

#endif
