#include <stdlib.h>
#include "framing_takeoff.h"

typedef struct {
    FramingTakeoff report;
    size_t capacity;
} TakeoffBuilder;

static FramingTakeoffResult result(FramingTakeoffCode code, DomainId wall_id)
{
    return (FramingTakeoffResult){code, wall_id};
}

static int collection_valid(const void *data, size_t count, size_t capacity, size_t size)
{
    return count <= capacity && capacity <= SIZE_MAX / size &&
        ((capacity == 0 && data == NULL) || (capacity != 0 && data != NULL));
}

void framing_takeoff_destroy(FramingTakeoff *takeoff)
{
    if (takeoff == NULL) { return; }
    free(takeoff->items);
    *takeoff = (FramingTakeoff){0};
}

static FramingTakeoffCode append_member(TakeoffBuilder *builder, const Timber *member)
{
    if (member->length <= 0 || member->depth <= 0 || member->width <= 0 ||
        member->type < TIMBER_STUD || member->type > TIMBER_SILL ||
        (member->type == TIMBER_STUD &&
            (member->details.stud.type < STUD_COMMON || member->details.stud.type > STUD_CRIPPLE))) {
        return FRAMING_TAKEOFF_INVALID_FRAMING;
    }
    if (builder->report.item_count == builder->capacity) {
        size_t maximum = SIZE_MAX / sizeof *builder->report.items;
        size_t capacity = builder->capacity;
        if (capacity >= maximum) { return FRAMING_TAKEOFF_NUMERIC_OVERFLOW; }
        size_t grown = capacity == 0 ? 1 : capacity > maximum / 2 ? maximum : capacity * 2;
        FramingTakeoffItem *items = realloc(builder->report.items, grown * sizeof *items);
        if (items == NULL) { return FRAMING_TAKEOFF_ALLOCATION_FAILED; }
        builder->report.items = items;
        builder->capacity = grown;
    }
    builder->report.items[builder->report.item_count++] = (FramingTakeoffItem){
        .type = member->type,
        .stud_type = member->type == TIMBER_STUD ? member->details.stud.type : STUD_COMMON,
        .length_mm = member->length, .depth_mm = member->depth, .width_mm = member->width,
        .quantity = 1, .total_length_mm = member->length
    };
    return FRAMING_TAKEOFF_SUCCESS;
}

static FramingTakeoffResult append_wall(TakeoffBuilder *builder, const Wall *wall)
{
    const WallFraming *f = &wall->framing;
    if (f->bottomplate.type != TIMBER_PLATE || f->topplate.type != TIMBER_PLATE ||
        f->stud_count < 2 ||
        !collection_valid(f->studs, f->stud_count, f->stud_capacity, sizeof *f->studs) ||
        !collection_valid(f->nogs, f->nog_count, f->nog_capacity, sizeof *f->nogs) ||
        !collection_valid(f->members, f->member_count, f->member_capacity, sizeof *f->members)) {
        return result(FRAMING_TAKEOFF_INVALID_FRAMING, wall->id);
    }
    /* Each location owns distinct physical members. No opening traversal. */
    const Timber *arrays[] = {&f->bottomplate, &f->topplate, f->studs, f->nogs, f->members};
    const size_t counts[] = {1, 1, f->stud_count, f->nog_count, f->member_count};
    for (size_t group = 0; group < 5; group++) {
        for (size_t i = 0; i < counts[group]; i++) {
            const Timber *member = &arrays[group][i];
            if ((group == 2 && member->type != TIMBER_STUD) ||
                (group == 3 && member->type != TIMBER_NOGGIN)) {
                return result(FRAMING_TAKEOFF_INVALID_FRAMING, wall->id);
            }
            FramingTakeoffCode code = append_member(builder, member);
            if (code != FRAMING_TAKEOFF_SUCCESS) { return result(code, wall->id); }
        }
    }
    return result(FRAMING_TAKEOFF_SUCCESS, 0);
}

static FramingTakeoffResult append_storey(TakeoffBuilder *builder, const Storey *storey)
{
    const BuildStructure *s = &storey->structure;
    if (!collection_valid(s->walls, s->wall_count, s->wall_capacity, sizeof *s->walls)) {
        return result(FRAMING_TAKEOFF_INVALID_SOURCE, 0);
    }
    for (size_t i = 0; i < s->wall_count; i++) {
        FramingTakeoffResult status = append_wall(builder, &s->walls[i]);
        if (status.code != FRAMING_TAKEOFF_SUCCESS) { return status; }
    }
    return result(FRAMING_TAKEOFF_SUCCESS, 0);
}

static int item_compare(const void *pa, const void *pb)
{
    const FramingTakeoffItem *a = pa, *b = pb;
    if (a->type != b->type) { return a->type < b->type ? -1 : 1; }
    if (a->type == TIMBER_STUD && a->stud_type != b->stud_type) {
        return a->stud_type < b->stud_type ? -1 : 1;
    }
    if (a->length_mm != b->length_mm) { return a->length_mm < b->length_mm ? -1 : 1; }
    if (a->depth_mm != b->depth_mm) { return a->depth_mm < b->depth_mm ? -1 : 1; }
    return (a->width_mm > b->width_mm) - (a->width_mm < b->width_mm);
}

static FramingTakeoffResult finish(TakeoffBuilder *builder, FramingTakeoffResult status,
    FramingTakeoff *output)
{
    FramingTakeoff *candidate = &builder->report;
    if (status.code == FRAMING_TAKEOFF_SUCCESS) {
        if (candidate->item_count > 1) {
            qsort(candidate->items, candidate->item_count, sizeof *candidate->items, item_compare);
        }
        size_t unique = 0;
        for (size_t i = 0; i < candidate->item_count; i++) {
            FramingTakeoffItem next = candidate->items[i];
            if (unique != 0 && item_compare(&candidate->items[unique - 1], &next) == 0) {
                FramingTakeoffItem *item = &candidate->items[unique - 1];
                if (item->quantity > UINT64_MAX - next.quantity ||
                    item->total_length_mm > INT64_MAX - next.total_length_mm) {
                    status = result(FRAMING_TAKEOFF_NUMERIC_OVERFLOW, 0);
                    break;
                }
                item->quantity += next.quantity;
                item->total_length_mm += next.total_length_mm;
            } else {
                candidate->items[unique++] = next;
            }
        }
        if (status.code == FRAMING_TAKEOFF_SUCCESS) {
            candidate->item_count = unique;
            framing_takeoff_destroy(output);
            *output = *candidate;
            *candidate = (FramingTakeoff){0};
        }
    }
    framing_takeoff_destroy(candidate);
    return status;
}

FramingTakeoffResult framing_takeoff_build_wall(const Wall *wall, FramingTakeoff *output)
{
    if (wall == NULL || output == NULL) { return result(FRAMING_TAKEOFF_INVALID_ARGUMENT, 0); }
    TakeoffBuilder builder = {0};
    FramingTakeoffResult status = append_wall(&builder, wall);
    return finish(&builder, status, output);
}

FramingTakeoffResult framing_takeoff_build_storey(const Storey *storey, FramingTakeoff *output)
{
    if (storey == NULL || output == NULL) { return result(FRAMING_TAKEOFF_INVALID_ARGUMENT, 0); }
    TakeoffBuilder builder = {0};
    FramingTakeoffResult status = append_storey(&builder, storey);
    return finish(&builder, status, output);
}

FramingTakeoffResult framing_takeoff_build_project(const SiteHelperProject *project, FramingTakeoff *output)
{
    if (project == NULL || output == NULL) { return result(FRAMING_TAKEOFF_INVALID_ARGUMENT, 0); }
    if (!collection_valid(project->storeys, project->storey_count, project->storey_capacity,
            sizeof *project->storeys)) {
        return result(FRAMING_TAKEOFF_INVALID_SOURCE, 0);
    }
    TakeoffBuilder builder = {0};
    FramingTakeoffResult status = result(FRAMING_TAKEOFF_SUCCESS, 0);
    for (size_t i = 0; i < project->storey_count; i++) {
        status = append_storey(&builder, &project->storeys[i]);
        if (status.code != FRAMING_TAKEOFF_SUCCESS) { break; }
    }
    return finish(&builder, status, output);
}
