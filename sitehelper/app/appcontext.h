#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "sitehelper_model.h"

typedef struct {
    BuildSettings settings;
    BuildStructure structure;

    size_t current_room;
    size_t current_wall;

    bool room_selected;
    bool wall_selected;
} AppContext;

#endif