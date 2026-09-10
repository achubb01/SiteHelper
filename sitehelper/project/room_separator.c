#include "sitehelper_project.h"

DomainId sitehelper_project_add_room_separator(SiteHelperProject *project, PlanSegment segment)
{
    if (project == NULL || !plan_segment_valid(segment)) { return DOMAIN_ID_INVALID; }
    DomainIdGenerator ids = project->domain_ids;
    DomainId id = domain_id_generate(&ids);
    /* Preserve the established project watermark invariant at exhaustion. */
    if (id == DOMAIN_ID_INVALID || ids.next == DOMAIN_ID_INVALID) { return DOMAIN_ID_INVALID; }
    RoomSeparator separator = {.id = id, .segment = segment};
    if (!build_insert_room_separator(&project->structure, &separator,
            project->structure.room_separator_count)) {
        return DOMAIN_ID_INVALID;
    }
    project->domain_ids = ids;
    return id;
}

int sitehelper_project_remove_room_separator_by_id(SiteHelperProject *project, DomainId id)
{
    return project != NULL && build_remove_room_separator_by_id(&project->structure, id);
}

int sitehelper_project_set_room_separator_segment(SiteHelperProject *project,
    DomainId id, PlanSegment segment)
{
    if (project == NULL || !plan_segment_valid(segment)) { return 0; }
    RoomSeparator *separator = build_find_room_separator_by_id(&project->structure, id);
    if (separator == NULL) { return 0; }
    separator->segment = segment;
    return 1;
}
