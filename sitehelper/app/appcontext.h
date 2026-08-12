#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "sitehelper_model.h"
#include "domain_id.h"
#include "sitehelper_project.h"

typedef struct
{
    SiteHelperProject project;

    DomainId current_room_id;
    DomainId current_wall_id;

    bool room_selected;
    bool wall_selected;
} AppContext;

#endif