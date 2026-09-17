#include <stdlib.h>
#include "sitehelper_project.h"

#include "wall.h"
#include "slab.h"
#include "roof.h"

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

    for (size_t i = 0; i < project->storey_count; i++) {
        build_destroy(&project->storeys[i].structure);
        slab_collection_destroy(&project->storeys[i].slabs);
        roof_collection_destroy(&project->storeys[i].roofs);
    }
    free(project->storeys);

    *project = (SiteHelperProject){0};
}

DomainId sitehelper_project_add_room(SiteHelperProject *project, DomainId storey_id)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, id) ||
        !build_add_room(&storey->structure, id)) { return DOMAIN_ID_INVALID; }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_set_room_location(SiteHelperProject *project,
    DomainId room_id, PlanPosition location)
{
    if (project == NULL) {
        return 0;
    }
    Room *room = sitehelper_project_find_room_by_id(project, room_id);
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
    Room *room = sitehelper_project_find_room_by_id(project, room_id);
    if (room == NULL) {
        return 0;
    }
    room->has_location = false;
    room->location = (PlanPosition){0};
    return 1;
}

DomainId sitehelper_project_add_wall(
    SiteHelperProject *project, DomainId storey_id,
    WallPlanSegment segment
)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL) {

        return DOMAIN_ID_INVALID;
    }

    DomainIdGenerator candidate_ids = project->domain_ids;
    DomainId wall_id = domain_id_generate(&candidate_ids);

    if (wall_id == DOMAIN_ID_INVALID || candidate_ids.next == DOMAIN_ID_INVALID ||
        sitehelper_project_contains_domain_id(project, wall_id)) {
        return DOMAIN_ID_INVALID;
    }

    Wall wall = { .id = wall_id };
    if (!wall_set_plan_segment(&wall, segment)) {
        return DOMAIN_ID_INVALID;
    }

    if (!build_append_wall(&storey->structure, &wall)) {

        return DOMAIN_ID_INVALID;
    }

    project->domain_ids = candidate_ids;

    return wall_id;
}

Storey *sitehelper_project_find_storey_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        if (project->storeys[i].id == id) { return &project->storeys[i]; }
    }
    return NULL;
}

const Storey *sitehelper_project_find_storey_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        if (project->storeys[i].id == id) { return &project->storeys[i]; }
    }
    return NULL;
}

Storey *sitehelper_project_find_owning_storey(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Storey *s = &project->storeys[i];
        if (s->id == id || build_contains_domain_id(&s->structure, id) ||
            slab_collection_find_by_id_const(&s->slabs, id) != NULL ||
            roof_collection_find_by_id_const(&s->roofs, id) != NULL) { return s; }
        for (size_t r = 0; r < s->roofs.count; r++) {
            if (roof_find_portion_by_id_const(&s->roofs.items[r], id) != NULL) { return s; }
        }
    }
    return NULL;
}

const Storey *sitehelper_project_find_owning_storey_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Storey *s = &project->storeys[i];
        if (s->id == id || build_contains_domain_id(&s->structure, id) ||
            slab_collection_find_by_id_const(&s->slabs, id) != NULL ||
            roof_collection_find_by_id_const(&s->roofs, id) != NULL) { return s; }
        for (size_t r = 0; r < s->roofs.count; r++) {
            if (roof_find_portion_by_id_const(&s->roofs.items[r], id) != NULL) { return s; }
        }
    }
    return NULL;
}

int sitehelper_project_contains_domain_id(const SiteHelperProject *project, DomainId id)
{
    return sitehelper_project_find_owning_storey_const(project, id) != NULL;
}

int sitehelper_project_insert_storey(SiteHelperProject *project, DomainId id, int elevation_mm)
{
    if (project == NULL || id == DOMAIN_ID_INVALID ||
        project->storey_count > project->storey_capacity ||
        (project->storey_capacity != 0 && project->storeys == NULL) ||
        sitehelper_project_contains_domain_id(project, id)) { return 0; }
    if (project->storey_count == project->storey_capacity) {
        size_t maximum = SIZE_MAX / sizeof *project->storeys;
        size_t capacity = project->storey_capacity;
        if (capacity >= maximum) { return 0; }
        size_t grown = capacity == 0 ? 1 : capacity > maximum / 2 ? maximum : capacity * 2;
        Storey *storage = realloc(project->storeys, grown * sizeof *storage);
        if (storage == NULL) { return 0; }
        project->storeys = storage;
        project->storey_capacity = grown;
    }
    project->storeys[project->storey_count++] = (Storey){.id = id, .elevation_mm = elevation_mm};
    return 1;
}

DomainId sitehelper_project_add_storey(SiteHelperProject *project, int elevation_mm)
{
    if (project == NULL) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        !sitehelper_project_insert_storey(project, id, elevation_mm)) { return DOMAIN_ID_INVALID; }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_remove_wall_by_id(SiteHelperProject *project, DomainId id)
{
    Storey *storey = sitehelper_project_find_owning_storey(project, id);
    return storey != NULL && build_remove_wall_by_id(&storey->structure, id);
}

Room *sitehelper_project_find_room_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Room *found = build_find_room_by_id(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

const Room *sitehelper_project_find_room_by_id_const(const SiteHelperProject *project, DomainId id)
{
    return sitehelper_project_find_room_with_owner_const(project, id, NULL);
}

const Room *sitehelper_project_find_room_with_owner_const(const SiteHelperProject *project,
    DomainId id, const Storey **owner)
{
    if (owner != NULL) { *owner = NULL; }
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Room *found = build_find_room_by_id_const(&project->storeys[i].structure, id);
        if (found != NULL) {
            if (owner != NULL) { *owner = &project->storeys[i]; }
            return found;
        }
    }
    return NULL;
}

Wall *sitehelper_project_find_wall_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Wall *found = build_find_wall_by_id(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

const Wall *sitehelper_project_find_wall_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Wall *found = build_find_wall_by_id_const(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

RoomSeparator *sitehelper_project_find_room_separator_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        RoomSeparator *found = build_find_room_separator_by_id(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}

const RoomSeparator *sitehelper_project_find_room_separator_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const RoomSeparator *found = build_find_room_separator_by_id_const(&project->storeys[i].structure, id);
        if (found != NULL) { return found; }
    }
    return NULL;
}
