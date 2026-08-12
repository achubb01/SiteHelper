#ifndef APPSTATE_H
#define APPSTATE_H

#include "appcontext.h"

Room *app_current_room(AppContext *app);
Wall *app_current_wall(AppContext *app);

#endif