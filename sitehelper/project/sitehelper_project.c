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

int sitehelper_project_set_room_location(SiteHelperProject *project,
    DomainId room_id, PlanPosition location)
{
    if (project == NULL) {
        return 0;
    }
    Room *room = build_find_room_by_id(&project->structure, room_id);
    if (room == NULL) {
        return 0;
    }
    room->location = location;
    room->has_location = true;
    return 1;
}

int sitehelper_project_clear_room_location(SiteHelperProject *project,
    DomainId room_id)
{
    if (project == NULL) {
        return 0;
    }
    Room *room = build_find_room_by_id(&project->structure, room_id);
    if (room == NULL) {
        return 0;
    }
    room->has_location = false;
    room->location = (PlanPosition){0};
    return 1;
}

DomainId sitehelper_project_add_wall(
    SiteHelperProject *project,
    WallPlanSegment segment
)
{
    if (project == NULL) {

        return DOMAIN_ID_INVALID;
    }

    DomainIdGenerator candidate_ids = project->domain_ids;
    DomainId wall_id = domain_id_generate(&candidate_ids);

    if (wall_id == DOMAIN_ID_INVALID) {
        return DOMAIN_ID_INVALID;
    }

    Wall wall = { .id = wall_id };
    if (!wall_set_plan_segment(&wall, segment)) {
        return DOMAIN_ID_INVALID;
    }

    if (!build_append_wall(&project->structure, &wall)) {

        return DOMAIN_ID_INVALID;
    }

    project->domain_ids = candidate_ids;

    return wall_id;
}
