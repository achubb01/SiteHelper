#ifndef WALL_QUERY_H
#define WALL_QUERY_H

#include "appcontext.h"

const Timber *wall_find_timber_at_position(
    const Wall *wall,
    Position position
);

#endif