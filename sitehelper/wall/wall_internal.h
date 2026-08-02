#ifndef WALL_INTERNAL_H
#define WALL_INTERNAL_H

#include "wall.h"

typedef int (*PositionCallback)(
    int position,
    void *context
);

typedef struct
{
    Wall *wall;
    const BuildSettings *settings;

    int y;
    int length;

    StudType type;
} StudGenerationContext;

int wall_generate_positions(
    int start,
    int end,
    const BuildSettings *settings,
    PositionCallback callback,
    void *context
);

void wall_clear_noggins(
    Wall *wall
);

int wall_generate_noggins(
    Wall *wall,
    const BuildSettings *settings
);

void wall_clear_studs(
    Wall *wall
);

int wall_add_custom_stud(
    Wall *wall,
    const BuildSettings *settings,
    int x,
    int y,
    int length,
    StudType type
);

void wall_remove_stud(
    Wall *wall,
    size_t index
);

int wall_compare_stud_position(
    const void *a,
    const void *b
);

void wall_clear_members(
    Wall *wall
);

int wall_add_header(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
);

int wall_add_sill(
    Wall *wall,
    const BuildSettings *settings,
    const Opening *opening
);

int wall_apply_openings(
    Wall *wall,
    const BuildSettings *settings
);

int wall_repair_stud_spacing(
    Wall *wall,
    const BuildSettings *settings
);

int wall_add_stud_at_position(
    int position,
    void *context
);

#endif