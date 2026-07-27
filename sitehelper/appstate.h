#ifndef APPSTATE_H
#define APPSTATE_H

#include "appcontext.h"

// void app_set_current_room(AppContext *app);
// void app_set_current_wall(AppContext *app);
Room *build_get_room(BuildStructure *structure, size_t index);
Room *app_current_room(AppContext *app);
Wall *build_get_wall(BuildStructure *structure, size_t index, size_t index2);
Wall *app_current_wall(AppContext *app);

#endif