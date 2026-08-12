#ifndef SITEHELPER_EDITOR_H
#define SITEHELPER_EDITOR_H

#include "domain_id.h"

typedef struct
{
    DomainId current_room_id;
    DomainId current_wall_id;
} SiteHelperEditor;

void sitehelper_editor_init(
    SiteHelperEditor *editor
);

#endif