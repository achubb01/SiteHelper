#include "roof_source_edit_command.h"

#include <stdlib.h>
#include <string.h>

#include "roof.h"

static RoofPortionSpec owned_spec(const RoofSourceEditOwnedPortion *portion)
{
    return (RoofPortionSpec){
        portion->support_vertices,
        portion->support_vertex_count,
        portion->generation,
        portion->slope_ppm,
        portion->reference_z_mm,
        portion->direction,
        portion->single_slope_reference
    };
}

static int owned_portion_copy(const RoofPortionSpec *spec, RoofSourceEditOwnedPortion *output)
{
    if (spec == NULL || output == NULL || spec->support_vertices == NULL ||
        spec->support_vertex_count == 0 ||
        spec->support_vertex_count > SIZE_MAX / sizeof *spec->support_vertices) { return 0; }
    PlanPosition *vertices = malloc(spec->support_vertex_count * sizeof *vertices);
    if (vertices == NULL) { return 0; }
    memcpy(vertices, spec->support_vertices, spec->support_vertex_count * sizeof *vertices);
    *output = (RoofSourceEditOwnedPortion){
        .support_vertices = vertices,
        .support_vertex_count = spec->support_vertex_count,
        .generation = spec->generation,
        .slope_ppm = spec->slope_ppm,
        .reference_z_mm = spec->reference_z_mm,
        .direction = spec->direction,
        .single_slope_reference = spec->single_slope_reference
    };
    return 1;
}

static int valid_roof_id(DomainId roof_id)
{
    return roof_id != DOMAIN_ID_INVALID;
}

int roof_source_edit_command_create_add_portion(DomainId roof_id, DomainId existing_portion_id,
    RoofCompositionKind kind, const RoofPortionSpec *spec, RoofSourceEditCommand *output)
{
    if (output == NULL || !valid_roof_id(roof_id) || existing_portion_id == DOMAIN_ID_INVALID ||
        (kind != ROOF_COMPOSITION_INTERSECTS && kind != ROOF_COMPOSITION_ABUTS)) { return 0; }
    RoofSourceEditCommand candidate = {.kind = ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED,
        .roof_id = roof_id};
    candidate.data.add_portion.existing_portion_id = existing_portion_id;
    candidate.data.add_portion.composition_kind = kind;
    if (!owned_portion_copy(spec, &candidate.data.add_portion.portion)) { return 0; }
    roof_source_edit_command_destroy(output);
    *output = candidate;
    return 1;
}

int roof_source_edit_command_create_remove_portion(DomainId roof_id, DomainId portion_id,
    RoofSourceEditCommand *command)
{
    if (command == NULL || !valid_roof_id(roof_id) || portion_id == DOMAIN_ID_INVALID) { return 0; }
    roof_source_edit_command_destroy(command);
    *command = (RoofSourceEditCommand){.kind = ROOF_SOURCE_EDIT_REMOVE_PORTION,
        .roof_id = roof_id, .data.remove_portion = {portion_id}};
    return 1;
}

int roof_source_edit_command_create_set_portion(DomainId roof_id, DomainId portion_id,
    const RoofPortionSpec *spec, RoofSourceEditCommand *output)
{
    if (output == NULL || !valid_roof_id(roof_id) || portion_id == DOMAIN_ID_INVALID) { return 0; }
    RoofSourceEditCommand candidate = {.kind = ROOF_SOURCE_EDIT_SET_PORTION, .roof_id = roof_id};
    candidate.data.set_portion.portion_id = portion_id;
    if (!owned_portion_copy(spec, &candidate.data.set_portion.portion)) { return 0; }
    roof_source_edit_command_destroy(output);
    *output = candidate;
    return 1;
}

int roof_source_edit_command_create_set_composition(DomainId roof_id, RoofComposition composition,
    RoofSourceEditCommand *command)
{
    if (command == NULL || !valid_roof_id(roof_id) ||
        composition.first_portion_id == DOMAIN_ID_INVALID ||
        composition.second_portion_id == DOMAIN_ID_INVALID ||
        composition.first_portion_id == composition.second_portion_id ||
        (composition.kind != ROOF_COMPOSITION_INTERSECTS &&
         composition.kind != ROOF_COMPOSITION_ABUTS)) { return 0; }
    roof_source_edit_command_destroy(command);
    *command = (RoofSourceEditCommand){.kind = ROOF_SOURCE_EDIT_SET_COMPOSITION,
        .roof_id = roof_id, .data.composition = composition};
    return 1;
}

int roof_source_edit_command_create_remove_composition(DomainId roof_id, DomainId first_portion_id,
    DomainId second_portion_id, RoofSourceEditCommand *command)
{
    if (command == NULL || !valid_roof_id(roof_id) || first_portion_id == DOMAIN_ID_INVALID ||
        second_portion_id == DOMAIN_ID_INVALID || first_portion_id == second_portion_id) { return 0; }
    roof_source_edit_command_destroy(command);
    *command = (RoofSourceEditCommand){.kind = ROOF_SOURCE_EDIT_REMOVE_COMPOSITION,
        .roof_id = roof_id,
        .data.remove_composition = {first_portion_id, second_portion_id}};
    return 1;
}

int roof_source_edit_command_create_set_termination(DomainId roof_id, RoofTermination termination,
    RoofSourceEditCommand *command)
{
    if (command == NULL || !valid_roof_id(roof_id) || termination.portion_id == DOMAIN_ID_INVALID ||
        termination.termination_offset_mm <= 0 ||
        (termination.end != ROOF_END_NEGATIVE_AXIS && termination.end != ROOF_END_POSITIVE_AXIS)) {
        return 0;
    }
    roof_source_edit_command_destroy(command);
    *command = (RoofSourceEditCommand){.kind = ROOF_SOURCE_EDIT_SET_TERMINATION,
        .roof_id = roof_id, .data.termination = termination};
    return 1;
}

int roof_source_edit_command_create_remove_termination(DomainId roof_id, DomainId portion_id,
    RoofEnd end, RoofSourceEditCommand *command)
{
    if (command == NULL || !valid_roof_id(roof_id) || portion_id == DOMAIN_ID_INVALID ||
        (end != ROOF_END_NEGATIVE_AXIS && end != ROOF_END_POSITIVE_AXIS)) { return 0; }
    roof_source_edit_command_destroy(command);
    *command = (RoofSourceEditCommand){.kind = ROOF_SOURCE_EDIT_REMOVE_TERMINATION,
        .roof_id = roof_id, .data.remove_termination = {portion_id, end}};
    return 1;
}

int roof_source_edit_command_clone(const RoofSourceEditCommand *source,
    RoofSourceEditCommand *output)
{
    if (source == NULL || output == NULL || source == output ||
        source->kind < ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED ||
        source->kind >= ROOF_SOURCE_EDIT_KIND_COUNT) { return 0; }
    RoofSourceEditCommand candidate = *source;
    if (source->kind == ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED) {
        candidate.data.add_portion.portion = (RoofSourceEditOwnedPortion){0};
        RoofPortionSpec spec = owned_spec(&source->data.add_portion.portion);
        if (!owned_portion_copy(&spec, &candidate.data.add_portion.portion)) { return 0; }
    } else if (source->kind == ROOF_SOURCE_EDIT_SET_PORTION) {
        candidate.data.set_portion.portion = (RoofSourceEditOwnedPortion){0};
        RoofPortionSpec spec = owned_spec(&source->data.set_portion.portion);
        if (!owned_portion_copy(&spec, &candidate.data.set_portion.portion)) { return 0; }
    }
    roof_source_edit_command_destroy(output);
    *output = candidate;
    return 1;
}

void roof_source_edit_command_destroy(RoofSourceEditCommand *command)
{
    if (command == NULL) { return; }
    if (command->kind == ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED) {
        free(command->data.add_portion.portion.support_vertices);
    } else if (command->kind == ROOF_SOURCE_EDIT_SET_PORTION) {
        free(command->data.set_portion.portion.support_vertices);
    }
    *command = (RoofSourceEditCommand){0};
}

static int apply_to_definition(RoofDefinition *definition, const RoofSourceEditCommand *command,
    DomainId added_portion_id)
{
    RoofCode code = ROOF_INVALID_ARGUMENT;
    switch (command->kind) {
        case ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED: {
            if (added_portion_id == DOMAIN_ID_INVALID) { return 0; }
            RoofPortionSpec spec = owned_spec(&command->data.add_portion.portion);
            code = roof_definition_append_portion(definition, added_portion_id, &spec);
            if (code != ROOF_SUCCESS) { return 0; }
            code = roof_definition_set_composition(definition,
                (RoofComposition){command->data.add_portion.existing_portion_id,
                    added_portion_id, command->data.add_portion.composition_kind});
            return code == ROOF_SUCCESS;
        }
        case ROOF_SOURCE_EDIT_REMOVE_PORTION:
            return roof_definition_remove_portion(definition,
                command->data.remove_portion.portion_id) == ROOF_SUCCESS;
        case ROOF_SOURCE_EDIT_SET_PORTION: {
            RoofPortionSpec spec = owned_spec(&command->data.set_portion.portion);
            return roof_definition_replace_portion(definition,
                command->data.set_portion.portion_id, &spec) == ROOF_SUCCESS;
        }
        case ROOF_SOURCE_EDIT_SET_COMPOSITION:
            return roof_definition_set_composition(definition,
                command->data.composition) == ROOF_SUCCESS;
        case ROOF_SOURCE_EDIT_REMOVE_COMPOSITION:
            return roof_definition_remove_composition(definition,
                command->data.remove_composition.first_portion_id,
                command->data.remove_composition.second_portion_id) == ROOF_SUCCESS;
        case ROOF_SOURCE_EDIT_SET_TERMINATION:
            return roof_definition_set_termination(definition,
                command->data.termination) == ROOF_SUCCESS;
        case ROOF_SOURCE_EDIT_REMOVE_TERMINATION:
            return roof_definition_remove_termination(definition,
                command->data.remove_termination.portion_id,
                command->data.remove_termination.end) == ROOF_SUCCESS;
        case ROOF_SOURCE_EDIT_KIND_COUNT:
        default:
            return 0;
    }
}

int roof_source_edit_command_build_expected(const Roof *before,
    const RoofSourceEditCommand *command, DomainId affected_portion_id, Roof *expected)
{
    if (before == NULL || command == NULL || expected == NULL || before->id != command->roof_id) {
        return 0;
    }
    Roof candidate = {0};
    if (roof_clone(before, &candidate) != ROOF_SUCCESS ||
        !apply_to_definition(&candidate.definition, command, affected_portion_id) ||
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
    roof_destroy(expected);
    *expected = candidate;
    return 1;
}

int roof_source_edit_command_execute(SiteHelperProject *project,
    const RoofSourceEditCommand *command, DomainId *affected_portion_id)
{
    if (affected_portion_id == NULL) { return 0; }
    *affected_portion_id = DOMAIN_ID_INVALID;
    if (project == NULL || command == NULL) { return 0; }
    switch (command->kind) {
        case ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED: {
            RoofPortionSpec spec = owned_spec(&command->data.add_portion.portion);
            DomainId id = sitehelper_project_add_roof_portion_composed(project, command->roof_id,
                &spec, command->data.add_portion.existing_portion_id,
                command->data.add_portion.composition_kind);
            if (id == DOMAIN_ID_INVALID) { return 0; }
            *affected_portion_id = id;
            return 1;
        }
        case ROOF_SOURCE_EDIT_REMOVE_PORTION:
            if (!sitehelper_project_remove_roof_portion(project, command->roof_id,
                    command->data.remove_portion.portion_id)) { return 0; }
            *affected_portion_id = command->data.remove_portion.portion_id;
            return 1;
        case ROOF_SOURCE_EDIT_SET_PORTION: {
            RoofPortionSpec spec = owned_spec(&command->data.set_portion.portion);
            if (!sitehelper_project_set_roof_portion(project, command->roof_id,
                    command->data.set_portion.portion_id, &spec)) { return 0; }
            *affected_portion_id = command->data.set_portion.portion_id;
            return 1;
        }
        case ROOF_SOURCE_EDIT_SET_COMPOSITION:
            return sitehelper_project_set_roof_composition(project, command->roof_id,
                command->data.composition);
        case ROOF_SOURCE_EDIT_REMOVE_COMPOSITION:
            return sitehelper_project_remove_roof_composition(project, command->roof_id,
                command->data.remove_composition.first_portion_id,
                command->data.remove_composition.second_portion_id);
        case ROOF_SOURCE_EDIT_SET_TERMINATION:
            if (!sitehelper_project_set_roof_termination(project, command->roof_id,
                    command->data.termination)) { return 0; }
            *affected_portion_id = command->data.termination.portion_id;
            return 1;
        case ROOF_SOURCE_EDIT_REMOVE_TERMINATION:
            if (!sitehelper_project_remove_roof_termination(project, command->roof_id,
                    command->data.remove_termination.portion_id,
                    command->data.remove_termination.end)) { return 0; }
            *affected_portion_id = command->data.remove_termination.portion_id;
            return 1;
        case ROOF_SOURCE_EDIT_KIND_COUNT:
        default:
            return 0;
    }
}

int roof_source_edit_command_redo(SiteHelperProject *project,
    const RoofSourceEditCommand *command, DomainId affected_portion_id)
{
    if (project == NULL || command == NULL) { return 0; }
    if (command->kind != ROOF_SOURCE_EDIT_ADD_PORTION_COMPOSED) {
        DomainId ignored;
        return roof_source_edit_command_execute(project, command, &ignored);
    }
    RoofPortionSpec spec = owned_spec(&command->data.add_portion.portion);
    return sitehelper_project_restore_roof_portion_composed(project, command->roof_id,
        affected_portion_id, &spec, command->data.add_portion.existing_portion_id,
        command->data.add_portion.composition_kind);
}
