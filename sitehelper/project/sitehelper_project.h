#ifndef SITEHELPER_PROJECT_H
#define SITEHELPER_PROJECT_H

#include "build_settings.h"
#include "build_structure.h"
#include "domain_id.h"

typedef struct
{
    BuildSettings settings;
    BuildStructure structure;
    DomainIdGenerator domain_ids;
} SiteHelperProject;

void sitehelper_project_init(
    SiteHelperProject *project
);

void sitehelper_project_destroy(
    SiteHelperProject *project
);

#endif