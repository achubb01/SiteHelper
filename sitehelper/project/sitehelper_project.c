#include "sitehelper_project.h"

#include "wall.h"

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

DomainId sitehelper_project_add_room(
    SiteHelperProject *project
)
{
    if (project == NULL) {
        return DOMAIN_ID_INVALID;
    }

    DomainId room_id =
        domain_id_generate(
            &project->domain_ids
        );

    if (room_id == DOMAIN_ID_INVALID) {
        return DOMAIN_ID_INVALID;
    }

    if (!build_add_room(
            &project->structure,
            room_id)) {

        return DOMAIN_ID_INVALID;
    }

    return room_id;
}

DomainId sitehelper_project_add_wall(
    SiteHelperProject *project,
    DomainId room_id
)
{
    if (project == NULL ||
        room_id == DOMAIN_ID_INVALID) {

        return DOMAIN_ID_INVALID;
    }

    Room *room =
        build_find_room_by_id(
            &project->structure,
            room_id
        );

    if (room == NULL) {
        return DOMAIN_ID_INVALID;
    }

    DomainId wall_id =
        domain_id_generate(
            &project->domain_ids
        );

    if (wall_id == DOMAIN_ID_INVALID) {
        return DOMAIN_ID_INVALID;
    }

    if (!room_add_wall(
            room,
            wall_id)) {

        return DOMAIN_ID_INVALID;
    }

    return wall_id;
}