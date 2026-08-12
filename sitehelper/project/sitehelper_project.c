#include "sitehelper_project.h"

void sitehelper_project_init(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return;
    }

    *project = (SiteHelperProject){0};

    project->settings = (BuildSettings){
        .stud_height = 2400,
        .stud_depth = 90,
        .stud_width = 35,

        .stud_spacing = 600,
        .nog_spacing = 1200,

        .opening_width_allowance = 0,
        .opening_height_allowance = 0,

        .stud_spacing_mode =
            STUD_SPACING_MAXIMISE
    };

    domain_id_generator_init(
        &project->domain_ids
    );
}

void sitehelper_project_destroy(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return;
    }

    build_destroy(
        &project->structure
    );

    *project = (SiteHelperProject){0};
}