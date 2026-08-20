#include <stdlib.h>

#include "opening_command.h"

static int opening_command_copy_wall_definition(
    const Wall *source,
    Wall *candidate
)
{
    if (
        source == NULL
        || candidate == NULL
    ) {
        return 0;
    }

    *candidate = (Wall){
        .id = source->id,

        .definition = {
            .length =
                source->definition.length
        },

        .framing = {0}
    };

    if (
        source->definition.opening_count == 0
    ) {
        return 1;
    }

    size_t opening_count =
        source->definition.opening_count;

    Opening *openings =
        malloc(
            opening_count
            * sizeof *openings
        );

    if (openings == NULL) {
        return 0;
    }

    for (
        size_t i = 0;
        i < opening_count;
        i++
    ) {
        openings[i] =
            source->definition.openings[i];
    }

    candidate->definition.openings =
        openings;

    candidate->definition.opening_count =
        opening_count;

    candidate->definition.opening_capacity =
        opening_count;

    return 1;
}

int opening_command_create(
    DomainId room_id,
    DomainId wall_id,
    OpeningType type,
    int frame_position,
    int frame_bottom,
    int width,
    int height,
    OpeningCommand *command
)
{
    if (
        command == NULL
        || room_id == DOMAIN_ID_INVALID
        || wall_id == DOMAIN_ID_INVALID
        || frame_position < 0
        || frame_bottom < 0
        || width <= 0
        || height <= 0
    ) {
        return 0;
    }

    if (
        type != OPENING_DOOR
        && type != OPENING_WINDOW
    ) {
        return 0;
    }

    *command = (OpeningCommand){
        .room_id = room_id,
        .wall_id = wall_id,
        .type = type,
        .frame_position = frame_position,
        .frame_bottom = frame_bottom,
        .width = width,
        .height = height
    };

    return 1;
}

int opening_command_execute(
    SiteHelperProject *project,
    const OpeningCommand *command,
    DomainId *opening_id_out
)
{
    if (opening_id_out == NULL) {
        return 0;
    }

    *opening_id_out =
        DOMAIN_ID_INVALID;

    if (
        project == NULL
        || command == NULL
        || command->room_id == DOMAIN_ID_INVALID
        || command->wall_id == DOMAIN_ID_INVALID
    ) {
        return 0;
    }

    Room *room =
        build_find_room_by_id(
            &project->structure,
            command->room_id
        );

    if (room == NULL) {
        return 0;
    }

    Wall *wall =
        room_find_wall_by_id(
            room,
            command->wall_id
        );

    if (wall == NULL) {
        return 0;
    }

    Wall candidate;

    if (!opening_command_copy_wall_definition(
            wall,
            &candidate)) {
        return 0;
    }

    /*
     * Work with a candidate allocator.
     *
     * The project's allocator remains
     * completely untouched until commit.
     */
    DomainIdGenerator candidate_ids =
        project->domain_ids;

    DomainId opening_id =
        domain_id_generate(
            &candidate_ids
        );

    if (opening_id == DOMAIN_ID_INVALID) {
        wall_destroy(
            &candidate
        );

        return 0;
    }

    if (!wall_add_opening(
            &candidate,
            &project->settings,
            opening_id,
            command->type,
            command->frame_position,
            command->frame_bottom,
            command->width,
            command->height)) {

        wall_destroy(
            &candidate
        );

        return 0;
    }

    if (!wall_generate(
            &candidate,
            &project->settings)) {

        wall_destroy(
            &candidate
        );

        return 0;
    }

    /*
     * Everything that can fail has now
     * succeeded.
     *
     * Commit the model mutation and the
     * identity allocation together.
     */
    Wall previous =
        *wall;

    *wall =
        candidate;

    project->domain_ids =
        candidate_ids;

    wall_destroy(
        &previous
    );

    *opening_id_out =
        opening_id;

    return 1;
}

int opening_command_undo(
    SiteHelperProject *project,
    const OpeningCommand *command,
    DomainId opening_id
)
{
    if (
        project == NULL
        || command == NULL
        || opening_id == DOMAIN_ID_INVALID
    ) {
        return 0;
    }

    Room *room =
        build_find_room_by_id(
            &project->structure,
            command->room_id
        );

    if (room == NULL) {
        return 0;
    }

    Wall *wall =
        room_find_wall_by_id(
            room,
            command->wall_id
        );

    if (wall == NULL) {
        return 0;
    }

    Wall candidate = {0};

    if (!opening_command_copy_wall_definition(
            wall,
            &candidate)) {
        return 0;
    }

    if (!wall_remove_opening_by_id(
            &candidate,
            opening_id)) {

        wall_destroy(
            &candidate
        );

        return 0;
    }

    if (!wall_generate(
            &candidate,
            &project->settings)) {

        wall_destroy(
            &candidate
        );

        return 0;
    }

    Wall previous =
        *wall;

    *wall =
        candidate;

    wall_destroy(
        &previous
    );

    return 1;
}