#include "sitehelper_project.h"
#include "wall.h"

static SiteHelperProjectValidation validation(SiteHelperProjectValidationCode code,
    DomainId subject, DomainId related)
{
    return (SiteHelperProjectValidation){code, subject, related};
}

/* Called only after every collection's metadata has been checked. Scanning
 * trades quadratic time for no allocation; stop at the second occurrence. */
static int duplicate_id(const SiteHelperProject *project, DomainId id)
{
    int seen = 0;
    for (size_t s = 0; s < project->storey_count; s++) {
        const BuildStructure *structure = &project->storeys[s].structure;
        if (project->storeys[s].id == id && seen++) { return 1; }
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
        for (size_t i = 0; i < structure->room_separator_count; i++) {
            if (structure->room_separators[i].id == id && seen++) {
                return 1;
            }
        }
    }
    return 0;
}

static SiteHelperProjectValidation validate_id(const SiteHelperProject *project,
    DomainId id, SiteHelperProjectValidationCode invalid_code, DomainId parent,
    DomainId *maximum)
{
    if (id == DOMAIN_ID_INVALID) {
        return validation(invalid_code, id, parent);
    }
    if (duplicate_id(project, id)) {
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
    if (project->storey_count > project->storey_capacity ||
        (project->storey_count != 0 && project->storeys == NULL)) {
        return validation(SITEHELPER_PROJECT_INVALID_STOREY_COLLECTION, 0, 0);
    }
    for (size_t s = 0; s < project->storey_count; s++) {
        const BuildStructure *structure = &project->storeys[s].structure;
        if (structure->room_count > structure->room_capacity ||
            (structure->room_count != 0 && structure->rooms == NULL)) {
            return validation(SITEHELPER_PROJECT_INVALID_ROOM_COLLECTION, 0, 0);
        }
        if (structure->wall_count > structure->wall_capacity ||
            (structure->wall_count != 0 && structure->walls == NULL)) {
            return validation(SITEHELPER_PROJECT_INVALID_WALL_COLLECTION, 0, 0);
        }
        if (structure->room_separator_count > structure->room_separator_capacity ||
            (structure->room_separator_count != 0 && structure->room_separators == NULL)) {
            return validation(SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_COLLECTION, 0, 0);
        }
        /* Establish safe metadata for all authoritative collections before any lookup or ID
         * scan can inspect a later collection. */
        for (size_t i = 0; i < structure->wall_count; i++) {
            const Wall *wall = &structure->walls[i];
            if (wall->definition.opening_count > wall->definition.opening_capacity ||
                (wall->definition.opening_count != 0 && wall->definition.openings == NULL)) {
                return validation(SITEHELPER_PROJECT_INVALID_OPENING_COLLECTION, wall->id, 0);
            }
        }
    }
    DomainId maximum = DOMAIN_ID_INVALID;
    for (size_t s = 0; s < project->storey_count; s++) {
        const BuildStructure *structure = &project->storeys[s].structure;
        SiteHelperProjectValidation storey_result = validate_id(project, project->storeys[s].id,
            SITEHELPER_PROJECT_INVALID_STOREY_ID, 0, &maximum);
        if (storey_result.code != SITEHELPER_PROJECT_VALID) { return storey_result; }
        for (size_t i = 0; i < structure->room_count; i++) {
            SiteHelperProjectValidation result = validate_id(project, structure->rooms[i].id,
                SITEHELPER_PROJECT_INVALID_ROOM_ID, 0, &maximum);
            if (result.code != SITEHELPER_PROJECT_VALID) {
                return result;
            }
        }
        for (size_t i = 0; i < structure->wall_count; i++) {
            const Wall *wall = &structure->walls[i];
            SiteHelperProjectValidation result = validate_id(project, wall->id,
                SITEHELPER_PROJECT_INVALID_WALL_ID, 0, &maximum);
            if (result.code != SITEHELPER_PROJECT_VALID) {
                return result;
            }
            for (size_t j = 0; j < wall->definition.opening_count; j++) {
                result = validate_id(project, wall->definition.openings[j].id,
                    SITEHELPER_PROJECT_INVALID_OPENING_ID, wall->id, &maximum);
                if (result.code != SITEHELPER_PROJECT_VALID) {
                    return result;
                }
            }
        }
        for (size_t i = 0; i < structure->room_separator_count; i++) {
            SiteHelperProjectValidation result = validate_id(project, structure->room_separators[i].id,
                SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_ID, 0, &maximum);
            if (result.code != SITEHELPER_PROJECT_VALID) {
                return result;
            }
        }
    }
    if (project->domain_ids.next == DOMAIN_ID_INVALID || project->domain_ids.next <= maximum) {
        return validation(SITEHELPER_PROJECT_INVALID_ID_GENERATOR, maximum, 0);
    }
    for (size_t s = 0; s < project->storey_count; s++) {
        const BuildStructure *structure = &project->storeys[s].structure;
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
        for (size_t i = 0; i < structure->room_separator_count; i++) {
            const RoomSeparator *separator = &structure->room_separators[i];
            if (!plan_segment_valid(separator->segment)) {
                return validation(SITEHELPER_PROJECT_INVALID_ROOM_SEPARATOR_GEOMETRY, separator->id, 0);
            }
        }
    }
    return validation(SITEHELPER_PROJECT_VALID, 0, 0);
}
