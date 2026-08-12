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
            room_id) != NULL) {

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

int room_add_wall(
    Room *room,
    DomainId wall_id
)
{
    if (room == NULL ||
        wall_id == DOMAIN_ID_INVALID) {
        return 0;
    }

    if (room_find_wall_by_id(
            room,
            wall_id) != NULL) {

        return 0;
    }

    if (room->wall_count ==
        room->wall_capacity) {

        size_t new_capacity =
            room->wall_capacity == 0
                ? 1
                : room->wall_capacity * 2;

        Wall *new_walls = realloc(
            room->walls,
            new_capacity * sizeof *new_walls
        );

        if (new_walls == NULL) {
            return 0;
        }

        room->walls = new_walls;
        room->wall_capacity = new_capacity;
    }

    room->walls[
        room->wall_count
    ] = (Wall){
        .id = wall_id
    };

    room->wall_count++;

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

Wall *room_find_wall_by_id(
    Room *room,
    DomainId wall_id
)
{
    if (room == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < room->wall_count;
         i++) {

        Wall *wall =
            &room->walls[i];

        if (wall->id == wall_id) {
            return wall;
        }
    }

    return NULL;
}

const Wall *room_find_wall_by_id_const(
    const Room *room,
    DomainId wall_id
)
{
    if (room == NULL ||
        wall_id == DOMAIN_ID_INVALID) {

        return NULL;
    }

    for (size_t i = 0;
         i < room->wall_count;
         i++) {

        const Wall *wall =
            &room->walls[i];

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