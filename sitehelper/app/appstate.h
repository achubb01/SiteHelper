#ifndef APPSTATE_H
#define APPSTATE_H

#include "sitehelper_project.h"
#include "sitehelper_editor.h"

Room *app_current_room(
    SiteHelperProject *project,
    const SiteHelperEditor *editor
);

Wall *app_current_wall(
    SiteHelperProject *project,
    const SiteHelperEditor *editor
);

#endif