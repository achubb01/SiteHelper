#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static BuildSettings settings(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    return project.settings;
}

static Wall generated_wall(DomainId id, int length, const BuildSettings *s, int openings)
{
    Wall wall = {.id = id};
    assert(wall_set_plan_segment(&wall, (WallPlanSegment){{0, 0}, {length, 0}}));
    if (openings) {
        assert(wall_add_opening(&wall, s, id + 100, OPENING_WINDOW, 900, 700, 1200, 1000));
        assert(wall_add_opening(&wall, s, id + 200, OPENING_DOOR, 3500, 0, 900, 2100));
    }
    assert(wall_generate(&wall, s));
    return wall;
}

static int matches(const FramingTakeoffItem *item, const Timber *member)
{
    return item->type == member->type && item->length_mm == member->length &&
        item->depth_mm == member->depth && item->width_mm == member->width &&
        (member->type != TIMBER_STUD || item->stud_type == member->details.stud.type);
}

static const FramingTakeoffItem *find(const FramingTakeoff *report, TimberType type,
    StudType subtype, int length, int depth, int width)
{
    Timber key = {.type = type, .details.stud.type = subtype,
        .length = length, .depth = depth, .width = width};
    for (size_t i = 0; i < report->item_count; i++) {
        if (matches(&report->items[i], &key)) { return &report->items[i]; }
    }
    return NULL;
}

/* Independent, deliberately simple oracle: scan physical output for each row.
 * No opening formulas, spacing rules, sorting or aggregation implementation. */
static void verify(const FramingTakeoff *report, const Wall *const *walls, size_t wall_count)
{
    uint64_t expected_count = 0, actual_count = 0;
    for (size_t w = 0; w < wall_count; w++) {
        const WallFraming *f = &walls[w]->framing;
        expected_count += 2 + f->stud_count + f->nog_count + f->member_count;
    }
    for (size_t i = 0; i < report->item_count; i++) {
        const FramingTakeoffItem *item = &report->items[i];
        uint64_t count = 0;
        int64_t length = 0;
        for (size_t w = 0; w < wall_count; w++) {
            const WallFraming *f = &walls[w]->framing;
            count += matches(item, &f->bottomplate) + matches(item, &f->topplate);
            for (size_t j = 0; j < f->stud_count; j++) { count += matches(item, &f->studs[j]); }
            for (size_t j = 0; j < f->nog_count; j++) { count += matches(item, &f->nogs[j]); }
            for (size_t j = 0; j < f->member_count; j++) { count += matches(item, &f->members[j]); }
        }
        length = (int64_t)count * item->length_mm;
        assert(count > 0 && item->quantity == count && item->total_length_mm == length);
        if (item->type != TIMBER_STUD) { assert(item->stud_type == STUD_COMMON); }
        for (size_t j = 0; j < i; j++) {
            const FramingTakeoffItem *other = &report->items[j];
            assert(item->type != other->type || item->stud_type != other->stud_type ||
                item->length_mm != other->length_mm || item->depth_mm != other->depth_mm ||
                item->width_mm != other->width_mm);
        }
        actual_count += count;
    }
    assert(actual_count == expected_count);
}

static void equivalent(const FramingTakeoff *a, const FramingTakeoff *b)
{
    assert(a->item_count == b->item_count);
    for (size_t i = 0; i < a->item_count; i++) {
        const FramingTakeoffItem *x = &a->items[i], *y = &b->items[i];
        assert(x->type == y->type && x->stud_type == y->stud_type);
        assert(x->length_mm == y->length_mm && x->depth_mm == y->depth_mm && x->width_mm == y->width_mm);
        assert(x->quantity == y->quantity && x->total_length_mm == y->total_length_mm);
    }
}

static void test_empty_and_lifetime(void)
{
    SiteHelperProject project;
    sitehelper_project_init(&project);
    Storey empty = {0};
    FramingTakeoff report = {0};
    assert(framing_takeoff_build_project(&project, &report).code == FRAMING_TAKEOFF_SUCCESS);
    assert(report.items == NULL && report.item_count == 0);
    assert(framing_takeoff_build_storey(&empty, &report).code == FRAMING_TAKEOFF_SUCCESS);
    BuildSettings s = settings();
    Wall wall = generated_wall(1, 3600, &s, 0);
    assert(framing_takeoff_build_wall(&wall, &report).code == FRAMING_TAKEOFF_SUCCESS);
    assert(report.item_count > 0);
    wall_destroy(&wall);
    assert(find(&report, TIMBER_PLATE, STUD_COMMON, 3600, 90, 35)->quantity == 2);
    assert(framing_takeoff_build_storey(&empty, &report).code == FRAMING_TAKEOFF_SUCCESS);
    assert(report.items == NULL && report.item_count == 0);
    wall = generated_wall(2, 6000, &s, 1);
    assert(framing_takeoff_build_wall(&wall, &report).code == FRAMING_TAKEOFF_SUCCESS);
    assert(framing_takeoff_build_project(&project, &report).code == FRAMING_TAKEOFF_SUCCESS);
    assert(report.items == NULL && report.item_count == 0);
    wall_destroy(&wall);
    framing_takeoff_destroy(&report);
    framing_takeoff_destroy(&report);
    framing_takeoff_destroy(NULL);
    assert(report.items == NULL && report.item_count == 0);
    sitehelper_project_destroy(&project);
}

static void test_basic_and_wide_totals(void)
{
    BuildSettings s = settings();
    Wall wall = generated_wall(1, 3600, &s, 0);
    FramingTakeoff report = {0};
    assert(framing_takeoff_build_wall(&wall, &report).code == FRAMING_TAKEOFF_SUCCESS);
    const Wall *walls[] = {&wall};
    verify(&report, walls, 1);
    const FramingTakeoffItem *plate = find(&report, TIMBER_PLATE, STUD_COMMON, 3600, 90, 35);
    assert(plate && plate->quantity == 2 && plate->total_length_mm == 7200);
    assert(find(&report, TIMBER_STUD, STUD_COMMON, 2400, 90, 35)->quantity == wall.framing.stud_count);
    assert(wall.framing.nog_count > 0);
    assert(find(&report, TIMBER_NOGGIN, STUD_COMMON, wall.framing.nogs[0].length, 90, 35));

    /* A consumed-dimensions fixture proves wide sums and no definition lookup. */
    wall.framing.bottomplate.length = INT_MAX;
    wall.framing.topplate.length = INT_MAX;
    wall.framing.bottomplate.details.plate.placeholder = 123;
    wall.framing.topplate.details.plate.placeholder = 456;
    wall.definition.segment = (WallPlanSegment){0};
    assert(framing_takeoff_build_wall(&wall, &report).code == FRAMING_TAKEOFF_SUCCESS);
    plate = find(&report, TIMBER_PLATE, STUD_COMMON, INT_MAX, 90, 35);
    assert(plate && plate->quantity == 2 && plate->total_length_mm == 2 * (int64_t)INT_MAX);
    verify(&report, walls, 1);
    wall_destroy(&wall);
    framing_takeoff_destroy(&report);

    s.nog_spacing = s.stud_height;
    wall = generated_wall(2, 2400, &s, 0);
    assert(wall.framing.nog_count == 0 && wall.framing.member_count == 0);
    assert(framing_takeoff_build_wall(&wall, &report).code == FRAMING_TAKEOFF_SUCCESS);
    verify(&report, walls, 1);
    wall_destroy(&wall);
    framing_takeoff_destroy(&report);
}

static void test_openings_and_read_only(void)
{
    BuildSettings s = settings();
    Wall wall = generated_wall(1, 6000, &s, 1);
    Wall before = wall;
    size_t counts[] = {wall.framing.stud_count, wall.framing.nog_count, wall.framing.member_count};
    Timber *arrays[] = {wall.framing.studs, wall.framing.nogs, wall.framing.members};
    Timber *copies[3];
    for (size_t i = 0; i < 3; i++) {
        copies[i] = malloc(counts[i] * sizeof(Timber));
        assert(copies[i]);
        memcpy(copies[i], arrays[i], counts[i] * sizeof(Timber));
    }
    Opening openings[2];
    memcpy(openings, wall.definition.openings, sizeof openings);
    FramingTakeoff report = {0};
    assert(framing_takeoff_build_wall(&wall, &report).code == FRAMING_TAKEOFF_SUCCESS);
    const Wall *walls[] = {&wall};
    verify(&report, walls, 1);
    int subtypes[4] = {0}, headers = 0, sills = 0, short_studs = 0;
    for (size_t i = 0; i < report.item_count; i++) {
        const FramingTakeoffItem *item = &report.items[i];
        if (item->type == TIMBER_STUD) {
            subtypes[item->stud_type]++;
            short_studs += item->length_mm < s.stud_height;
        }
        headers += item->type == TIMBER_HEADER;
        sills += item->type == TIMBER_SILL;
    }
    for (size_t i = 0; i < 4; i++) { assert(subtypes[i] > 0); }
    assert(headers > 0 && sills > 0 && short_studs > 0);
    assert(memcmp(&before, &wall, sizeof wall) == 0);
    assert(memcmp(openings, wall.definition.openings, sizeof openings) == 0);
    for (size_t i = 0; i < 3; i++) {
        assert(memcmp(copies[i], arrays[i], counts[i] * sizeof(Timber)) == 0);
        free(copies[i]);
    }
    /* Definitions can change before regeneration: the committed framing wins. */
    wall.definition.openings[0].width += 100;
    FramingTakeoff rebuilt = {0};
    assert(framing_takeoff_build_wall(&wall, &rebuilt).code == FRAMING_TAKEOFF_SUCCESS);
    equivalent(&report, &rebuilt);
    framing_takeoff_destroy(&rebuilt);
    framing_takeoff_destroy(&report);
    wall_destroy(&wall);
}

static void test_keys_scopes_and_order(void)
{
    BuildSettings s = settings();
    Wall first[3];
    first[0] = generated_wall(1, 2400, &s, 0);
    s.stud_depth = 140;
    first[1] = generated_wall(2, 2400, &s, 0);
    s.stud_width = 45;
    first[2] = generated_wall(3, 2400, &s, 0);
    s = settings();
    Wall second[2];
    second[0] = generated_wall(4, 2400, &s, 0);
    second[1] = generated_wall(5, 6000, &s, 1);
    Storey storeys[] = {
        {.id = 20, .structure = {.walls = first, .wall_count = 3, .wall_capacity = 3}},
        {.id = 21, .structure = {.walls = second, .wall_count = 2, .wall_capacity = 2}}
    };
    SiteHelperProject project = {.storeys = storeys, .storey_count = 2, .storey_capacity = 2};
    SiteHelperProject before = project;
    FramingTakeoff all = {0}, narrow = {0}, reordered = {0};
    assert(framing_takeoff_build_project(&project, &all).code == FRAMING_TAKEOFF_SUCCESS);
    const Wall *walls[] = {&first[0], &first[1], &first[2], &second[0], &second[1]};
    verify(&all, walls, 5);
    assert(memcmp(&before, &project, sizeof project) == 0);
    assert(find(&all, TIMBER_PLATE, STUD_COMMON, 2400, 90, 35)->quantity == 4);
    assert(find(&all, TIMBER_STUD, STUD_COMMON, 2400, 90, 35));
    assert(find(&all, TIMBER_STUD, STUD_COMMON, 2400, 140, 35));
    assert(find(&all, TIMBER_STUD, STUD_COMMON, 2400, 140, 45));
    assert(find(&all, TIMBER_STUD, STUD_KING, 2400, 90, 35));
    assert(find(&all, TIMBER_PLATE, STUD_COMMON, 6000, 90, 35));
    assert(framing_takeoff_build_storey(&storeys[0], &narrow).code == FRAMING_TAKEOFF_SUCCESS);
    verify(&narrow, walls, 3);
    assert(find(&narrow, TIMBER_PLATE, STUD_COMMON, 2400, 90, 35)->quantity == 2);
    assert(framing_takeoff_build_wall(&second[0], &narrow).code == FRAMING_TAKEOFF_SUCCESS);
    verify(&narrow, &walls[3], 1);

    for (size_t i = 1; i < all.item_count; i++) {
        const FramingTakeoffItem *a = &all.items[i - 1], *b = &all.items[i];
        int ak[] = {a->type, a->stud_type, a->length_mm, a->depth_mm, a->width_mm};
        int bk[] = {b->type, b->stud_type, b->length_mm, b->depth_mm, b->width_mm};
        size_t key = 0;
        while (key < 5 && ak[key] == bk[key]) { key++; }
        assert(key < 5 && ak[key] < bk[key]);
    }
    Storey swap_storey = storeys[0]; storeys[0] = storeys[1]; storeys[1] = swap_storey;
    Wall swap_wall = first[0]; first[0] = first[2]; first[2] = swap_wall;
    for (size_t w = 0; w < 2; w++) {
        WallFraming *f = &second[w].framing;
        for (size_t i = 0; i < f->stud_count / 2; i++) {
            Timber swap = f->studs[i]; f->studs[i] = f->studs[f->stud_count - i - 1];
            f->studs[f->stud_count - i - 1] = swap;
        }
    }
    assert(framing_takeoff_build_project(&project, &reordered).code == FRAMING_TAKEOFF_SUCCESS);
    equivalent(&all, &reordered);
    for (size_t i = 0; i < 3; i++) { wall_destroy(&first[i]); }
    for (size_t i = 0; i < 2; i++) { wall_destroy(&second[i]); }
    equivalent(&all, &reordered); /* Snapshots survive all framing destruction. */
    framing_takeoff_destroy(&all);
    framing_takeoff_destroy(&narrow);
    framing_takeoff_destroy(&reordered);
}

static void test_failures_preserve_snapshot(void)
{
    BuildSettings s = settings();
    Wall good = generated_wall(1, 6000, &s, 1);
    FramingTakeoff report = {0}, expected = {0};
    assert(framing_takeoff_build_wall(&good, &report).code == FRAMING_TAKEOFF_SUCCESS);
    assert(framing_takeoff_build_wall(&good, &expected).code == FRAMING_TAKEOFF_SUCCESS);
    FramingTakeoff original = report;
    Wall bad = {.id = 99};
    for (int fault = 0; fault < 17; fault++) {
        bad = good; bad.id = 99; /* Borrowed corruption fixture, never destroyed. */
        Timber stud = good.framing.studs[0], nog = good.framing.nogs[0];
        Timber member = good.framing.members[0];
        switch (fault) {
        case 0: bad.framing = (WallFraming){0}; break;
        case 1: bad.framing.bottomplate.length = 0; break;
        case 2: bad.framing.topplate.type = TIMBER_STUD; break;
        case 3: bad.framing.studs = NULL; break;
        case 4: bad.framing.stud_count = 0; break;
        case 5: bad.framing.stud_count = bad.framing.stud_capacity + 1; break;
        case 6: bad.framing.nogs = NULL; break;
        case 7: bad.framing.nog_count = bad.framing.nog_capacity + 1; break;
        case 8: bad.framing.members = NULL; break;
        case 9: bad.framing.member_count = bad.framing.member_capacity + 1; break;
        case 10: good.framing.studs[0].type = TIMBER_PLATE; break;
        case 11: good.framing.studs[0].details.stud.type = (StudType)99; break;
        case 12: good.framing.nogs[0].type = TIMBER_STUD; break;
        case 13: good.framing.members[0].type = (TimberType)99; break;
        case 14: good.framing.members[0].depth = -1; break;
        case 15: good.framing.nogs[0].width = 0; break;
        case 16: bad.framing.stud_capacity = SIZE_MAX; break;
        }
        FramingTakeoffResult status = framing_takeoff_build_wall(&bad, &report);
        assert(status.code == FRAMING_TAKEOFF_INVALID_FRAMING && status.wall_id == 99);
        assert(memcmp(&original, &report, sizeof report) == 0);
        equivalent(&report, &expected);
        good.framing.studs[0] = stud; good.framing.nogs[0] = nog; good.framing.members[0] = member;
    }
    Wall walls[] = {good, {.id = 99}};
    Storey storey = {.structure = {.walls = walls, .wall_count = 2, .wall_capacity = 2}};
    Storey storeys[] = {{0}, storey};
    SiteHelperProject project = {.storeys = storeys, .storey_count = 2, .storey_capacity = 2};
    assert(framing_takeoff_build_storey(&storey, &report).code == FRAMING_TAKEOFF_INVALID_FRAMING);
    FramingTakeoffResult status = framing_takeoff_build_project(&project, &report);
    assert(status.code == FRAMING_TAKEOFF_INVALID_FRAMING && status.wall_id == 99);
    assert(framing_takeoff_build_wall(NULL, &report).code == FRAMING_TAKEOFF_INVALID_ARGUMENT);
    assert(framing_takeoff_build_storey(NULL, &report).code == FRAMING_TAKEOFF_INVALID_ARGUMENT);
    assert(framing_takeoff_build_project(NULL, &report).code == FRAMING_TAKEOFF_INVALID_ARGUMENT);
    assert(framing_takeoff_build_wall(&good, NULL).code == FRAMING_TAKEOFF_INVALID_ARGUMENT);
    assert(framing_takeoff_build_storey(&storey, NULL).code == FRAMING_TAKEOFF_INVALID_ARGUMENT);
    assert(framing_takeoff_build_project(&project, NULL).code == FRAMING_TAKEOFF_INVALID_ARGUMENT);
    storey.structure.walls = NULL;
    assert(framing_takeoff_build_storey(&storey, &report).code == FRAMING_TAKEOFF_INVALID_SOURCE);
    project.storey_count = 3;
    assert(framing_takeoff_build_project(&project, &report).code == FRAMING_TAKEOFF_INVALID_SOURCE);
    project.storey_count = 2; project.storeys = NULL;
    assert(framing_takeoff_build_project(&project, &report).code == FRAMING_TAKEOFF_INVALID_SOURCE);
    assert(memcmp(&original, &report, sizeof report) == 0);
    equivalent(&report, &expected);
    wall_destroy(&good);
    framing_takeoff_destroy(&report);
    framing_takeoff_destroy(&expected);
}

#ifdef SITEHELPER_TEST_WRAP_ALLOC
static void test_allocation_failures(void)
{
    BuildSettings s = settings();
    Wall walls[] = {generated_wall(1, 6000, &s, 1), generated_wall(2, 2400, &s, 0)};
    Storey storey = {.structure = {.walls = walls, .wall_count = 2, .wall_capacity = 2}};
    SiteHelperProject project = {.storeys = &storey, .storey_count = 1, .storey_capacity = 1};
    size_t failures = 0;
    for (int populated = 0; populated < 2; populated++) {
        for (size_t n = 0; ; n++) {
            FramingTakeoff output = {0}, expected = {0};
            if (populated) {
                assert(framing_takeoff_build_wall(&walls[1], &output).code == FRAMING_TAKEOFF_SUCCESS);
                assert(framing_takeoff_build_wall(&walls[1], &expected).code == FRAMING_TAKEOFF_SUCCESS);
            }
            FramingTakeoff original = output;
            allocation_failed = 0; fail_after = n;
            FramingTakeoffResult status = framing_takeoff_build_project(&project, &output);
            fail_after = SIZE_MAX;
            if (allocation_failed) {
                failures++;
                assert(status.code == FRAMING_TAKEOFF_ALLOCATION_FAILED);
                assert(memcmp(&original, &output, sizeof output) == 0);
                equivalent(&output, &expected);
                assert(framing_takeoff_build_project(&project, &output).code == FRAMING_TAKEOFF_SUCCESS);
            } else { assert(status.code == FRAMING_TAKEOFF_SUCCESS); }
            const Wall *sources[] = {&walls[0], &walls[1]};
            verify(&output, sources, 2);
            framing_takeoff_destroy(&output);
            framing_takeoff_destroy(&expected);
            if (!allocation_failed) { break; }
            assert(n < 64);
        }
    }
    assert(failures > 2);
    printf("take-off allocation failures checked: %zu\n", failures);
    wall_destroy(&walls[0]); wall_destroy(&walls[1]);
}
#endif

int main(void)
{
    test_empty_and_lifetime();
    test_basic_and_wide_totals();
    test_openings_and_read_only();
    test_keys_scopes_and_order();
    test_failures_preserve_snapshot();
#ifdef SITEHELPER_TEST_WRAP_ALLOC
    test_allocation_failures();
#endif
    puts("framing take-off tests passed");
    return 0;
}
