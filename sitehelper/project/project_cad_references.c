#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sitehelper_project.h"

static int collection_valid(const SiteHelperProject *project)
{
    return project != NULL &&
        project->cad_plan_reference_count <= project->cad_plan_reference_capacity &&
        (project->cad_plan_reference_capacity == 0 || project->cad_plan_references != NULL);
}

static size_t find_index(const SiteHelperProject *project, DomainId storey_id)
{
    for (size_t i = 0; i < project->cad_plan_reference_count; i++) {
        if (project->cad_plan_references[i].storey_id == storey_id) {
            return i;
        }
    }
    return SIZE_MAX;
}

const CadPlanReference *sitehelper_project_find_cad_plan_reference(
    const SiteHelperProject *project, DomainId storey_id)
{
    if (!collection_valid(project) || storey_id == DOMAIN_ID_INVALID) {
        return NULL;
    }
    size_t index = find_index(project, storey_id);
    return index == SIZE_MAX ? NULL : &project->cad_plan_references[index].reference;
}

SiteHelperCadReferenceApplyCode sitehelper_project_adopt_cad_plan_reference(
    SiteHelperProject *project, DomainId storey_id, CadPlanReference *reference)
{
    if (project == NULL || reference == NULL || storey_id == DOMAIN_ID_INVALID) {
        return SITEHELPER_CAD_REFERENCE_APPLY_INVALID_ARGUMENT;
    }
    if (!collection_valid(project)) {
        return SITEHELPER_CAD_REFERENCE_APPLY_INVALID_COLLECTION;
    }
    if (sitehelper_project_find_storey_by_id_const(project, storey_id) == NULL) {
        return SITEHELPER_CAD_REFERENCE_APPLY_STOREY_NOT_FOUND;
    }
    if (reference->status != CAD_PLAN_REFERENCE_READY) {
        return SITEHELPER_CAD_REFERENCE_APPLY_REFERENCE_NOT_READY;
    }

    size_t index = find_index(project, storey_id);
    if (index != SIZE_MAX) {
        CadPlanReference previous = project->cad_plan_references[index].reference;
        project->cad_plan_references[index].reference = *reference;
        *reference = (CadPlanReference){0};
        cad_plan_reference_destroy(&previous);
        return SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS;
    }

    if (project->cad_plan_reference_count == project->cad_plan_reference_capacity) {
        size_t maximum = SIZE_MAX / sizeof *project->cad_plan_references;
        size_t capacity = project->cad_plan_reference_capacity;
        if (capacity >= maximum) {
            return SITEHELPER_CAD_REFERENCE_APPLY_ALLOCATION_FAILED;
        }
        size_t grown = capacity == 0 ? 1 :
            capacity > maximum / 2 ? maximum : capacity * 2;
        SiteHelperCadPlanReferenceAttachment *storage = realloc(
            project->cad_plan_references, grown * sizeof *storage);
        if (storage == NULL) {
            return SITEHELPER_CAD_REFERENCE_APPLY_ALLOCATION_FAILED;
        }
        project->cad_plan_references = storage;
        project->cad_plan_reference_capacity = grown;
    }

    project->cad_plan_references[project->cad_plan_reference_count++] =
        (SiteHelperCadPlanReferenceAttachment){
            .storey_id = storey_id,
            .reference = *reference
        };
    *reference = (CadPlanReference){0};
    return SITEHELPER_CAD_REFERENCE_APPLY_SUCCESS;
}

int sitehelper_project_clear_cad_plan_reference(
    SiteHelperProject *project, DomainId storey_id)
{
    if (!collection_valid(project) || storey_id == DOMAIN_ID_INVALID) {
        return 0;
    }
    size_t index = find_index(project, storey_id);
    if (index == SIZE_MAX) {
        return 0;
    }

    cad_plan_reference_destroy(&project->cad_plan_references[index].reference);
    size_t trailing = project->cad_plan_reference_count - index - 1;
    if (trailing != 0) {
        memmove(&project->cad_plan_references[index],
            &project->cad_plan_references[index + 1],
            trailing * sizeof *project->cad_plan_references);
    }
    project->cad_plan_reference_count--;
    return 1;
}
