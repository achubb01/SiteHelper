#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cut_list.h"
#include "framing_takeoff.h"
#include "wall.h"

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static size_t fail_after = SIZE_MAX;
static int allocation_failed;
void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static int reject_allocation(void)
{
    if (fail_after != SIZE_MAX && fail_after-- == 0) {
        allocation_failed = 1;
        return 1;
    }
    return 0;
}
void *__wrap_malloc(size_t size) { return reject_allocation() ? NULL : __real_malloc(size); }
void *__wrap_calloc(size_t count, size_t size) { return reject_allocation() ? NULL : __real_calloc(count, size); }
void *__wrap_realloc(void *p, size_t size) { return reject_allocation() ? NULL : __real_realloc(p, size); }
#endif

static DomainId add_wall(SiteHelperProject *project, DomainId storey_id, int openings)
{
    DomainId id = sitehelper_project_add_wall(project, storey_id,
        (WallPlanSegment){{0, 0}, {6000, 0}});
    assert(id != DOMAIN_ID_INVALID);
    Wall *wall = sitehelper_project_find_wall_by_id(project, id);
    if (openings) {
        DomainId window = domain_id_generate(&project->domain_ids);
        DomainId door = domain_id_generate(&project->domain_ids);
        assert(wall_add_opening(wall, &project->settings, window, OPENING_WINDOW, 900, 700, 1200, 1000));
        assert(wall_add_opening(wall, &project->settings, door, OPENING_DOOR, 3500, 0, 900, 2100));
    }
    assert(wall_generate(wall, &project->settings));
    return id;
}

static void fixture(SiteHelperProject *project)
{
    sitehelper_project_init(project);
    DomainId first = sitehelper_project_add_storey(project, 0);
    DomainId second = sitehelper_project_add_storey(project, 3000);
    assert(first && second);
    add_wall(project, first, 0);
    add_wall(project, first, 0);
    add_wall(project, second, 1);
}

/* Priority 21 is the oracle. Check one complete source block, including order,
 * purpose, dimensions, aggregation and provenance; never traverse Timber here. */
static size_t assert_wall_block(const CutList *list, size_t offset, const Wall *wall)
{
    FramingTakeoff takeoff = {0};
    assert(framing_takeoff_build_wall(wall, &takeoff).code == FRAMING_TAKEOFF_SUCCESS);
    assert(offset <= list->member_count && takeoff.item_count <= list->member_count - offset);
    for (size_t i = 0; i < takeoff.item_count; i++) {
        const RequiredMember *member = &list->members[offset + i];
        const FramingTakeoffItem *item = &takeoff.items[i];
        assert(member->source_wall_id == wall->id);
        assert(member->type == item->type && member->stud_type == item->stud_type);
        assert(member->length_mm == item->length_mm && member->depth_mm == item->depth_mm);
        assert(member->width_mm == item->width_mm && member->quantity == item->quantity);
        if (member->type != TIMBER_STUD) { assert(member->stud_type == STUD_COMMON); }
    }
    offset += takeoff.item_count;
    framing_takeoff_destroy(&takeoff);
    return offset;
}

static void assert_project_blocks(const CutList *list, const SiteHelperProject *project)
{
    size_t offset = 0;
    for (size_t i = 0; i < project->storey_count; i++) {
        const BuildStructure *s = &project->storeys[i].structure;
        for (size_t j = 0; j < s->wall_count; j++) {
            offset = assert_wall_block(list, offset, &s->walls[j]);
        }
    }
    assert(offset == list->member_count);
}

/* A byte copy of an owned result is only a test oracle, never another owner. */
static RequiredMember *copy_members(const CutList *list)
{
    if (list->member_count == 0) { return NULL; }
    RequiredMember *copy = malloc(list->member_count * sizeof *copy);
    assert(copy);
    memcpy(copy, list->members, list->member_count * sizeof *copy);
    return copy;
}

static void assert_unchanged(const CutList *list, const CutList *before, const RequiredMember *copy)
{
    assert(memcmp(list, before, sizeof *list) == 0);
    if (list->member_count != 0) {
        assert(memcmp(list->members, copy, list->member_count * sizeof *copy) == 0);
    }
}

/* Byte guards also protect input pointers, metadata and physical array order. */
typedef struct { const void *source; void *copy; size_t size; } ByteGuard;
static void guard_add(ByteGuard *guards, size_t *count, const void *source, size_t size)
{
    if (size == 0) { return; }
    assert(*count < 32);
    void *copy = malloc(size);
    assert(copy);
    memcpy(copy, source, size);
    guards[(*count)++] = (ByteGuard){source, copy, size};
}

static size_t guard_project(ByteGuard *guards, const SiteHelperProject *project)
{
    size_t count = 0;
    guard_add(guards, &count, project, sizeof *project);
    guard_add(guards, &count, project->storeys, project->storey_count * sizeof *project->storeys);
    for (size_t i = 0; i < project->storey_count; i++) {
        const BuildStructure *s = &project->storeys[i].structure;
        guard_add(guards, &count, s->walls, s->wall_count * sizeof *s->walls);
        for (size_t j = 0; j < s->wall_count; j++) {
            const Wall *wall = &s->walls[j];
            const WallFraming *f = &wall->framing;
            guard_add(guards, &count, wall->definition.openings,
                wall->definition.opening_count * sizeof *wall->definition.openings);
            guard_add(guards, &count, f->studs, f->stud_count * sizeof *f->studs);
            guard_add(guards, &count, f->nogs, f->nog_count * sizeof *f->nogs);
            guard_add(guards, &count, f->members, f->member_count * sizeof *f->members);
        }
    }
    return count;
}

static void guards_check_destroy(ByteGuard *guards, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        assert(memcmp(guards[i].source, guards[i].copy, guards[i].size) == 0);
        free(guards[i].copy);
    }
}

static void test_empty_and_destruction(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    Storey storey = {0};
    CutList list = {0};
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
    assert(list.members == NULL && list.member_count == 0);
    assert(cut_list_build_storey(&storey, &list).code == CUT_LIST_SUCCESS);
    assert(list.members == NULL && list.member_count == 0);
    Wall ungenerated = {.id = 42};
    CutListResult status = cut_list_build_wall(&ungenerated, &list);
    assert(status.code == CUT_LIST_INVALID_FRAMING && status.wall_id == 42);
    assert(list.members == NULL && list.member_count == 0);
    cut_list_destroy(&list);
    cut_list_destroy(&list);
    cut_list_destroy(NULL);
    assert(list.members == NULL && list.member_count == 0);
    sitehelper_project_destroy(&project);
}

static void test_provenance_scopes_order_and_inputs(void)
{
    SiteHelperProject project;
    fixture(&project);
    ByteGuard guards[32];
    size_t guard_count = guard_project(guards, &project);
    CutList list = {0}, narrow = {0};
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
    assert_project_blocks(&list, &project);
    Wall *first = &project.storeys[0].structure.walls[0];
    Wall *second = &project.storeys[0].structure.walls[1];
    size_t block = assert_wall_block(&list, 0, first);
    assert(list.members[0].source_wall_id != list.members[block].source_wall_id);
    assert(list.members[0].quantity > 1); /* Common studs aggregate within a Wall. */
    for (size_t i = 0; i < block; i++) {
        const RequiredMember *a = &list.members[i], *b = &list.members[block + i];
        assert(a->source_wall_id == first->id && b->source_wall_id == second->id);
        assert(a->type == b->type && a->stud_type == b->stud_type);
        assert(a->length_mm == b->length_mm && a->depth_mm == b->depth_mm && a->width_mm == b->width_mm);
        assert(a->quantity == b->quantity); /* Never a cross-Wall sum. */
        if (a->type == TIMBER_PLATE) { assert(a->quantity == 2); }
    }
    assert(cut_list_build_storey(&project.storeys[0], &narrow).code == CUT_LIST_SUCCESS);
    assert(narrow.member_count == 2 * block);
    assert(assert_wall_block(&narrow, block, second) == narrow.member_count);
    assert(cut_list_build_wall(second, &narrow).code == CUT_LIST_SUCCESS);
    assert(assert_wall_block(&narrow, 0, second) == narrow.member_count);
    guards_check_destroy(guards, guard_count);

    /* IDs now run against stored order. The source blocks must follow storage,
     * unlike Priority 21's project-wide order, and repeated queries agree. */
    Wall swap_wall = *first; *first = *second; *second = swap_wall;
    Storey swap_storey = project.storeys[0];
    project.storeys[0] = project.storeys[1]; project.storeys[1] = swap_storey;
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
    assert_project_blocks(&list, &project);
    assert(list.members[0].source_wall_id == project.storeys[0].structure.walls[0].id);
    assert(cut_list_build_project(&project, &narrow).code == CUT_LIST_SUCCESS);
    assert_project_blocks(&narrow, &project);
    assert(narrow.member_count == list.member_count);
    cut_list_destroy(&narrow);
    cut_list_destroy(&list);
    assert(list.members == NULL && list.member_count == 0);
    cut_list_destroy(&list);
    sitehelper_project_destroy(&project);
}

static void test_openings_sections_and_snapshot_replacement(void)
{
    SiteHelperProject project;
    fixture(&project);
    Wall *wall = &project.storeys[1].structure.walls[0];
    CutList list = {0};
    assert(cut_list_build_wall(wall, &list).code == CUT_LIST_SUCCESS);
    assert(assert_wall_block(&list, 0, wall) == list.member_count);
    unsigned subtypes = 0;
    int headers = 0, sills = 0;
    for (size_t i = 0; i < list.member_count; i++) {
        RequiredMember m = list.members[i];
        if (m.type == TIMBER_STUD) { subtypes |= 1u << m.stud_type; }
        headers += m.type == TIMBER_HEADER;
        sills += m.type == TIMBER_SILL;
    }
    assert(subtypes == ((1u << STUD_COMMON) | (1u << STUD_KING) |
        (1u << STUD_TRIMMER) | (1u << STUD_CRIPPLE)));
    assert(headers > 0 && sills > 0);
    CutList before = list;
    RequiredMember *copy = copy_members(&list);

    /* Lower-level generation with distinct resolved sections; no new model
     * overrides are needed to exercise report field preservation. */
    BuildSettings s = project.settings;
    s.stud_depth = 140;
    assert(wall_generate(&project.storeys[0].structure.walls[0], &s));
    s.stud_width = 45;
    assert(wall_generate(wall, &s));
    assert_unchanged(&list, &before, copy);
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
    assert_project_blocks(&list, &project);
    int sections[3] = {0};
    for (size_t i = 0; i < list.member_count; i++) {
        RequiredMember m = list.members[i];
        sections[0] += m.depth_mm == 90 && m.width_mm == 35;
        sections[1] += m.depth_mm == 140 && m.width_mm == 35;
        sections[2] += m.depth_mm == 140 && m.width_mm == 45;
    }
    assert(sections[0] && sections[1] && sections[2]);
    free(copy);
    before = list; copy = copy_members(&list);
    sitehelper_project_destroy(&project);
    assert_unchanged(&list, &before, copy);
    free(copy);
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
    assert(list.members == NULL && list.member_count == 0);
    cut_list_destroy(&list);
}

static void test_failure_transactions(void)
{
    SiteHelperProject project;
    fixture(&project);
    CutList list = {0};
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
    CutList before = list;
    RequiredMember *copy = copy_members(&list);
    Wall *wall = &project.storeys[1].structure.walls[0];
    WallFraming saved = wall->framing;
    /* Fail in the last Wall after earlier blocks have already been built. */
    for (int fault = 0; fault < 6; fault++) {
        switch (fault) {
        case 0: wall->framing = (WallFraming){0}; break;
        case 1: wall->framing.bottomplate.length = 0; break;
        case 2: wall->framing.topplate.width = -1; break;
        case 3: wall->framing.studs = NULL; break;
        case 4: wall->framing.nog_count = wall->framing.nog_capacity + 1; break;
        case 5: wall->framing.members = NULL; break;
        }
        FramingTakeoff takeoff = {0};
        assert(framing_takeoff_build_wall(wall, &takeoff).code == FRAMING_TAKEOFF_INVALID_FRAMING);
        CutListResult status = cut_list_build_project(&project, &list);
        assert(status.code == CUT_LIST_INVALID_FRAMING && status.wall_id == wall->id);
        assert_unchanged(&list, &before, copy);
        status = cut_list_build_storey(&project.storeys[1], &list);
        assert(status.code == CUT_LIST_INVALID_FRAMING && status.wall_id == wall->id);
        assert_unchanged(&list, &before, copy);
        status = cut_list_build_wall(wall, &list);
        assert(status.code == CUT_LIST_INVALID_FRAMING && status.wall_id == wall->id);
        assert_unchanged(&list, &before, copy);
        wall->framing = saved;
    }
    assert(cut_list_build_wall(NULL, &list).code == CUT_LIST_INVALID_ARGUMENT);
    assert(cut_list_build_storey(NULL, &list).code == CUT_LIST_INVALID_ARGUMENT);
    assert(cut_list_build_project(NULL, &list).code == CUT_LIST_INVALID_ARGUMENT);
    assert(cut_list_build_wall(wall, NULL).code == CUT_LIST_INVALID_ARGUMENT);
    assert(cut_list_build_storey(&project.storeys[0], NULL).code == CUT_LIST_INVALID_ARGUMENT);
    assert(cut_list_build_project(&project, NULL).code == CUT_LIST_INVALID_ARGUMENT);
    DomainId id = wall->id; wall->id = DOMAIN_ID_INVALID;
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_INVALID_SOURCE);
    wall->id = id;
    Storey invalid = project.storeys[1];
    invalid.structure.walls = NULL;
    assert(cut_list_build_storey(&invalid, &list).code == CUT_LIST_INVALID_SOURCE);
    invalid = project.storeys[1]; invalid.structure.wall_capacity = SIZE_MAX;
    assert(cut_list_build_storey(&invalid, &list).code == CUT_LIST_INVALID_SOURCE);
    SiteHelperProject invalid_project = project;
    invalid_project.storey_count = invalid_project.storey_capacity + 1;
    assert(cut_list_build_project(&invalid_project, &list).code == CUT_LIST_INVALID_SOURCE);
    invalid_project = project; invalid_project.storeys = NULL;
    assert(cut_list_build_project(&invalid_project, &list).code == CUT_LIST_INVALID_SOURCE);
    /* Enclosing metadata failure after a successful Storey is transactional. */
    Wall *walls = project.storeys[1].structure.walls;
    project.storeys[1].structure.walls = NULL;
    assert(cut_list_build_project(&project, &list).code == CUT_LIST_INVALID_SOURCE);
    project.storeys[1].structure.walls = walls;
    assert_unchanged(&list, &before, copy);
    assert(cut_list_build_wall(wall, &list).code == CUT_LIST_SUCCESS);
    assert(assert_wall_block(&list, 0, wall) == list.member_count);
    free(copy);
    cut_list_destroy(&list);
    sitehelper_project_destroy(&project);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    SiteHelperProject project;
    fixture(&project);
    ByteGuard guards[32];
    size_t guard_count = guard_project(guards, &project);
    size_t failures = 0;
    int later_wall_failed = 0;
    for (int populated = 0; populated < 2; populated++) {
        for (size_t n = 0; ; n++) {
            CutList list = {0};
            if (populated) {
                assert(cut_list_build_wall(&project.storeys[0].structure.walls[0], &list).code == CUT_LIST_SUCCESS);
            }
            CutList before = list;
            RequiredMember *copy = copy_members(&list);
            fail_after = n; allocation_failed = 0;
            CutListResult status = cut_list_build_project(&project, &list);
            fail_after = SIZE_MAX;
            if (allocation_failed) {
                failures++;
                assert(status.code == CUT_LIST_ALLOCATION_FAILED);
                assert(sitehelper_project_find_wall_by_id(&project, status.wall_id));
                later_wall_failed |= status.wall_id == project.storeys[1].structure.walls[0].id;
                assert_unchanged(&list, &before, copy);
                assert(cut_list_build_project(&project, &list).code == CUT_LIST_SUCCESS);
            } else { assert(status.code == CUT_LIST_SUCCESS); }
            assert_project_blocks(&list, &project);
            free(copy);
            cut_list_destroy(&list);
            if (!allocation_failed) { break; }
            assert(n < 128);
        }
    }
    assert(failures > 2 && later_wall_failed);
    printf("cut-list allocation failures checked: %zu\n", failures);
    guards_check_destroy(guards, guard_count);
    sitehelper_project_destroy(&project);
}
#endif

int main(void)
{
    test_empty_and_destruction();
    test_provenance_scopes_order_and_inputs();
    test_openings_sections_and_snapshot_replacement();
    test_failure_transactions();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("cut-list tests passed");
    return 0;
}
