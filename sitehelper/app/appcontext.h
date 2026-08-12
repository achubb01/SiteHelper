#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "sitehelper_model.h"
#include "domain_id.h"

typedef struct {
    BuildSettings settings;
    BuildStructure structure;

    DomainIdGenerator domain_ids;

    DomainId current_room_id;
    DomainId current_wall_id;

    bool room_selected;
    bool wall_selected;
} AppContext;

#endif