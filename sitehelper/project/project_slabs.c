#include "sitehelper_project.h"
#include "slab.h"

static int append_slab(SiteHelperProject *project, Storey *storey, DomainId id,
    const PlanPosition *vertices, size_t count, int thickness_mm, int offset_mm)
{
    if (storey == NULL || sitehelper_project_contains_domain_id(project, id)) { return 0; }
    Slab candidate = {0};
    if (slab_build(id, vertices, count, thickness_mm, offset_mm, &candidate) != SLAB_SUCCESS) { return 0; }
    SlabCode code = slab_collection_append(&storey->slabs, &candidate);
    slab_destroy(&candidate);
    return code == SLAB_SUCCESS;
}

DomainId sitehelper_project_add_slab(SiteHelperProject *project, DomainId storey_id,
    const PlanPosition *vertices, size_t vertex_count, int thickness_mm, int top_level_offset_mm)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID ||
        !append_slab(project, storey, id, vertices, vertex_count, thickness_mm, top_level_offset_mm)) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_insert_slab(SiteHelperProject *project, DomainId storey_id, const Slab *slab)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL || slab == NULL || sitehelper_project_contains_domain_id(project,slab->id)) { return 0; }
    Slab candidate = {0};
    if (slab_clone(slab,&candidate) != SLAB_SUCCESS) { return 0; }
    SlabCode code = slab_collection_append(&storey->slabs,&candidate);
    slab_destroy(&candidate);
    return code == SLAB_SUCCESS;
}

Slab *sitehelper_project_find_slab_by_id(SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        Slab *slab = slab_collection_find_by_id(&project->storeys[i].slabs, id);
        if (slab != NULL) { return slab; }
    }
    return NULL;
}

const Slab *sitehelper_project_find_slab_by_id_const(const SiteHelperProject *project, DomainId id)
{
    if (project == NULL || id == DOMAIN_ID_INVALID) { return NULL; }
    for (size_t i = 0; i < project->storey_count; i++) {
        const Slab *slab = slab_collection_find_by_id_const(&project->storeys[i].slabs, id);
        if (slab != NULL) { return slab; }
    }
    return NULL;
}

int sitehelper_project_remove_slab_by_id(SiteHelperProject *project, DomainId id)
{
    Storey *storey = sitehelper_project_find_owning_storey(project, id);
    return storey != NULL && slab_collection_remove_by_id(&storey->slabs, id);
}
