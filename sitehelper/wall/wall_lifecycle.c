#include <stdlib.h>

#include "wall.h"

void wall_destroy(
    Wall *wall
)
{
    if (wall == NULL) {
        return;
    }

    free(wall->studs);
    free(wall->nogs);
    free(wall->members);
    free(wall->openings);

    *wall = (Wall){0};
}

void room_destroy(
    Room *room
)
{
    if (room == NULL) {
        return;
    }

    for (size_t i = 0;
         i < room->wall_count;
         i++) {

        wall_destroy(
            &room->walls[i]
        );
    }

    free(room->walls);

    *room = (Room){0};
}

void build_destroy(
    BuildStructure *structure
)
{
    if (structure == NULL) {
        return;
    }

    for (size_t i = 0;
         i < structure->room_count;
         i++) {

        room_destroy(
            &structure->rooms[i]
        );
    }

    free(structure->rooms);

    *structure = (BuildStructure){0};
}