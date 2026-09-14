#include <stdlib.h>

#include "sitehelper_command.h"
#include "sitehelper_command_internal.h"
#include "delete_wall_command_internal.h"
#include "wall.h"
#include "slab.h"

typedef struct {
    DomainId storey_id;
    size_t index;
    Slab slab;
} DeletedSlabSnapshot;

struct SiteHelperCommandUndoState
{
    SiteHelperCommandType type;
    union
    {
        DeletedWallSnapshot deleted_wall;
        struct
        {
            DomainId wall_id;
            WallPlanSegment segment;
        } moved_wall;
        RoomLocationCommand previous_room_location;
        EditOpeningCommand previous_opening;
        struct {
            DomainId storey_id;
            RoomSeparator definition;
            size_t index; /* Delete undo restores stored collection order. */
        } room_separator;
        DeletedSlabSnapshot deleted_slab;
    };
};

int sitehelper_command_capture_undo_state(
    const SiteHelperProject *project, const SiteHelperCommand *command,
    SiteHelperCommandUndoState **state)
{
    if (project == NULL || command == NULL || state == NULL) {
        return 0;
    }
    *state = NULL;
    if (command->type != SITEHELPER_COMMAND_DELETE_WALL &&
        command->type != SITEHELPER_COMMAND_EDIT_OPENING &&
        command->type != SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT &&
        command->type != SITEHELPER_COMMAND_SET_ROOM_LOCATION &&
        command->type != SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR &&
        command->type != SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT &&
        command->type != SITEHELPER_COMMAND_DELETE_SLAB) {
        return 1;
    }
    SiteHelperCommandUndoState *candidate = calloc(1, sizeof *candidate);
    if (candidate == NULL) {
        return 0;
    }
    candidate->type = command->type;
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB) {
        const Slab *slab=sitehelper_project_find_slab_by_id_const(project,
            command->data.delete_slab.slab_id);
        const Storey *owner=slab == NULL ? NULL :
            sitehelper_project_find_owning_storey_const(project,slab->id);
        if (owner == NULL ||
            slab_collection_find_by_id_const(&owner->slabs,slab->id) != slab ||
            slab_clone(slab,&candidate->deleted_slab.slab) != SLAB_SUCCESS) {
            sitehelper_command_destroy_undo_state(candidate); return 0;
        }
        candidate->deleted_slab.storey_id=owner->id;
        candidate->deleted_slab.index=(size_t)(slab-owner->slabs.items);
    }
    else if (command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR ||
        command->type == SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT) {
        DomainId id = command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR
            ? command->data.delete_room_separator.separator_id
            : command->data.move_room_separator_endpoint.separator_id;
        const RoomSeparator *separator = sitehelper_project_find_room_separator_by_id_const(project, id);
        if (separator == NULL) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
        const Storey *storey = sitehelper_project_find_owning_storey_const(project, id);
        candidate->room_separator.storey_id = storey->id;
        candidate->room_separator.definition = *separator;
        candidate->room_separator.index = (size_t)(separator - storey->structure.room_separators);
    }
    else if (command->type == SITEHELPER_COMMAND_EDIT_OPENING) {
        const EditOpeningCommand *edit = &command->data.edit_opening;
        const Wall *wall = sitehelper_project_find_wall_by_id_const(project, edit->wall_id);
        const Opening *opening = wall_find_opening_by_id_const(wall, edit->opening_id);
        if (!edit_opening_command_create(edit->wall_id, edit->opening_id,
                opening, &candidate->previous_opening)) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
    }
    else if (command->type == SITEHELPER_COMMAND_SET_ROOM_LOCATION) {
        const Room *room = sitehelper_project_find_room_by_id_const(project,
            command->data.room_location.room_id);
        if (room == NULL) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
        candidate->previous_room_location = (RoomLocationCommand){
            .room_id = room->id, .has_location = room->has_location,
            .location = room->has_location ? room->location : (PlanPosition){0}
        };
    }
    else if (command->type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        const Wall *wall = sitehelper_project_find_wall_by_id_const(project,
            command->data.move_wall_endpoint.wall_id);
        if (wall == NULL) {
            sitehelper_command_destroy_undo_state(candidate);
            return 0;
        }
        candidate->moved_wall.wall_id = wall->id;
        candidate->moved_wall.segment = wall->definition.segment;
    }
    else if (!deleted_wall_snapshot_capture(project, &command->data.delete_wall,
            &candidate->deleted_wall)) {
        sitehelper_command_destroy_undo_state(candidate);
        return 0;
    }
    *state = candidate;
    return 1;
}

void sitehelper_command_destroy_undo_state(SiteHelperCommandUndoState *state)
{
    if (state == NULL) {
        return;
    }
    if (state->type == SITEHELPER_COMMAND_DELETE_WALL) {
        deleted_wall_snapshot_destroy(&state->deleted_wall);
    } else if (state->type == SITEHELPER_COMMAND_DELETE_SLAB) {
        slab_destroy(&state->deleted_slab.slab);
    }
    free(state);
}

int sitehelper_command_undo_with_state(
    SiteHelperProject *project, const SiteHelperCommand *command,
    const SiteHelperCommandResult *result, const SiteHelperCommandUndoState *state)
{
    if (project == NULL || command == NULL || result == NULL || command->type != result->type) {
        return 0;
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_SLAB) {
        DomainId id=command->data.delete_slab.slab_id;
        return state != NULL && state->type == command->type &&
            result->data.slab.slab_id == id && state->deleted_slab.slab.id == id &&
            project->domain_ids.next != DOMAIN_ID_INVALID && id < project->domain_ids.next &&
            sitehelper_project_insert_slab_at(project,state->deleted_slab.storey_id,
                &state->deleted_slab.slab,state->deleted_slab.index);
    }
    if (command->type == SITEHELPER_COMMAND_EDIT_OPENING) {
        if (state == NULL || state->type != command->type ||
            state->previous_opening.wall_id != command->data.edit_opening.wall_id ||
            state->previous_opening.opening_id != command->data.edit_opening.opening_id ||
            state->previous_opening.wall_id != result->data.edit_opening.wall_id ||
            state->previous_opening.opening_id != result->data.edit_opening.opening_id) {
            return 0;
        }
        return edit_opening_command_execute(project, &state->previous_opening);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR ||
        command->type == SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT) {
        DomainId id = command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR
            ? command->data.delete_room_separator.separator_id
            : command->data.move_room_separator_endpoint.separator_id;
        if (state == NULL || state->type != command->type ||
            state->room_separator.definition.id != id || result->data.room_separator.separator_id != id) {
            return 0;
        }
        if (command->type == SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR) {
            return sitehelper_project_insert_room_separator(project, state->room_separator.storey_id,
                &state->room_separator.definition, state->room_separator.index);
        }
        return sitehelper_project_set_room_separator_segment(project, id,
            state->room_separator.definition.segment);
    }
    if (command->type == SITEHELPER_COMMAND_SET_ROOM_LOCATION) {
        if (state == NULL || state->type != command->type ||
            state->previous_room_location.room_id != command->data.room_location.room_id ||
            state->previous_room_location.room_id != result->data.room_location.room_id) {
            return 0;
        }
        return room_location_command_execute(project, &state->previous_room_location);
    }
    if (command->type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        if (state == NULL || state->type != command->type ||
            state->moved_wall.wall_id != command->data.move_wall_endpoint.wall_id ||
            state->moved_wall.wall_id != result->data.move_wall_endpoint.wall_id) {
            return 0;
        }
        Wall *wall = sitehelper_project_find_wall_by_id(project, state->moved_wall.wall_id);
        const Storey *storey = sitehelper_project_find_owning_storey_const(project, state->moved_wall.wall_id);
        BuildSettings resolved;
        return storey != NULL && sitehelper_project_resolve_storey_build_settings(project, storey->id, &resolved) &&
            wall_apply_plan_segment(wall, &resolved, state->moved_wall.segment);
    }
    if (command->type == SITEHELPER_COMMAND_DELETE_WALL) {
        if (state == NULL || state->type != command->type ||
            state->deleted_wall.wall_id != command->data.delete_wall.wall_id ||
            state->deleted_wall.wall_id != result->data.delete_wall.wall_id) {
            return 0;
        }
        return deleted_wall_snapshot_restore(project, &state->deleted_wall);
    }
    return sitehelper_command_undo(project, command, result);
}

int sitehelper_command_from_edit_opening(const EditOpeningCommand *edit,
    SiteHelperCommand *command)
{
    if (edit == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_EDIT_OPENING, .data.edit_opening = *edit
    };
    return 1;
}

int sitehelper_command_from_create_slab(const CreateSlabCommand *create,
    SiteHelperCommand *command)
{
    if (create == NULL || command == NULL) { return 0; }
    SiteHelperCommand candidate={.type=SITEHELPER_COMMAND_CREATE_SLAB};
    if (!create_slab_command_clone(create,&candidate.data.create_slab)) { return 0; }
    *command=candidate; return 1;
}

int sitehelper_command_from_delete_slab(const DeleteSlabCommand *deletion,
    SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command=(SiteHelperCommand){.type=SITEHELPER_COMMAND_DELETE_SLAB,
        .data.delete_slab=*deletion}; return 1;
}

int sitehelper_command_clone(const SiteHelperCommand *source, SiteHelperCommand *output)
{
    if (source == NULL || output == NULL || source->type <= SITEHELPER_COMMAND_NONE ||
        source->type >= SITEHELPER_COMMAND_COUNT) { return 0; }
    SiteHelperCommand candidate=*source;
    if (source->type == SITEHELPER_COMMAND_CREATE_SLAB) {
        candidate.data.create_slab=(CreateSlabCommand){0};
        if (!create_slab_command_clone(&source->data.create_slab,
            &candidate.data.create_slab)) { return 0; }
    }
    *output=candidate; return 1;
}

void sitehelper_command_destroy(SiteHelperCommand *command)
{
    if (command == NULL) { return; }
    if (command->type == SITEHELPER_COMMAND_CREATE_SLAB) {
        create_slab_command_destroy(&command->data.create_slab);
    }
    *command=(SiteHelperCommand){0};
}

int sitehelper_command_from_delete_wall(
    const DeleteWallCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) {
        return 0;
    }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_DELETE_WALL,
        .data.delete_wall = *deletion
    };
    return 1;
}

int sitehelper_command_from_move_wall_endpoint(
    const MoveWallEndpointCommand *move, SiteHelperCommand *command)
{
    if (move == NULL || command == NULL) {
        return 0;
    }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
        .data.move_wall_endpoint = *move
    };
    return 1;
}

int sitehelper_command_from_room_location(
    const RoomLocationCommand *placement, SiteHelperCommand *command)
{
    if (placement == NULL || command == NULL) {
        return 0;
    }
    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_SET_ROOM_LOCATION,
        .data.room_location = *placement
    };
    return 1;
}

int sitehelper_command_from_add_room_separator(const AddRoomSeparatorCommand *add, SiteHelperCommand *command)
{
    if (add == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR, .data.add_room_separator = *add};
    return 1;
}

int sitehelper_command_from_delete_room_separator(const DeleteRoomSeparatorCommand *deletion, SiteHelperCommand *command)
{
    if (deletion == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR, .data.delete_room_separator = *deletion};
    return 1;
}

int sitehelper_command_from_move_room_separator_endpoint(const MoveRoomSeparatorEndpointCommand *move, SiteHelperCommand *command)
{
    if (move == NULL || command == NULL) { return 0; }
    *command = (SiteHelperCommand){.type = SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT, .data.move_room_separator_endpoint = *move};
    return 1;
}

int sitehelper_command_from_opening(
    const OpeningCommand *opening,
    SiteHelperCommand *command
)
{
    if (
        opening == NULL
        || command == NULL
    ) {
        return 0;
    }

    *command = (SiteHelperCommand){
        .type =
            SITEHELPER_COMMAND_ADD_OPENING,

        .data.opening =
            *opening
    };

    return 1;
}

int sitehelper_command_from_wall(
    const WallCommand *wall,
    SiteHelperCommand *command
)
{
    if (wall == NULL || command == NULL) {
        return 0;
    }

    *command = (SiteHelperCommand){
        .type = SITEHELPER_COMMAND_ADD_WALL,
        .data.wall = *wall
    };

    return 1;
}

int sitehelper_command_execute(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    SiteHelperCommandResult *result
)
{
    if (
        project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    /*
     * Failure must never leave stale
     * information in the result.
     */
    *result =
        (SiteHelperCommandResult){
            .type =
                SITEHELPER_COMMAND_NONE
        };

    switch (command->type) {

        case SITEHELPER_COMMAND_CREATE_SLAB: {
            DomainId id;
            if (!create_slab_command_execute(project,&command->data.create_slab,&id)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,.data.slab={id}};
            return 1;
        }
        case SITEHELPER_COMMAND_DELETE_SLAB:
            if (!delete_slab_command_execute(project,&command->data.delete_slab)) { return 0; }
            *result=(SiteHelperCommandResult){.type=command->type,
                .data.slab={command->data.delete_slab.slab_id}};
            return 1;

        case SITEHELPER_COMMAND_EDIT_OPENING:
            if (!edit_opening_command_execute(project, &command->data.edit_opening)) { return 0; }
            *result = (SiteHelperCommandResult){
                .type = command->type,
                .data.edit_opening = {command->data.edit_opening.wall_id,
                    command->data.edit_opening.opening_id}
            };
            return 1;

        case SITEHELPER_COMMAND_ADD_OPENING:
        {
            DomainId opening_id =
                DOMAIN_ID_INVALID;

            if (!opening_command_execute(
                    project,
                    &command->data.opening,
                    &opening_id)) {

                return 0;
            }

            *result =
                (SiteHelperCommandResult){
                    .type =
                        SITEHELPER_COMMAND_ADD_OPENING,

                    .data.add_opening = {

                        .wall_id =
                            command->data.opening.wall_id,

                        .opening_id =
                            opening_id
                    }
                };

            return 1;
        }

        case SITEHELPER_COMMAND_ADD_WALL:
        {
            DomainId wall_id = DOMAIN_ID_INVALID;

            if (!wall_command_execute(
                    project,
                    &command->data.wall,
                    &wall_id)) {

                return 0;
            }

            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_ADD_WALL,
                .data.add_wall = {
                    .wall_id = wall_id
                }
            };

            return 1;
        }

        case SITEHELPER_COMMAND_DELETE_WALL:
            if (!delete_wall_command_execute(project, &command->data.delete_wall)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_DELETE_WALL,
                .data.delete_wall.wall_id = command->data.delete_wall.wall_id
            };
            return 1;

        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
            if (!move_wall_endpoint_command_execute(project, &command->data.move_wall_endpoint)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT,
                .data.move_wall_endpoint.wall_id = command->data.move_wall_endpoint.wall_id
            };
            return 1;

        case SITEHELPER_COMMAND_SET_ROOM_LOCATION:
            if (!room_location_command_execute(project, &command->data.room_location)) {
                return 0;
            }
            *result = (SiteHelperCommandResult){
                .type = SITEHELPER_COMMAND_SET_ROOM_LOCATION,
                .data.room_location.room_id = command->data.room_location.room_id
            };
            return 1;

        case SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR: {
            DomainId id;
            if (!add_room_separator_command_execute(project, &command->data.add_room_separator, &id)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type, .data.room_separator.separator_id = id};
            return 1;
        }
        case SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR:
            if (!delete_room_separator_command_execute(project, &command->data.delete_room_separator)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.room_separator.separator_id = command->data.delete_room_separator.separator_id};
            return 1;
        case SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT:
            if (!move_room_separator_endpoint_command_execute(project, &command->data.move_room_separator_endpoint)) { return 0; }
            *result = (SiteHelperCommandResult){.type = command->type,
                .data.room_separator.separator_id = command->data.move_room_separator_endpoint.separator_id};
            return 1;

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}

int sitehelper_command_undo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
)
{
    if (
        project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    if (
        command->type
        != result->type
    ) {
        return 0;
    }

    switch (command->type) {

        case SITEHELPER_COMMAND_CREATE_SLAB:
            return result->data.slab.slab_id != DOMAIN_ID_INVALID &&
                create_slab_command_undo(project,&command->data.create_slab,
                    result->data.slab.slab_id);

        case SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR: {
            Storey *storey = sitehelper_project_find_storey_by_id(project, command->data.add_room_separator.storey_id);
            return storey != NULL && build_remove_room_separator_by_id(&storey->structure, result->data.room_separator.separator_id);
        }

        case SITEHELPER_COMMAND_ADD_OPENING:

            return opening_command_undo(
                project,
                &command->data.opening,
                result->data.add_opening.opening_id
            );

        case SITEHELPER_COMMAND_ADD_WALL:
            return wall_command_undo(
                project,
                &command->data.wall,
                result->data.add_wall.wall_id
            );

        case SITEHELPER_COMMAND_DELETE_WALL:
        case SITEHELPER_COMMAND_DELETE_SLAB:
        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
        case SITEHELPER_COMMAND_EDIT_OPENING:
        case SITEHELPER_COMMAND_SET_ROOM_LOCATION:
        case SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR:
        case SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT:
            return 0; /* Requires history-owned state, never a public result. */

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}

int sitehelper_command_redo(
    SiteHelperProject *project,
    const SiteHelperCommand *command,
    const SiteHelperCommandResult *result
)
{
    if (
        project == NULL
        || command == NULL
        || result == NULL
    ) {
        return 0;
    }

    if (
        command->type
        != result->type
    ) {
        return 0;
    }

    switch (command->type) {

        case SITEHELPER_COMMAND_CREATE_SLAB:
            return create_slab_command_redo(project,&command->data.create_slab,
                result->data.slab.slab_id);
        case SITEHELPER_COMMAND_DELETE_SLAB:
            return command->data.delete_slab.slab_id == result->data.slab.slab_id &&
                delete_slab_command_execute(project,&command->data.delete_slab);

        case SITEHELPER_COMMAND_EDIT_OPENING:
            if (command->data.edit_opening.wall_id != result->data.edit_opening.wall_id ||
                command->data.edit_opening.opening_id != result->data.edit_opening.opening_id) { return 0; }
            return edit_opening_command_execute(project, &command->data.edit_opening);

        case SITEHELPER_COMMAND_ADD_ROOM_SEPARATOR: {
            RoomSeparator separator = {.id = result->data.room_separator.separator_id,
                .segment = command->data.add_room_separator.segment};
            Storey *storey = sitehelper_project_find_storey_by_id(project, command->data.add_room_separator.storey_id);
            return storey != NULL && sitehelper_project_insert_room_separator(project, storey->id, &separator,
                storey->structure.room_separator_count);
        }
        case SITEHELPER_COMMAND_DELETE_ROOM_SEPARATOR:
            if (command->data.delete_room_separator.separator_id != result->data.room_separator.separator_id) { return 0; }
            return delete_room_separator_command_execute(project, &command->data.delete_room_separator);
        case SITEHELPER_COMMAND_MOVE_ROOM_SEPARATOR_ENDPOINT:
            if (command->data.move_room_separator_endpoint.separator_id != result->data.room_separator.separator_id) { return 0; }
            return move_room_separator_endpoint_command_execute(project, &command->data.move_room_separator_endpoint);

        case SITEHELPER_COMMAND_ADD_OPENING:

            return opening_command_redo(
                project,
                &command->data.opening,
                result->data.add_opening.opening_id
            );

        case SITEHELPER_COMMAND_ADD_WALL:
            return wall_command_redo(
                project,
                &command->data.wall,
                result->data.add_wall.wall_id
            );

        case SITEHELPER_COMMAND_DELETE_WALL:
            if (command->data.delete_wall.wall_id != result->data.delete_wall.wall_id) {
                return 0;
            }
            return delete_wall_command_execute(project, &command->data.delete_wall);

        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
            if (command->data.move_wall_endpoint.wall_id != result->data.move_wall_endpoint.wall_id) {
                return 0;
            }
            return move_wall_endpoint_command_execute(project, &command->data.move_wall_endpoint);

        case SITEHELPER_COMMAND_SET_ROOM_LOCATION:
            if (command->data.room_location.room_id != result->data.room_location.room_id) {
                return 0;
            }
            return room_location_command_execute(project, &command->data.room_location);

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}
