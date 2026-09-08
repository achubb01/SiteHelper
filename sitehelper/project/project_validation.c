#include "sitehelper_project.h"
#include "wall.h"

static SiteHelperProjectValidation validation(SiteHelperProjectValidationCode code,
    DomainId subject, DomainId related)
{
    return (SiteHelperProjectValidation){code, subject, related};
}

/* Called only after every collection's metadata has been checked. Scanning
 * trades quadratic time for no allocation; stop at the second occurrence. */
static int duplicate_id(const BuildStructure *structure, DomainId id)
{
    int seen = 0;
    for (size_t i = 0; i < structure->room_count; i++) {
        if (structure->rooms[i].id == id && seen++) {
            return 1;
        }
    }
    for (size_t i = 0; i < structure->wall_count; i++) {
        const Wall *wall = &structure->walls[i];
        if (wall->id == id && seen++) {
            return 1;
        }
        for (size_t j = 0; j < wall->definition.opening_count; j++) {
            if (wall->definition.openings[j].id == id && seen++) {
                return 1;
            }
        }
    }
    return 0;
}

static SiteHelperProjectValidation validate_id(const BuildStructure *structure,
    DomainId id, SiteHelperProjectValidationCode invalid_code, DomainId parent,
    DomainId *maximum)
{
    if (id == DOMAIN_ID_INVALID) {
        return validation(invalid_code, id, parent);
    }
    if (duplicate_id(structure, id)) {
        return validation(SITEHELPER_PROJECT_DUPLICATE_ID, id, DOMAIN_ID_INVALID);
    }
    if (id > *maximum) {
        *maximum = id;
    }
    return validation(SITEHELPER_PROJECT_VALID, DOMAIN_ID_INVALID, DOMAIN_ID_INVALID);
}

SiteHelperProjectValidation sitehelper_project_validate(const SiteHelperProject *project)
{
    if (project == NULL) {
        return validation(SITEHELPER_PROJECT_INVALID_ARGUMENT, 0, 0);
    }
    if (!build_settings_valid(&project->settings)) {
        return validation(SITEHELPER_PROJECT_INVALID_SETTINGS, 0, 0);
    }
    const BuildStructure *structure = &project->structure;
    if (structure->room_count > structure->room_capacity ||
        (structure->room_count != 0 && structure->rooms == NULL)) {
        return validation(SITEHELPER_PROJECT_INVALID_ROOM_COLLECTION, 0, 0);
    }
    if (structure->wall_count > structure->wall_capacity ||
        (structure->wall_count != 0 && structure->walls == NULL)) {
        return validation(SITEHELPER_PROJECT_INVALID_WALL_COLLECTION, 0, 0);
    }
    /* Establish safe metadata for the entire graph before any lookup or ID
     * scan can follow a reference into a later collection. */
    for (size_t i = 0; i < structure->room_count; i++) {
        const Room *room = &structure->rooms[i];
        if (room->wall_count > room->wall_capacity ||
            (room->wall_count != 0 && room->wall_ids == NULL)) {
            return validation(SITEHELPER_PROJECT_INVALID_ROOM_REFERENCE_COLLECTION, room->id, 0);
        }
    }
    for (size_t i = 0; i < structure->wall_count; i++) {
        const Wall *wall = &structure->walls[i];
        if (wall->definition.opening_count > wall->definition.opening_capacity ||
            (wall->definition.opening_count != 0 && wall->definition.openings == NULL)) {
            return validation(SITEHELPER_PROJECT_INVALID_OPENING_COLLECTION, wall->id, 0);
        }
    }
    DomainId maximum = DOMAIN_ID_INVALID;
    for (size_t i = 0; i < structure->room_count; i++) {
        SiteHelperProjectValidation result = validate_id(structure, structure->rooms[i].id,
            SITEHELPER_PROJECT_INVALID_ROOM_ID, 0, &maximum);
        if (result.code != SITEHELPER_PROJECT_VALID) {
            return result;
        }
    }
    for (size_t i = 0; i < structure->wall_count; i++) {
        const Wall *wall = &structure->walls[i];
        SiteHelperProjectValidation result = validate_id(structure, wall->id,
            SITEHELPER_PROJECT_INVALID_WALL_ID, 0, &maximum);
        if (result.code != SITEHELPER_PROJECT_VALID) {
            return result;
        }
        for (size_t j = 0; j < wall->definition.opening_count; j++) {
            result = validate_id(structure, wall->definition.openings[j].id,
                SITEHELPER_PROJECT_INVALID_OPENING_ID, wall->id, &maximum);
            if (result.code != SITEHELPER_PROJECT_VALID) {
                return result;
            }
        }
    }
    if (project->domain_ids.next == DOMAIN_ID_INVALID || project->domain_ids.next <= maximum) {
        return validation(SITEHELPER_PROJECT_INVALID_ID_GENERATOR, maximum, 0);
    }
    for (size_t i = 0; i < structure->room_count; i++) {
        const Room *room = &structure->rooms[i];
        for (size_t j = 0; j < room->wall_count; j++) {
            DomainId wall_id = room->wall_ids[j];
            if (build_find_wall_by_id_const(structure, wall_id) == NULL) {
                return validation(SITEHELPER_PROJECT_UNRESOLVED_WALL_REFERENCE, room->id, wall_id);
            }
            for (size_t previous = 0; previous < j; previous++) {
                if (room->wall_ids[previous] == wall_id) {
                    return validation(SITEHELPER_PROJECT_DUPLICATE_WALL_REFERENCE, room->id, wall_id);
                }
            }
        }
    }
    for (size_t i = 0; i < structure->wall_count; i++) {
        const Wall *wall = &structure->walls[i];
        if (wall_length_mm(wall) == 0) {
            return validation(SITEHELPER_PROJECT_INVALID_WALL_GEOMETRY, wall->id, 0);
        }
        /* Borrow authoritative definitions only, exposing the preceding
         * openings to the normal local validator without self-conflict. */
        Wall prefix = {.id = wall->id, .definition = wall->definition};
        prefix.definition.opening_count = 0;
        for (size_t j = 0; j < wall->definition.opening_count; j++) {
            const Opening *opening = &wall->definition.openings[j];
            WallOpeningProposal proposal = {
                .type = opening->type, .frame_position = opening->frame_position,
                .frame_bottom = opening->frame_bottom, .width = opening->width,
                .height = opening->height, .width_allowance = opening->width_allowance,
                .height_allowance = opening->height_allowance, .custom_allowance = opening->custom_allowance
            };
            WallOpeningValidation result = wall_validate_opening(&prefix, &project->settings, &proposal);
            if (result.code == WALL_OPENING_OVERLAPS_OPENING) {
                return validation(SITEHELPER_PROJECT_OVERLAPPING_OPENINGS, opening->id,
                    result.conflicting_opening_id);
            }
            if (result.code != WALL_OPENING_VALID) {
                return validation(SITEHELPER_PROJECT_INVALID_OPENING, opening->id, wall->id);
            }
            prefix.definition.opening_count++;
        }
    }
    return validation(SITEHELPER_PROJECT_VALID, 0, 0);
}
