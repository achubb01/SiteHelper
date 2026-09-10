#include "sitehelper_project.h"

DomainId sitehelper_project_add_room_separator(SiteHelperProject *project, DomainId storey_id, PlanSegment segment)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    if (storey == NULL || !plan_segment_valid(segment)) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    /* Preserve the established project watermark invariant at exhaustion. */
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID) { return DOMAIN_ID_INVALID; }
    RoomSeparator separator = {.id = id, .segment = segment};
    if (!sitehelper_project_insert_room_separator(project, storey_id, &separator,
            storey->structure.room_separator_count)) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_remove_room_separator_by_id(SiteHelperProject *project, DomainId id)
{
    Storey *storey = sitehelper_project_find_owning_storey(project, id);
    return storey != NULL && build_remove_room_separator_by_id(&storey->structure, id);
}

int sitehelper_project_set_room_separator_segment(SiteHelperProject *project,
    DomainId id, PlanSegment segment)
{
    if (project == NULL || !plan_segment_valid(segment)) { return 0; }
    RoomSeparator *separator = sitehelper_project_find_room_separator_by_id(project, id);
    if (separator == NULL) { return 0; }
    separator->segment = segment;
    return 1;
}

int sitehelper_project_insert_room_separator(SiteHelperProject *project, DomainId storey_id,
    const RoomSeparator *separator, size_t index)
{
    Storey *storey = sitehelper_project_find_storey_by_id(project, storey_id);
    return storey != NULL && separator != NULL &&
        !sitehelper_project_contains_domain_id(project, separator->id) &&
        build_insert_room_separator(&storey->structure, separator, index);
}
