#include <stdlib.h>

#include "sitehelper_command.h"
#include "sitehelper_command_internal.h"
#include "delete_wall_command_internal.h"
#include "wall.h"

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
        command->type != SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        return 1;
    }
    SiteHelperCommandUndoState *candidate = calloc(1, sizeof *candidate);
    if (candidate == NULL) {
        return 0;
    }
    candidate->type = command->type;
    if (command->type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        const Wall *wall = build_find_wall_by_id_const(&project->structure,
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
    if (command->type == SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT) {
        if (state == NULL || state->type != command->type ||
            state->moved_wall.wall_id != command->data.move_wall_endpoint.wall_id ||
            state->moved_wall.wall_id != result->data.move_wall_endpoint.wall_id) {
            return 0;
        }
        Wall *wall = build_find_wall_by_id(&project->structure, state->moved_wall.wall_id);
        return wall_apply_plan_segment(wall, &project->settings, state->moved_wall.segment);
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
                        .room_id =
                            command->data.opening.room_id,

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
                    .room_id = command->data.wall.room_id,
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
        case SITEHELPER_COMMAND_MOVE_WALL_ENDPOINT:
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

        case SITEHELPER_COMMAND_NONE:
        case SITEHELPER_COMMAND_COUNT:
        default:
            return 0;
    }
}
