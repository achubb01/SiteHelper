#include "sitehelper_project.h"
#include "wall.h"
#include "slab.h"
#include "project_settings_internal.h"

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
        const SlabCollection *slabs = &project->storeys[s].slabs;
        for (size_t i = 0; i < slabs->count; i++) {
            if (slabs->items[i].id == id && seen++) { return 1; }
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
    /* Slab metadata is established before any global identity traversal. */
    for (size_t s = 0; s < project->storey_count; s++) {
        const Storey *storey = &project->storeys[s];
        const SlabCollection *slabs = &storey->slabs;
        if (slabs->count > slabs->capacity ||
            (slabs->capacity == 0 && slabs->items != NULL) ||
            (slabs->capacity != 0 && slabs->items == NULL)) {
            return validation(SITEHELPER_PROJECT_INVALID_SLAB_COLLECTION, storey->id, storey->id);
        }
        if (slabs->capacity > SIZE_MAX / sizeof *slabs->items) {
            return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW, storey->id, storey->id);
        }
        for (size_t i = 0; i < slabs->count; i++) {
            const Slab *slab = &slabs->items[i];
            const SlabOutline *o = &slab->definition.outline;
            if (o->vertex_count > o->vertex_capacity ||
                (o->vertex_capacity == 0 && o->vertices != NULL) ||
                (o->vertex_capacity != 0 && o->vertices == NULL)) {
                return validation(SITEHELPER_PROJECT_INVALID_SLAB_OUTLINE_COLLECTION, slab->id, storey->id);
            }
            if (o->vertex_capacity > SIZE_MAX / sizeof *o->vertices) {
                return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW, slab->id, storey->id);
            }
            const SlabPenetrationCollection *p = &slab->definition.penetrations;
            if (p->count > p->capacity || (p->capacity == 0 && p->items != NULL) ||
                (p->capacity != 0 && p->items == NULL)) {
                return validation(SITEHELPER_PROJECT_INVALID_SLAB_PENETRATION_COLLECTION,slab->id,storey->id);
            }
            if (p->capacity > SIZE_MAX / sizeof *p->items) {
                return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW,slab->id,storey->id);
            }
            for (size_t j = 0; j < p->count; j++) {
                const SlabOutline *hole = &p->items[j].outline;
                if (hole->vertex_count > hole->vertex_capacity ||
                    (hole->vertex_capacity == 0 && hole->vertices != NULL) ||
                    (hole->vertex_capacity != 0 && hole->vertices == NULL)) {
                    return validation(SITEHELPER_PROJECT_INVALID_SLAB_PENETRATION_OUTLINE_COLLECTION,slab->id,storey->id);
                }
                if (hole->vertex_capacity > SIZE_MAX / sizeof *hole->vertices) {
                    return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW,slab->id,storey->id);
                }
            }
            const SlabRegionCollection *r = &slab->definition.regions;
            if (r->count > r->capacity || (r->capacity == 0 && r->items != NULL) ||
                (r->capacity != 0 && r->items == NULL)) {
                return validation(SITEHELPER_PROJECT_INVALID_SLAB_REGION_COLLECTION,slab->id,storey->id);
            }
            if (r->capacity > SIZE_MAX / sizeof *r->items) {
                return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW,slab->id,storey->id);
            }
            for (size_t j = 0; j < r->count; j++) {
                const SlabOutline *outline = &r->items[j].outline;
                if (outline->vertex_count > outline->vertex_capacity ||
                    (outline->vertex_capacity == 0 && outline->vertices != NULL) ||
                    (outline->vertex_capacity != 0 && outline->vertices == NULL)) {
                    return validation(SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE_COLLECTION,slab->id,storey->id);
                }
                if (outline->vertex_capacity > SIZE_MAX / sizeof *outline->vertices) {
                    return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW,slab->id,storey->id);
                }
            }
            const SlabEdgeRebateCollection *rebates = &slab->definition.edge_rebates;
            if (rebates->count > rebates->capacity ||
                (rebates->capacity == 0 && rebates->items != NULL) ||
                (rebates->capacity != 0 && rebates->items == NULL)) {
                return validation(SITEHELPER_PROJECT_INVALID_SLAB_EDGE_REBATE_COLLECTION,
                    slab->id,storey->id);
            }
            if (rebates->capacity > SIZE_MAX / sizeof *rebates->items) {
                return validation(SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW,slab->id,storey->id);
            }
        }
    }
    for (size_t s = 0; s < project->storey_count; s++) {
        if (!storey_build_settings_valid(&project->storeys[s].settings)) {
            return validation(SITEHELPER_PROJECT_INVALID_STOREY_SETTINGS, project->storeys[s].id, 0);
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
        const SlabCollection *slabs = &project->storeys[s].slabs;
        for (size_t i = 0; i < slabs->count; i++) {
            SiteHelperProjectValidation result = validate_id(project, slabs->items[i].id,
                SITEHELPER_PROJECT_INVALID_SLAB_ID, project->storeys[s].id, &maximum);
            if (result.code != SITEHELPER_PROJECT_VALID) { return result; }
        }
    }
    if (project->domain_ids.next == DOMAIN_ID_INVALID || project->domain_ids.next <= maximum) {
        return validation(SITEHELPER_PROJECT_INVALID_ID_GENERATOR, maximum, 0);
    }
    for (size_t s = 0; s < project->storey_count; s++) {
        const BuildStructure *structure = &project->storeys[s].structure;
        BuildSettings resolved;
        project_resolve_build_settings(&project->settings, &project->storeys[s].settings, &resolved);
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
                WallOpeningValidation result = wall_validate_opening(&prefix, &resolved, &proposal);
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
    for (size_t s = 0; s < project->storey_count; s++) {
        const Storey *storey = &project->storeys[s];
        for (size_t i = 0; i < storey->slabs.count; i++) {
            const Slab *slab = &storey->slabs.items[i];
            SlabCode code = slab_definition_validate(&slab->definition);
            if (code != SLAB_SUCCESS) {
                SiteHelperProjectValidationCode project_code = code == SLAB_INVALID_THICKNESS ?
                    SITEHELPER_PROJECT_INVALID_SLAB_THICKNESS : code == SLAB_NUMERIC_OVERFLOW ?
                    SITEHELPER_PROJECT_SLAB_NUMERIC_OVERFLOW : SITEHELPER_PROJECT_INVALID_SLAB_GEOMETRY;
                if (code == SLAB_INVALID_PENETRATION_OUTLINE) { project_code = SITEHELPER_PROJECT_INVALID_SLAB_PENETRATION_OUTLINE; }
                if (code == SLAB_PENETRATION_OUTSIDE) { project_code = SITEHELPER_PROJECT_SLAB_PENETRATION_OUTSIDE; }
                if (code == SLAB_PENETRATION_OVERLAP) { project_code = SITEHELPER_PROJECT_SLAB_PENETRATION_OVERLAP; }
                if (code == SLAB_INVALID_REGION_OUTLINE) { project_code = SITEHELPER_PROJECT_INVALID_SLAB_REGION_OUTLINE; }
                if (code == SLAB_INVALID_REGION_THICKNESS) { project_code = SITEHELPER_PROJECT_INVALID_SLAB_REGION_THICKNESS; }
                if (code == SLAB_REGION_OUTSIDE) { project_code = SITEHELPER_PROJECT_SLAB_REGION_OUTSIDE; }
                if (code == SLAB_REGION_OVERLAP) { project_code = SITEHELPER_PROJECT_SLAB_REGION_OVERLAP; }
                if (code == SLAB_REGION_PENETRATION_INTERSECTION) { project_code = SITEHELPER_PROJECT_SLAB_REGION_PENETRATION_INTERSECTION; }
                if (code == SLAB_EDGE_REBATE_INVALID_EDGE) { project_code = SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_EDGE; }
                if (code == SLAB_EDGE_REBATE_INVALID_INTERVAL) { project_code = SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_INTERVAL; }
                if (code == SLAB_EDGE_REBATE_INVALID_DIMENSIONS) { project_code = SITEHELPER_PROJECT_SLAB_EDGE_REBATE_INVALID_DIMENSIONS; }
                if (code == SLAB_EDGE_REBATE_OVERLAP) { project_code = SITEHELPER_PROJECT_SLAB_EDGE_REBATE_OVERLAP; }
                return validation(project_code, slab->id, storey->id);
            }
        }
    }
    return validation(SITEHELPER_PROJECT_VALID, 0, 0);
}
