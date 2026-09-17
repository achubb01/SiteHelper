#include <stdlib.h>
#include <string.h>

#include "create_roof_command.h"
#include "roof.h"

static RoofPortionSpec command_spec(const CreateRoofCommand *command)
{
    return (RoofPortionSpec){
        .support_vertices = command->support_vertices,
        .support_vertex_count = command->support_vertex_count,
        .generation = command->generation,
        .slope_ppm = command->slope_ppm,
        .reference_z_mm = command->reference_z_mm,
        .direction = command->direction,
        .single_slope_reference = command->single_slope_reference
    };
}

int create_roof_command_create(DomainId storey_id, const RoofPortionSpec *spec,
    CreateRoofCommand *output)
{
    if (output == NULL || spec == NULL || storey_id == DOMAIN_ID_INVALID ||
        spec->support_vertices == NULL || spec->support_vertex_count == 0 ||
        spec->support_vertex_count > SIZE_MAX / sizeof *spec->support_vertices) {
        return 0;
    }
    PlanPosition *vertices = malloc(spec->support_vertex_count * sizeof *vertices);
    if (vertices == NULL) { return 0; }
    memcpy(vertices, spec->support_vertices, spec->support_vertex_count * sizeof *vertices);

    /* Validate both the authoritative source and its current deterministic
     * regeneration contract without consuming project identities. */
    Roof candidate = {.id = 1};
    RoofPortionSpec copied = *spec;
    copied.support_vertices = vertices;
    RoofPrototypeGeometry geometry = {0};
    if (roof_definition_append_portion(&candidate.definition, 2, &copied) != ROOF_SUCCESS ||
        roof_validate(&candidate) != ROOF_SUCCESS ||
        roof_build_derived_geometry(&candidate, &geometry) != ROOF_SUCCESS) {
        roof_prototype_geometry_destroy(&geometry);
        roof_destroy(&candidate);
        free(vertices);
        return 0;
    }
    roof_prototype_geometry_destroy(&geometry);
    roof_destroy(&candidate);

    CreateRoofCommand command = {
        .storey_id = storey_id,
        .support_vertices = vertices,
        .support_vertex_count = spec->support_vertex_count,
        .generation = spec->generation,
        .slope_ppm = spec->slope_ppm,
        .reference_z_mm = spec->reference_z_mm,
        .direction = spec->direction,
        .single_slope_reference = spec->single_slope_reference
    };
    create_roof_command_destroy(output);
    *output = command;
    return 1;
}

int create_roof_command_clone(const CreateRoofCommand *source, CreateRoofCommand *output)
{
    if (source == NULL) { return 0; }
    RoofPortionSpec spec = command_spec(source);
    return create_roof_command_create(source->storey_id, &spec, output);
}

void create_roof_command_destroy(CreateRoofCommand *command)
{
    if (command == NULL) { return; }
    free(command->support_vertices);
    *command = (CreateRoofCommand){0};
}

int create_roof_command_execute(SiteHelperProject *project, const CreateRoofCommand *command,
    DomainId *roof_id, DomainId *portion_id)
{
    if (roof_id == NULL || portion_id == NULL) { return 0; }
    *roof_id = DOMAIN_ID_INVALID;
    *portion_id = DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL) { return 0; }
    RoofPortionSpec spec = command_spec(command);
    *roof_id = sitehelper_project_add_roof(project, command->storey_id, &spec, portion_id);
    if (*roof_id == DOMAIN_ID_INVALID) {
        *portion_id = DOMAIN_ID_INVALID;
        return 0;
    }
    return 1;
}

int create_roof_command_undo(SiteHelperProject *project, const CreateRoofCommand *command,
    DomainId roof_id, DomainId portion_id)
{
    if (project == NULL || command == NULL || roof_id == DOMAIN_ID_INVALID ||
        portion_id == DOMAIN_ID_INVALID) { return 0; }
    const Storey *owner = sitehelper_project_find_owning_storey_const(project, roof_id);
    const Roof *roof = sitehelper_project_find_roof_by_id_const(project, roof_id);
    return owner != NULL && owner->id == command->storey_id && roof != NULL &&
        roof->definition.portion_count == 1 && roof->definition.portions[0].id == portion_id &&
        sitehelper_project_remove_roof_by_id(project, roof_id);
}

int create_roof_command_redo(SiteHelperProject *project, const CreateRoofCommand *command,
    DomainId roof_id, DomainId portion_id)
{
    if (project == NULL || command == NULL || roof_id == DOMAIN_ID_INVALID ||
        portion_id == DOMAIN_ID_INVALID || project->domain_ids.next == DOMAIN_ID_INVALID ||
        roof_id >= project->domain_ids.next || portion_id >= project->domain_ids.next ||
        sitehelper_project_contains_domain_id(project, roof_id) ||
        sitehelper_project_contains_domain_id(project, portion_id)) { return 0; }

    Roof candidate = {.id = roof_id};
    RoofPortionSpec spec = command_spec(command);
    if (roof_definition_append_portion(&candidate.definition, portion_id, &spec) != ROOF_SUCCESS ||
        roof_validate(&candidate) != ROOF_SUCCESS) {
        roof_destroy(&candidate);
        return 0;
    }
    RoofPrototypeGeometry geometry = {0};
    if (roof_build_derived_geometry(&candidate, &geometry) != ROOF_SUCCESS) {
        roof_prototype_geometry_destroy(&geometry);
        roof_destroy(&candidate);
        return 0;
    }
    roof_prototype_geometry_destroy(&geometry);
    const Storey *storey = sitehelper_project_find_storey_by_id_const(project, command->storey_id);
    int ok = storey != NULL && sitehelper_project_insert_roof_at(project, command->storey_id,
        &candidate, storey->roofs.count);
    roof_destroy(&candidate);
    return ok;
}
