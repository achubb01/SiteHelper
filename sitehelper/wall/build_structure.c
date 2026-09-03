#include <stdlib.h>

#include "wall.h"

int build_add_room(
    BuildStructure *structure,
    DomainId room_id
)
{
    if (structure == NULL ||
        room_id == DOMAIN_ID_INVALID) {
        return 0;
    }

    if (build_find_room_by_id(
            structure,
            room_id) != NULL ||
        build_find_wall_by_id(structure, room_id) != NULL) {

        return 0;
    }

    if (structure->room_count ==
        structure->room_capacity) {

        size_t new_capacity =
            structure->room_capacity == 0
                ? 1
                : structure->room_capacity * 2;

        Room *new_rooms = realloc(
            structure->rooms,
            new_capacity * sizeof *new_rooms
        );

        if (new_rooms == NULL) {
            return 0;
        }

        structure->rooms = new_rooms;
        structure->room_capacity = new_capacity;
    }

    structure->rooms[
        structure->room_count
    ] = (Room){
        .id = room_id
    };

    structure->room_count++;

    return 1;
}

int room_add_wall_reference(
    Room *room,
    DomainId wall_id
)
{
    if (room == NULL ||
        wall_id == DOMAIN_ID_INVALID) {
        return 0;
    }

    if (room_has_wall_id(room, wall_id)) {

        return 0;
    }

    if (room->wall_count ==
        room->wall_capacity) {

        size_t new_capacity =
            room->wall_capacity == 0
                ? 1
                : room->wall_capacity * 2;

        DomainId *new_wall_ids = realloc(
            room->wall_ids,
            new_capacity * sizeof *new_wall_ids
        );

        if (new_wall_ids == NULL) {
            return 0;
        }

        room->wall_ids = new_wall_ids;
        room->wall_capacity = new_capacity;
    }

    room->wall_ids[room->wall_count] = wall_id;

    room->wall_count++;

    return 1;
}

int room_remove_wall_reference(Room *room, DomainId wall_id)
{
    if (room == NULL || wall_id == DOMAIN_ID_INVALID) {

        return 0;
    }

    for (size_t index = 0; index < room->wall_count; index++) {
        if (room->wall_ids[index] != wall_id) {
            continue;
        }

        for (size_t next = index + 1; next < room->wall_count; next++) {
            room->wall_ids[next - 1] = room->wall_ids[next];
        }

        room->wall_count--;
        room->wall_ids[room->wall_count] = DOMAIN_ID_INVALID;
        return 1;
    }

    return 0;
}

int room_has_wall_id(const Room *room, DomainId wall_id)
{
    if (room == NULL || wall_id == DOMAIN_ID_INVALID) {
        return 0;
    }

    for (size_t index = 0; index < room->wall_count; index++) {
        if (room->wall_ids[index] == wall_id) {
            return 1;
        }
    }

    return 0;
}

int build_append_wall(BuildStructure *structure, Wall *wall)
{
    if (structure == NULL || wall == NULL ||
        wall->id == DOMAIN_ID_INVALID ||
        build_find_wall_by_id(structure, wall->id) != NULL ||
        build_find_room_by_id(structure, wall->id) != NULL) {

        return 0;
    }

    if (structure->wall_count == structure->wall_capacity) {
        size_t new_capacity = structure->wall_capacity == 0
            ? 1
            : structure->wall_capacity * 2;
        Wall *new_walls = realloc(
            structure->walls,
            new_capacity * sizeof *new_walls
        );

        if (new_walls == NULL) {
            return 0;
        }

        structure->walls = new_walls;
        structure->wall_capacity = new_capacity;
    }

    structure->walls[structure->wall_count] = *wall;
    structure->wall_count++;
    *wall = (Wall){0};
    return 1;
}

int build_remove_wall_by_id(BuildStructure *structure, DomainId wall_id)
{
    if (structure == NULL || wall_id == DOMAIN_ID_INVALID) {
        return 0;
    }

    size_t index;
    for (index = 0; index < structure->wall_count; index++) {
        if (structure->walls[index].id == wall_id) {
            break;
        }
    }

    if (index == structure->wall_count) {
        return 0;
    }

    for (size_t room_index = 0;
         room_index < structure->room_count;
         room_index++) {

        (void)room_remove_wall_reference(
            &structure->rooms[room_index],
            wall_id
        );
    }

    wall_destroy(&structure->walls[index]);
    for (size_t next = index + 1;
         next < structure->wall_count;
         next++) {

        structure->walls[next - 1] = structure->walls[next];
    }

    structure->wall_count--;
    structure->walls[structure->wall_count] = (Wall){0};
    return 1;
}

Room *build_find_room_by_id(
    BuildStructure *structure,
    DomainId room_id
)
{
    if (structure == NULL ||
        room_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < structure->room_count;
         i++) {

        Room *room =
            &structure->rooms[i];

        if (room->id == room_id) {
            return room;
        }
    }

    return NULL;
}

const Room *build_find_room_by_id_const(
    const BuildStructure *structure,
    DomainId room_id
)
{
    if (structure == NULL ||
        room_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < structure->room_count;
         i++) {

        const Room *room =
            &structure->rooms[i];

        if (room->id == room_id) {
            return room;
        }
    }

    return NULL;
}

Wall *build_find_wall_by_id(
    BuildStructure *structure,
    DomainId wall_id
)
{
    if (structure == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < structure->wall_count;
         i++) {

        Wall *wall =
            &structure->walls[i];

        if (wall->id == wall_id) {
            return wall;
        }
    }

    return NULL;
}

const Wall *build_find_wall_by_id_const(
    const BuildStructure *structure,
    DomainId wall_id
)
{
    if (structure == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < structure->wall_count;
         i++) {

        const Wall *wall =
            &structure->walls[i];

        if (wall->id == wall_id) {
            return wall;
        }
    }

    return NULL;
}

int build_set_stud_spacing(
    BuildSettings *settings,
    int spacing
)
{
    if (settings == NULL) {
        return 0;
    }

    if (spacing <= 0) {
        return 0;
    }

    settings->stud_spacing = spacing;

    return 1;
}
