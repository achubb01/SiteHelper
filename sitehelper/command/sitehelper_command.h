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

/* Commands are owned values. CREATE_SLAB and polygon feature ADD commands own
 * outlines. Do not shallow-copy them; clone explicitly and destroy each owner
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
