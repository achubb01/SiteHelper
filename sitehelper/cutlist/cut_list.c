#include <stdlib.h>
#include "cut_list.h"
#include "framing_takeoff.h"

typedef struct {
    CutList list;
    size_t capacity;
} CutListBuilder;

static CutListResult result(CutListCode code, DomainId wall_id)
{
    return (CutListResult){code, wall_id};
}

static CutListCode from_takeoff(FramingTakeoffCode code)
{
    switch (code) {
    case FRAMING_TAKEOFF_SUCCESS: return CUT_LIST_SUCCESS;
    case FRAMING_TAKEOFF_INVALID_ARGUMENT: return CUT_LIST_INVALID_ARGUMENT;
    case FRAMING_TAKEOFF_INVALID_SOURCE: return CUT_LIST_INVALID_SOURCE;
    case FRAMING_TAKEOFF_INVALID_FRAMING: return CUT_LIST_INVALID_FRAMING;
    case FRAMING_TAKEOFF_ALLOCATION_FAILED: return CUT_LIST_ALLOCATION_FAILED;
    case FRAMING_TAKEOFF_NUMERIC_OVERFLOW: return CUT_LIST_NUMERIC_OVERFLOW;
    }
    return CUT_LIST_INVALID_SOURCE;
}

static int collection_valid(const void *data, size_t count, size_t capacity, size_t size)
{
    return count <= capacity && capacity <= SIZE_MAX / size &&
        ((capacity == 0 && data == NULL) || (capacity != 0 && data != NULL));
}

void cut_list_destroy(CutList *list)
{
    if (list == NULL) { return; }
    free(list->members);
    *list = (CutList){0};
}

static CutListCode reserve(CutListBuilder *builder, size_t additional)
{
    size_t maximum = SIZE_MAX / sizeof *builder->list.members;
    if (additional > maximum - builder->list.member_count) { return CUT_LIST_NUMERIC_OVERFLOW; }
    size_t needed = builder->list.member_count + additional;
    if (needed <= builder->capacity) { return CUT_LIST_SUCCESS; }
    size_t grown = builder->capacity > maximum / 2 ? maximum : builder->capacity * 2;
    if (grown < needed) { grown = needed; }
    RequiredMember *members = realloc(builder->list.members, grown * sizeof *members);
    if (members == NULL) { return CUT_LIST_ALLOCATION_FAILED; }
    builder->list.members = members;
    builder->capacity = grown;
    return CUT_LIST_SUCCESS;
}

static CutListResult append_wall(CutListBuilder *builder, const Wall *wall)
{
    if (wall->id == DOMAIN_ID_INVALID) { return result(CUT_LIST_INVALID_SOURCE, 0); }
    FramingTakeoff takeoff = {0};
    FramingTakeoffResult status = framing_takeoff_build_wall(wall, &takeoff);
    CutListCode code = from_takeoff(status.code);
    if (code == CUT_LIST_SUCCESS) {
        code = reserve(builder, takeoff.item_count);
        if (code == CUT_LIST_SUCCESS) {
            for (size_t i = 0; i < takeoff.item_count; i++) {
                const FramingTakeoffItem *item = &takeoff.items[i];
                builder->list.members[builder->list.member_count++] = (RequiredMember){
                    .source_wall_id = wall->id, .type = item->type, .stud_type = item->stud_type,
                    .length_mm = item->length_mm, .depth_mm = item->depth_mm,
                    .width_mm = item->width_mm, .quantity = item->quantity
                };
            }
        }
    }
    framing_takeoff_destroy(&takeoff);
    /* Even take-off aggregation overflow without its own diagnostic source
     * can be attributed to this Wall at the conversion boundary. */
    return result(code, code == CUT_LIST_SUCCESS ? 0 : wall->id);
}

static CutListResult append_storey(CutListBuilder *builder, const Storey *storey)
{
    const BuildStructure *s = &storey->structure;
    if (!collection_valid(s->walls, s->wall_count, s->wall_capacity, sizeof *s->walls)) {
        return result(CUT_LIST_INVALID_SOURCE, 0);
    }
    for (size_t i = 0; i < s->wall_count; i++) {
        CutListResult status = append_wall(builder, &s->walls[i]);
        if (status.code != CUT_LIST_SUCCESS) { return status; }
    }
    return result(CUT_LIST_SUCCESS, 0);
}

static CutListResult finish(CutListBuilder *builder, CutListResult status, CutList *output)
{
    if (status.code == CUT_LIST_SUCCESS) {
        cut_list_destroy(output);
        *output = builder->list;
        builder->list = (CutList){0};
    }
    cut_list_destroy(&builder->list);
    return status;
}

CutListResult cut_list_build_wall(const Wall *wall, CutList *output)
{
    if (wall == NULL || output == NULL) { return result(CUT_LIST_INVALID_ARGUMENT, 0); }
    CutListBuilder builder = {0};
    CutListResult status = append_wall(&builder, wall);
    return finish(&builder, status, output);
}

CutListResult cut_list_build_storey(const Storey *storey, CutList *output)
{
    if (storey == NULL || output == NULL) { return result(CUT_LIST_INVALID_ARGUMENT, 0); }
    CutListBuilder builder = {0};
    CutListResult status = append_storey(&builder, storey);
    return finish(&builder, status, output);
}

CutListResult cut_list_build_project(const SiteHelperProject *project, CutList *output)
{
    if (project == NULL || output == NULL) { return result(CUT_LIST_INVALID_ARGUMENT, 0); }
    if (!collection_valid(project->storeys, project->storey_count, project->storey_capacity,
            sizeof *project->storeys)) {
        return result(CUT_LIST_INVALID_SOURCE, 0);
    }
    CutListBuilder builder = {0};
    CutListResult status = result(CUT_LIST_SUCCESS, 0);
    for (size_t i = 0; i < project->storey_count; i++) {
        status = append_storey(&builder, &project->storeys[i]);
        if (status.code != CUT_LIST_SUCCESS) { break; }
    }
    return finish(&builder, status, output);
}
