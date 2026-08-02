#ifndef WALL_QUERY_H
#define WALL_QUERY_H

#include "sitehelper_model.h"

const Timber *wall_find_timber_at_position(
    const Wall *wall,
    Position position
);

#endif