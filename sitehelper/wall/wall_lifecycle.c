#include <stdlib.h>

#include "wall.h"

void wall_destroy(Wall *wall)
{
    if (wall == NULL) {
        return;
    }

    wall_framing_destroy(
        &wall->framing
    );

    free(
        wall->definition.openings
    );

    *wall = (Wall){0};
}

void wall_framing_destroy(
    WallFraming *framing
)
{
    if (framing == NULL) {
        return;
    }

    free(framing->studs);
    free(framing->nogs);
    free(framing->members);

    *framing = (WallFraming){0};
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