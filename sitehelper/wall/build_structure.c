#include <stdlib.h>

#include "wall.h"

int build_add_room(BuildStructure *structure)
{
    if (structure == NULL) {
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

    structure->rooms[structure->room_count] =
        (Room){0};

    structure->room_count++;

    return 1;
}

int room_add_wall(Room *room)
{
    if (room == NULL) {
        return 0;
    }

    if (room->wall_count == room->wall_capacity) {
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

    room->walls[room->wall_count] = (Wall){0};
    room->wall_count++;

    return 1;
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